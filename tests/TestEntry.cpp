/*
 *  Copyright (C) 2018 KeePassXC Team <team@keepassxc.org>
 *  Copyright (C) 2013 Felix Geyer <debfx@fobos.de>
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, version 3.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <QTest>

#include "TestEntry.h"
#include "core/Clock.h"
#include "core/Group.h"
#include "core/Metadata.h"
#include "core/TimeInfo.h"
#include "crypto/Crypto.h"

QTEST_GUILESS_MAIN(TestEntry)

void TestEntry::initTestCase()
{
	QVERIFY(Crypto::init());
}

void TestEntry::testHistoryItemDeletion()
{
	QScopedPointer<Entry> entry(new Entry());
	QPointer<Entry> historyEntry = new Entry();

	entry->addHistoryItem(historyEntry);
	QCOMPARE(entry->historyItems().size(), 1);

	QList<Entry*> historyEntriesToRemove;
	historyEntriesToRemove.append(historyEntry);
	entry->removeHistoryItems(historyEntriesToRemove);
	QCOMPARE(entry->historyItems().size(), 0);
	QVERIFY(historyEntry.isNull());
}

void TestEntry::testCopyDataFrom()
{
	QScopedPointer<Entry> entry(new Entry());

	entry->setTitle(QStringLiteral("testtitle"));
	entry->attributes()->set(QStringLiteral("attr1"), QStringLiteral("abc"));
	entry->attributes()->set(QStringLiteral("attr2"), QStringLiteral("def"));

	entry->attachments()->set(QStringLiteral("test"), QByteArray("123"));
	entry->attachments()->set(QStringLiteral("test2"), QByteArray("456"));

	QScopedPointer<Entry> entry2(new Entry());
	entry2->copyDataFrom(entry.data());

	QCOMPARE(entry2->title(), QStringLiteral("testtitle"));
	QCOMPARE(entry2->attributes()->value(QStringLiteral("attr1")), QStringLiteral("abc"));
	QCOMPARE(entry2->attributes()->value(QStringLiteral("attr2")), QStringLiteral("def"));

	QCOMPARE(entry2->attachments()->keys().size(), 2);
	QCOMPARE(entry2->attachments()->value(QStringLiteral("test")), QByteArray("123"));
	QCOMPARE(entry2->attachments()->value(QStringLiteral("test2")), QByteArray("456"));
}

void TestEntry::testClone()
{
	QScopedPointer<Entry> entryOrg(new Entry());
	entryOrg->setUuid(QUuid::createUuid());
	entryOrg->setPassword(QStringLiteral("pass"));
	entryOrg->setTitle(QStringLiteral("Original Title"));
	entryOrg->beginUpdate();
	entryOrg->setTitle(QStringLiteral("New Title"));
	entryOrg->endUpdate();
	TimeInfo entryOrgTime = entryOrg->timeInfo();
	QDateTime dateTime = Clock::datetimeUtc(60);
	entryOrgTime.setCreationTime(dateTime);
	entryOrg->setTimeInfo(entryOrgTime);

	QScopedPointer<Entry> entryCloneNone(entryOrg->clone(Entry::CloneNoFlags));
	QCOMPARE(entryCloneNone->uuid(), entryOrg->uuid());
	QCOMPARE(entryCloneNone->title(), QStringLiteral("New Title"));
	QCOMPARE(entryCloneNone->historyItems().size(), 0);
	QCOMPARE(entryCloneNone->timeInfo().creationTime(), entryOrg->timeInfo().creationTime());

	QScopedPointer<Entry> entryCloneNewUuid(entryOrg->clone(Entry::CloneNewUuid));
	QVERIFY(entryCloneNewUuid->uuid() != entryOrg->uuid());
	QVERIFY(!entryCloneNewUuid->uuid().isNull());
	QCOMPARE(entryCloneNewUuid->title(), QStringLiteral("New Title"));
	QCOMPARE(entryCloneNewUuid->historyItems().size(), 0);
	QCOMPARE(entryCloneNewUuid->timeInfo().creationTime(), entryOrg->timeInfo().creationTime());

	// Reset modification time
	entryOrgTime.setLastModificationTime(Clock::datetimeUtc(60));
	entryOrg->setTimeInfo(entryOrgTime);

	QScopedPointer<Entry> entryCloneRename(entryOrg->clone(Entry::CloneRenameTitle));
	QCOMPARE(entryCloneRename->uuid(), entryOrg->uuid());
	QCOMPARE(entryCloneRename->title(), QStringLiteral("New Title - Clone"));
	// Cloning should not modify time info unless explicity requested
	QCOMPARE(entryCloneRename->timeInfo(), entryOrg->timeInfo());

	QScopedPointer<Entry> entryCloneResetTime(entryOrg->clone(Entry::CloneResetTimeInfo));
	QCOMPARE(entryCloneResetTime->uuid(), entryOrg->uuid());
	QCOMPARE(entryCloneResetTime->title(), QStringLiteral("New Title"));
	QCOMPARE(entryCloneResetTime->historyItems().size(), 0);
	QVERIFY(entryCloneResetTime->timeInfo().creationTime() != entryOrg->timeInfo().creationTime());

	// Date back history of original entry
	Entry *firstHistoryItem = entryOrg->historyItems()[0];
	TimeInfo entryOrgHistoryTimeInfo = firstHistoryItem->timeInfo();
	QDateTime datedBackEntryOrgModificationTime = entryOrgHistoryTimeInfo.lastModificationTime().addMSecs(-10);
	entryOrgHistoryTimeInfo.setLastModificationTime(datedBackEntryOrgModificationTime);
	entryOrgHistoryTimeInfo.setCreationTime(datedBackEntryOrgModificationTime);
	firstHistoryItem->setTimeInfo(entryOrgHistoryTimeInfo);

	QScopedPointer<Entry> entryCloneHistory(entryOrg->clone(Entry::CloneIncludeHistory | Entry::CloneResetTimeInfo));
	QCOMPARE(entryCloneHistory->uuid(), entryOrg->uuid());
	QCOMPARE(entryCloneHistory->title(), QStringLiteral("New Title"));
	QCOMPARE(entryCloneHistory->historyItems().size(), entryOrg->historyItems().size());
	QCOMPARE(entryCloneHistory->historyItems().at(0)->title(), QStringLiteral("Original Title"));
	QVERIFY(entryCloneHistory->timeInfo().creationTime() != entryOrg->timeInfo().creationTime());
	// Timeinfo of history items should not be modified
	QList<Entry*> entryOrgHistory = entryOrg->historyItems(), clonedHistory = entryCloneHistory->historyItems();
	auto entryOrgHistoryItem = entryOrgHistory.constBegin();
	for (auto entryCloneHistoryItem = clonedHistory.constBegin(); entryCloneHistoryItem != clonedHistory.constEnd();
	     ++entryCloneHistoryItem, ++entryOrgHistoryItem)
	{
		QCOMPARE((*entryOrgHistoryItem)->timeInfo(), (*entryCloneHistoryItem)->timeInfo());
	}

	Database db;
	auto *entryOrgClone = entryOrg->clone(Entry::CloneNoFlags);
	entryOrgClone->setGroup(db.rootGroup());

	Entry *entryCloneUserRef = entryOrgClone->clone(Entry::CloneUserAsRef);
	entryCloneUserRef->setGroup(db.rootGroup());
	QCOMPARE(entryCloneUserRef->uuid(), entryOrgClone->uuid());
	QCOMPARE(entryCloneUserRef->title(), QStringLiteral("New Title"));
	QCOMPARE(entryCloneUserRef->historyItems().size(), 0);
	QCOMPARE(entryCloneUserRef->timeInfo().creationTime(), entryOrgClone->timeInfo().creationTime());
	QVERIFY(entryCloneUserRef->attributes()->isReference(EntryAttributes::UserNameKey));
	QCOMPARE(entryCloneUserRef->resolvePlaceholder(entryCloneUserRef->username()), entryOrgClone->username());

	Entry *entryClonePassRef = entryOrgClone->clone(Entry::ClonePassAsRef);
	entryClonePassRef->setGroup(db.rootGroup());
	QCOMPARE(entryClonePassRef->uuid(), entryOrgClone->uuid());
	QCOMPARE(entryClonePassRef->title(), QStringLiteral("New Title"));
	QCOMPARE(entryClonePassRef->historyItems().size(), 0);
	QCOMPARE(entryClonePassRef->timeInfo().creationTime(), entryOrgClone->timeInfo().creationTime());
	QVERIFY(entryClonePassRef->attributes()->isReference(EntryAttributes::PasswordKey));
	QCOMPARE(entryClonePassRef->resolvePlaceholder(entryCloneUserRef->password()), entryOrg->password());
	QCOMPARE(entryClonePassRef->attributes()->referenceUuid(EntryAttributes::PasswordKey), entryOrgClone->uuid());
}

void TestEntry::testResolveUrl()
{
	QScopedPointer<Entry> entry(new Entry());
	QString testUrl = QStringLiteral("www.google.com");
	QString testCmd = QStringLiteral("cmd://firefox ") + testUrl;
	QString testFileUnix = QStringLiteral("/home/example/test.txt");
	QString testFileWindows = QStringLiteral("c:/WINDOWS/test.txt");
	QString testComplexCmd = QStringLiteral("cmd://firefox --start-now --url 'http://") + testUrl + QStringLiteral("' --quit");
	QString nonHttpUrl = QStringLiteral("ftp://google.com");
	QString noUrl = QStringLiteral("random text inserted here");

	// Test standard URL's
	QCOMPARE(entry->resolveUrl(QString()), QString());
	QCOMPARE(entry->resolveUrl(testUrl), QStringLiteral("https://") + testUrl);
	QCOMPARE(entry->resolveUrl(QStringLiteral("http://") + testUrl), QStringLiteral("http://") + testUrl);
	// Test file:// URL's
	QCOMPARE(entry->resolveUrl(QStringLiteral("file://") + testFileUnix), QStringLiteral("file://") + testFileUnix);
	QCOMPARE(entry->resolveUrl(testFileUnix), QStringLiteral("file://") + testFileUnix);
	QCOMPARE(entry->resolveUrl(QStringLiteral("file:///") + testFileWindows), QStringLiteral("file:///") + testFileWindows);
	QCOMPARE(entry->resolveUrl(testFileWindows), QStringLiteral("file:///") + testFileWindows);
	// Test cmd:// with no URL
	QCOMPARE(entry->resolveUrl(QStringLiteral("cmd://firefox")), QString());
	QCOMPARE(entry->resolveUrl(QStringLiteral("cmd://firefox --no-url")), QString());
	// Test cmd:// with URL's
	QCOMPARE(entry->resolveUrl(testCmd), QStringLiteral("https://") + testUrl);
	QCOMPARE(entry->resolveUrl(testComplexCmd), QStringLiteral("http://") + testUrl);
	// Test non-http URL
	QCOMPARE(entry->resolveUrl(nonHttpUrl), QString());
	// Test no URL
	QCOMPARE(entry->resolveUrl(noUrl), QString());
}

void TestEntry::testResolveUrlPlaceholders()
{
	Entry entry;
	entry.setUrl(QStringLiteral("https://user:pw@keepassxc.org:80/path/example.php?q=e&s=t+2#fragment"));

	QString rmvscm = QStringLiteral("//user:pw@keepassxc.org:80/path/example.php?q=e&s=t+2#fragment"); // Entry URL without scheme name.
	QString scm = QStringLiteral("https"); // Scheme name of the entry URL.
	QString host = QStringLiteral("keepassxc.org"); // Host component of the entry URL.
	QString port = QStringLiteral("80"); // Port number of the entry URL.
	QString path = QStringLiteral("/path/example.php"); // Path component of the entry URL.
	QString query = QStringLiteral("q=e&s=t+2"); // Query information of the entry URL.
	QString userinfo = QStringLiteral("user:pw"); // User information of the entry URL.
	QString username = QStringLiteral("user"); // User name of the entry URL.
	QString password = QStringLiteral("pw"); // Password of the entry URL.
	QString fragment = QStringLiteral("fragment"); // Fragment of the entry URL.

	QCOMPARE(entry.resolvePlaceholder(QStringLiteral("{URL:RMVSCM}")), rmvscm);
	QCOMPARE(entry.resolvePlaceholder(QStringLiteral("{URL:WITHOUTSCHEME}")), rmvscm);
	QCOMPARE(entry.resolvePlaceholder(QStringLiteral("{URL:SCM}")), scm);
	QCOMPARE(entry.resolvePlaceholder(QStringLiteral("{URL:SCHEME}")), scm);
	QCOMPARE(entry.resolvePlaceholder(QStringLiteral("{URL:HOST}")), host);
	QCOMPARE(entry.resolvePlaceholder(QStringLiteral("{URL:PORT}")), port);
	QCOMPARE(entry.resolvePlaceholder(QStringLiteral("{URL:PATH}")), path);
	QCOMPARE(entry.resolvePlaceholder(QStringLiteral("{URL:QUERY}")), query);
	QCOMPARE(entry.resolvePlaceholder(QStringLiteral("{URL:USERINFO}")), userinfo);
	QCOMPARE(entry.resolvePlaceholder(QStringLiteral("{URL:USERNAME}")), username);
	QCOMPARE(entry.resolvePlaceholder(QStringLiteral("{URL:PASSWORD}")), password);
	QCOMPARE(entry.resolvePlaceholder(QStringLiteral("{URL:FRAGMENT}")), fragment);
}

void TestEntry::testResolveRecursivePlaceholders()
{
	Database db;
	auto *root = db.rootGroup();

	auto *entry1 = new Entry();
	entry1->setGroup(root);
	entry1->setUuid(QUuid::createUuid());
	entry1->setTitle(QStringLiteral("{USERNAME}"));
	entry1->setUsername(QStringLiteral("{PASSWORD}"));
	entry1->setPassword(QStringLiteral("{URL}"));
	entry1->setUrl(QStringLiteral("{S:CustomTitle}"));
	entry1->attributes()->set(QStringLiteral("CustomTitle"), QStringLiteral("RecursiveValue"));
	QCOMPARE(entry1->resolveMultiplePlaceholders(entry1->title()), QStringLiteral("RecursiveValue"));

	auto *entry2 = new Entry();
	entry2->setGroup(root);
	entry2->setUuid(QUuid::createUuid());
	entry2->setTitle(QStringLiteral("Entry2Title"));
	entry2->setUsername(QStringLiteral("{S:CustomUserNameAttribute}"));
	entry2->setPassword(QStringLiteral("{REF:P@I:%1}").arg(entry1->uuidToHex()));
	entry2->setUrl(QStringLiteral("http://{S:IpAddress}:{S:Port}/{S:Uri}"));
	entry2->attributes()->set(QStringLiteral("CustomUserNameAttribute"), QStringLiteral("CustomUserNameValue"));
	entry2->attributes()->set(QStringLiteral("IpAddress"), QStringLiteral("127.0.0.1"));
	entry2->attributes()->set(QStringLiteral("Port"), QStringLiteral("1234"));
	entry2->attributes()->set(QStringLiteral("Uri"), QStringLiteral("uri/path"));

	auto *entry3 = new Entry();
	entry3->setGroup(root);
	entry3->setUuid(QUuid::createUuid());
	entry3->setTitle(QStringLiteral("{REF:T@I:%1}").arg(entry2->uuidToHex()));
	entry3->setUsername(QStringLiteral("{REF:U@I:%1}").arg(entry2->uuidToHex()));
	entry3->setPassword(QStringLiteral("{REF:P@I:%1}").arg(entry2->uuidToHex()));
	entry3->setUrl(QStringLiteral("{REF:A@I:%1}").arg(entry2->uuidToHex()));

	QCOMPARE(entry3->resolveMultiplePlaceholders(entry3->title()), QStringLiteral("Entry2Title"));
	QCOMPARE(entry3->resolveMultiplePlaceholders(entry3->username()), QStringLiteral("CustomUserNameValue"));
	QCOMPARE(entry3->resolveMultiplePlaceholders(entry3->password()), QStringLiteral("RecursiveValue"));
	QCOMPARE(entry3->resolveMultiplePlaceholders(entry3->url()), QStringLiteral("http://127.0.0.1:1234/uri/path"));

	auto *entry4 = new Entry();
	entry4->setGroup(root);
	entry4->setUuid(QUuid::createUuid());
	entry4->setTitle(QStringLiteral("{REF:T@I:%1}").arg(entry3->uuidToHex()));
	entry4->setUsername(QStringLiteral("{REF:U@I:%1}").arg(entry3->uuidToHex()));
	entry4->setPassword(QStringLiteral("{REF:P@I:%1}").arg(entry3->uuidToHex()));
	entry4->setUrl(QStringLiteral("{REF:A@I:%1}").arg(entry3->uuidToHex()));

	QCOMPARE(entry4->resolveMultiplePlaceholders(entry4->title()), QStringLiteral("Entry2Title"));
	QCOMPARE(entry4->resolveMultiplePlaceholders(entry4->username()), QStringLiteral("CustomUserNameValue"));
	QCOMPARE(entry4->resolveMultiplePlaceholders(entry4->password()), QStringLiteral("RecursiveValue"));
	QCOMPARE(entry4->resolveMultiplePlaceholders(entry4->url()), QStringLiteral("http://127.0.0.1:1234/uri/path"));

	auto *entry5 = new Entry();
	entry5->setGroup(root);
	entry5->setUuid(QUuid::createUuid());
	entry5->attributes()->set(QStringLiteral("Scheme"), QStringLiteral("http"));
	entry5->attributes()->set(QStringLiteral("Host"), QStringLiteral("host.org"));
	entry5->attributes()->set(QStringLiteral("Port"), QStringLiteral("2017"));
	entry5->attributes()->set(QStringLiteral("Path"), QStringLiteral("/some/path"));
	entry5->attributes()->set(QStringLiteral("UserName"), QStringLiteral("username"));
	entry5->attributes()->set(QStringLiteral("Password"), QStringLiteral("password"));
	entry5->attributes()->set(QStringLiteral("Query"), QStringLiteral("q=e&t=s"));
	entry5->attributes()->set(QStringLiteral("Fragment"), QStringLiteral("fragment"));
	entry5->setUrl(QStringLiteral("{S:Scheme}://{S:UserName}:{S:Password}@{S:Host}:{S:Port}{S:Path}?{S:Query}#{S:Fragment}"));
	entry5->setTitle(QStringLiteral("title+{URL:Path}+{URL:Fragment}+title"));

	const QString url = QStringLiteral("http://username:password@host.org:2017/some/path?q=e&t=s#fragment");
	QCOMPARE(entry5->resolveMultiplePlaceholders(entry5->url()), url);
	QCOMPARE(entry5->resolveMultiplePlaceholders(entry5->title()), QStringLiteral("title+/some/path+fragment+title"));

	auto *entry6 = new Entry();
	entry6->setGroup(root);
	entry6->setUuid(QUuid::createUuid());
	entry6->setTitle(QStringLiteral("{REF:T@I:%1}").arg(entry3->uuidToHex()));
	entry6->setUsername(QStringLiteral("{TITLE}"));
	entry6->setPassword(QStringLiteral("{PASSWORD}"));

	QCOMPARE(entry6->resolvePlaceholder(entry6->title()), QStringLiteral("Entry2Title"));
	QCOMPARE(entry6->resolvePlaceholder(entry6->username()), QStringLiteral("Entry2Title"));
	QCOMPARE(entry6->resolvePlaceholder(entry6->password()), QStringLiteral("{PASSWORD}"));

	auto *entry7 = new Entry();
	entry7->setGroup(root);
	entry7->setUuid(QUuid::createUuid());
	entry7->setTitle(QStringLiteral("{REF:T@I:%1} and something else").arg(entry3->uuidToHex()));
	entry7->setUsername(QStringLiteral("{TITLE}"));
	entry7->setPassword(QStringLiteral("PASSWORD"));
	entry7->setNotes(QStringLiteral("{lots} {of} {braces}"));

	QCOMPARE(entry7->resolvePlaceholder(entry7->title()), QStringLiteral("Entry2Title and something else"));
	QCOMPARE(entry7->resolvePlaceholder(entry7->username()), QStringLiteral("Entry2Title and something else"));
	QCOMPARE(entry7->resolvePlaceholder(entry7->password()), QStringLiteral("PASSWORD"));
	QCOMPARE(entry7->resolvePlaceholder(entry7->notes()), QStringLiteral("{lots} {of} {braces}"));
}

void TestEntry::testResolveReferencePlaceholders()
{
	Database db;
	auto *root = db.rootGroup();

	auto *entry1 = new Entry();
	entry1->setGroup(root);
	entry1->setUuid(QUuid::createUuid());
	entry1->setTitle(QStringLiteral("Title1"));
	entry1->setUsername(QStringLiteral("Username1"));
	entry1->setPassword(QStringLiteral("Password1"));
	entry1->setUrl(QStringLiteral("Url1"));
	entry1->setNotes(QStringLiteral("Notes1"));
	entry1->attributes()->set(QStringLiteral("CustomAttribute1"), QStringLiteral("CustomAttributeValue1"));

	auto *group = new Group();
	group->setParent(root);
	auto *entry2 = new Entry();
	entry2->setGroup(group);
	entry2->setUuid(QUuid::createUuid());
	entry2->setTitle(QStringLiteral("Title2"));
	entry2->setUsername(QStringLiteral("Username2"));
	entry2->setPassword(QStringLiteral("Password2"));
	entry2->setUrl(QStringLiteral("Url2"));
	entry2->setNotes(QStringLiteral("Notes2"));
	entry2->attributes()->set(QStringLiteral("CustomAttribute2"), QStringLiteral("CustomAttributeValue2"));

	auto *entry3 = new Entry();
	entry3->setGroup(group);
	entry3->setUuid(QUuid::createUuid());
	entry3->setTitle(QStringLiteral("{S:AttributeTitle}"));
	entry3->setUsername(QStringLiteral("{S:AttributeUsername}"));
	entry3->setPassword(QStringLiteral("{S:AttributePassword}"));
	entry3->setUrl(QStringLiteral("{S:AttributeUrl}"));
	entry3->setNotes(QStringLiteral("{S:AttributeNotes}"));
	entry3->attributes()->set(QStringLiteral("AttributeTitle"), QStringLiteral("TitleValue"));
	entry3->attributes()->set(QStringLiteral("AttributeUsername"), QStringLiteral("UsernameValue"));
	entry3->attributes()->set(QStringLiteral("AttributePassword"), QStringLiteral("PasswordValue"));
	entry3->attributes()->set(QStringLiteral("AttributeUrl"), QStringLiteral("UrlValue"));
	entry3->attributes()->set(QStringLiteral("AttributeNotes"), QStringLiteral("NotesValue"));

	auto *tstEntry = new Entry();
	tstEntry->setGroup(root);
	tstEntry->setUuid(QUuid::createUuid());

	QCOMPARE(tstEntry->resolveMultiplePlaceholders(QStringLiteral("{REF:T@I:%1}").arg(entry1->uuidToHex())), entry1->title());
	QCOMPARE(tstEntry->resolveMultiplePlaceholders(QStringLiteral("{REF:T@T:%1}").arg(entry1->title())), entry1->title());
	QCOMPARE(tstEntry->resolveMultiplePlaceholders(QStringLiteral("{REF:T@U:%1}").arg(entry1->username())), entry1->title());
	QCOMPARE(tstEntry->resolveMultiplePlaceholders(QStringLiteral("{REF:T@P:%1}").arg(entry1->password())), entry1->title());
	QCOMPARE(tstEntry->resolveMultiplePlaceholders(QStringLiteral("{REF:T@A:%1}").arg(entry1->url())), entry1->title());
	QCOMPARE(tstEntry->resolveMultiplePlaceholders(QStringLiteral("{REF:T@N:%1}").arg(entry1->notes())), entry1->title());
	QCOMPARE(tstEntry->resolveMultiplePlaceholders(QStringLiteral("{REF:T@O:%1}").arg(entry1->attributes()->value(QStringLiteral("CustomAttribute1")))),
	         entry1->title());

	QCOMPARE(tstEntry->resolveMultiplePlaceholders(QStringLiteral("{REF:T@I:%1}").arg(entry1->uuidToHex())), entry1->title());
	QCOMPARE(tstEntry->resolveMultiplePlaceholders(QStringLiteral("{REF:T@T:%1}").arg(entry1->title())), entry1->title());
	QCOMPARE(tstEntry->resolveMultiplePlaceholders(QStringLiteral("{REF:U@U:%1}").arg(entry1->username())),
	         entry1->username());
	QCOMPARE(tstEntry->resolveMultiplePlaceholders(QStringLiteral("{REF:P@P:%1}").arg(entry1->password())),
	         entry1->password());
	QCOMPARE(tstEntry->resolveMultiplePlaceholders(QStringLiteral("{REF:A@A:%1}").arg(entry1->url())), entry1->url());
	QCOMPARE(tstEntry->resolveMultiplePlaceholders(QStringLiteral("{REF:N@N:%1}").arg(entry1->notes())), entry1->notes());

	QCOMPARE(tstEntry->resolveMultiplePlaceholders(QStringLiteral("{REF:T@I:%1}").arg(entry2->uuidToHex())), entry2->title());
	QCOMPARE(tstEntry->resolveMultiplePlaceholders(QStringLiteral("{REF:T@T:%1}").arg(entry2->title())), entry2->title());
	QCOMPARE(tstEntry->resolveMultiplePlaceholders(QStringLiteral("{REF:T@U:%1}").arg(entry2->username())), entry2->title());
	QCOMPARE(tstEntry->resolveMultiplePlaceholders(QStringLiteral("{REF:T@P:%1}").arg(entry2->password())), entry2->title());
	QCOMPARE(tstEntry->resolveMultiplePlaceholders(QStringLiteral("{REF:T@A:%1}").arg(entry2->url())), entry2->title());
	QCOMPARE(tstEntry->resolveMultiplePlaceholders(QStringLiteral("{REF:T@N:%1}").arg(entry2->notes())), entry2->title());
	QCOMPARE(tstEntry->resolveMultiplePlaceholders(QStringLiteral("{REF:T@O:%1}").arg(entry2->attributes()->value(QStringLiteral("CustomAttribute2")))),
	         entry2->title());

	QCOMPARE(tstEntry->resolveMultiplePlaceholders(QStringLiteral("{REF:T@T:%1}").arg(entry2->title())), entry2->title());
	QCOMPARE(tstEntry->resolveMultiplePlaceholders(QStringLiteral("{REF:U@U:%1}").arg(entry2->username())),
	         entry2->username());
	QCOMPARE(tstEntry->resolveMultiplePlaceholders(QStringLiteral("{REF:P@P:%1}").arg(entry2->password())),
	         entry2->password());
	QCOMPARE(tstEntry->resolveMultiplePlaceholders(QStringLiteral("{REF:A@A:%1}").arg(entry2->url())), entry2->url());
	QCOMPARE(tstEntry->resolveMultiplePlaceholders(QStringLiteral("{REF:N@N:%1}").arg(entry2->notes())), entry2->notes());

	QCOMPARE(tstEntry->resolveMultiplePlaceholders(QStringLiteral("{REF:T@I:%1}").arg(entry3->uuidToHex())),
	         entry3->attributes()->value(QStringLiteral("AttributeTitle")));
	QCOMPARE(tstEntry->resolveMultiplePlaceholders(QStringLiteral("{REF:U@I:%1}").arg(entry3->uuidToHex())),
	         entry3->attributes()->value(QStringLiteral("AttributeUsername")));
	QCOMPARE(tstEntry->resolveMultiplePlaceholders(QStringLiteral("{REF:P@I:%1}").arg(entry3->uuidToHex())),
	         entry3->attributes()->value(QStringLiteral("AttributePassword")));
	QCOMPARE(tstEntry->resolveMultiplePlaceholders(QStringLiteral("{REF:A@I:%1}").arg(entry3->uuidToHex())),
	         entry3->attributes()->value(QStringLiteral("AttributeUrl")));
	QCOMPARE(tstEntry->resolveMultiplePlaceholders(QStringLiteral("{REF:N@I:%1}").arg(entry3->uuidToHex())),
	         entry3->attributes()->value(QStringLiteral("AttributeNotes")));

	QCOMPARE(tstEntry->resolveMultiplePlaceholders(QStringLiteral("{REF:T@I:%1}").arg(entry3->uuidToHex().toUpper())),
	         entry3->attributes()->value(QStringLiteral("AttributeTitle")));
	QCOMPARE(tstEntry->resolveMultiplePlaceholders(QStringLiteral("{REF:U@I:%1}").arg(entry3->uuidToHex().toUpper())),
	         entry3->attributes()->value(QStringLiteral("AttributeUsername")));
	QCOMPARE(tstEntry->resolveMultiplePlaceholders(QStringLiteral("{REF:P@I:%1}").arg(entry3->uuidToHex().toUpper())),
	         entry3->attributes()->value(QStringLiteral("AttributePassword")));
	QCOMPARE(tstEntry->resolveMultiplePlaceholders(QStringLiteral("{REF:A@I:%1}").arg(entry3->uuidToHex().toUpper())),
	         entry3->attributes()->value(QStringLiteral("AttributeUrl")));
	QCOMPARE(tstEntry->resolveMultiplePlaceholders(QStringLiteral("{REF:N@I:%1}").arg(entry3->uuidToHex().toUpper())),
	         entry3->attributes()->value(QStringLiteral("AttributeNotes")));

	QCOMPARE(tstEntry->resolveMultiplePlaceholders(QStringLiteral("{REF:t@i:%1}").arg(entry3->uuidToHex().toLower())),
	         entry3->attributes()->value(QStringLiteral("AttributeTitle")));
	QCOMPARE(tstEntry->resolveMultiplePlaceholders(QStringLiteral("{REF:u@i:%1}").arg(entry3->uuidToHex().toLower())),
	         entry3->attributes()->value(QStringLiteral("AttributeUsername")));
	QCOMPARE(tstEntry->resolveMultiplePlaceholders(QStringLiteral("{REF:p@i:%1}").arg(entry3->uuidToHex().toLower())),
	         entry3->attributes()->value(QStringLiteral("AttributePassword")));
	QCOMPARE(tstEntry->resolveMultiplePlaceholders(QStringLiteral("{REF:a@i:%1}").arg(entry3->uuidToHex().toLower())),
	         entry3->attributes()->value(QStringLiteral("AttributeUrl")));
	QCOMPARE(tstEntry->resolveMultiplePlaceholders(QStringLiteral("{REF:n@i:%1}").arg(entry3->uuidToHex().toLower())),
	         entry3->attributes()->value(QStringLiteral("AttributeNotes")));
}

void TestEntry::testResolveUuidPlaceholder()
{
	Database db;
	auto *root = db.rootGroup();

	auto *entry = new Entry();
	entry->setGroup(root);
	entry->setUuid(QUuid::createUuid());
	entry->setTitle(QStringLiteral("Test Entry"));
	entry->setUsername(QStringLiteral("TestUser"));
	entry->setPassword(QStringLiteral("TestPass"));
	entry->setNotes(QStringLiteral("Test with UUID: {UUID}"));

	// Test that {UUID} placeholder resolves to the entry's UUID
	QString expectedUuid = entry->uuidToHex();
	QString resolvedNotes = entry->resolveMultiplePlaceholders(entry->notes());
	QCOMPARE(resolvedNotes, QStringLiteral("Test with UUID: %1").arg(expectedUuid));

	// Test {UUID} placeholder directly
	QCOMPARE(entry->resolveMultiplePlaceholders(QStringLiteral("{UUID}")), expectedUuid);

	// Test case insensitivity
	QCOMPARE(entry->resolveMultiplePlaceholders(QStringLiteral("{uuid}")), expectedUuid);
	QCOMPARE(entry->resolveMultiplePlaceholders(QStringLiteral("{Uuid}")), expectedUuid);

	// Test mixed case in text
	QCOMPARE(entry->resolveMultiplePlaceholders(QStringLiteral("UUID is {UUID} here")), QStringLiteral("UUID is %1 here").arg(expectedUuid));

	// Test advanced attribute with {REF:U@I:{UUID}} - should resolve to the entry's own username
	entry->attributes()->set(QStringLiteral("SelfReference"), QStringLiteral("{REF:U@I:{UUID}}"));
	QString attributeValue = entry->attributes()->value(QStringLiteral("SelfReference"));
	QString resolvedSelfRef = entry->resolveMultiplePlaceholders(attributeValue);

	// Test the manual reference to confirm it works as before
	QString manualReference = QStringLiteral("{REF:U@I:%1}").arg(entry->uuidToHex());
	entry->attributes()->set(QStringLiteral("ManualReference"), manualReference);
	QString resolvedManualRef = entry->resolveMultiplePlaceholders(entry->attributes()->value(QStringLiteral("ManualReference")));

	// Test that both approaches work
	QCOMPARE(resolvedManualRef, entry->username());
	QCOMPARE(resolvedSelfRef, entry->username());
}

void TestEntry::testResolveNonIdPlaceholdersToUuid()
{
	Database db;
	auto *root = db.rootGroup();

	auto *referencedEntryTitle = new Entry();
	referencedEntryTitle->setGroup(root);
	referencedEntryTitle->setTitle(QStringLiteral("myTitle"));
	referencedEntryTitle->setUuid(QUuid::createUuid());

	auto *referencedEntryUsername = new Entry();
	referencedEntryUsername->setGroup(root);
	referencedEntryUsername->setUsername(QStringLiteral("myUser"));
	referencedEntryUsername->setUuid(QUuid::createUuid());

	auto *referencedEntryPassword = new Entry();
	referencedEntryPassword->setGroup(root);
	referencedEntryPassword->setPassword(QStringLiteral("myPassword"));
	referencedEntryPassword->setUuid(QUuid::createUuid());

	auto *referencedEntryUrl = new Entry();
	referencedEntryUrl->setGroup(root);
	referencedEntryUrl->setUrl(QStringLiteral("myUrl"));
	referencedEntryUrl->setUuid(QUuid::createUuid());

	auto *referencedEntryNotes = new Entry();
	referencedEntryNotes->setGroup(root);
	referencedEntryNotes->setNotes(QStringLiteral("myNotes"));
	referencedEntryNotes->setUuid(QUuid::createUuid());

	const QList<QChar> placeholders{QLatin1Char('T'), QLatin1Char('U'), QLatin1Char('P'), QLatin1Char('A'), QLatin1Char('N')};
	for (const QChar &searchIn: placeholders)
	{
		const Entry *referencedEntry = nullptr;
		QString newEntryNotesRaw = QStringLiteral("{REF:I@%1:%2}");

		switch (searchIn.toLatin1())
		{
		case 'T':
			referencedEntry = referencedEntryTitle;
			newEntryNotesRaw = newEntryNotesRaw.arg(searchIn, referencedEntry->title());
			break;
		case 'U':
			referencedEntry = referencedEntryUsername;
			newEntryNotesRaw = newEntryNotesRaw.arg(searchIn, referencedEntry->username());
			break;
		case 'P':
			referencedEntry = referencedEntryPassword;
			newEntryNotesRaw = newEntryNotesRaw.arg(searchIn, referencedEntry->password());
			break;
		case 'A':
			referencedEntry = referencedEntryUrl;
			newEntryNotesRaw = newEntryNotesRaw.arg(searchIn, referencedEntry->url());
			break;
		case 'N':
			referencedEntry = referencedEntryNotes;
			newEntryNotesRaw = newEntryNotesRaw.arg(searchIn, referencedEntry->notes());
			break;
		default:
			break;
		}

		auto *newEntry = new Entry();
		newEntry->setGroup(root);
		newEntry->setNotes(newEntryNotesRaw);

		const QString newEntryNotesResolved = newEntry->resolveMultiplePlaceholders(newEntry->notes());
		QCOMPARE(newEntryNotesResolved, referencedEntry->uuidToHex());
	}
}

void TestEntry::testResolveConversionPlaceholders()
{
	Database db;
	auto *root = db.rootGroup();

	auto *entry1 = new Entry();
	entry1->setGroup(root);
	entry1->setUuid(QUuid::createUuid());
	entry1->setTitle(QStringLiteral("Title1 {T-CONV:/{USERNAME}/lower/} {T-CONV:/{PASSWORD}/upper/}"));
	entry1->setUsername(QStringLiteral("Username1"));
	entry1->setPassword(QStringLiteral("Password1"));
	entry1->setUrl(QStringLiteral("https://example.com/?test=3423&h=sdsds"));

	auto *entry2 = new Entry();
	entry2->setGroup(root);
	entry2->setUuid(QUuid::createUuid());
	entry2->setTitle(QStringLiteral("Title2"));
	entry2->setUsername(QStringLiteral("{T-CONV:/{REF:U@I:%1}/UPPER/}").arg(entry1->uuidToHex()));
	entry2->setPassword(QStringLiteral("{REF:P@I:%1}").arg(entry1->uuidToHex()));
	entry2->setUrl(QStringLiteral("cmd://ssh {USERNAME}@server.com -p {PASSWORD}"));

	// Test complicated and nested conversions
	QCOMPARE(entry1->resolveMultiplePlaceholders(entry1->title()), QStringLiteral("Title1 username1 PASSWORD1"));
	QCOMPARE(entry2->resolveMultiplePlaceholders(entry2->url()), QStringLiteral("cmd://ssh USERNAME1@server.com -p Password1"));
	// Test base64 and hex conversions
	QCOMPARE(entry1->resolveMultiplePlaceholders(QStringLiteral("{T-CONV:/{PASSWORD}/base64/}")), QStringLiteral("UGFzc3dvcmQx"));
	QCOMPARE(entry1->resolveMultiplePlaceholders(QStringLiteral("{T-CONV:/{PASSWORD}/hex/}")), QStringLiteral("50617373776f726431"));
	// Test URL encode and decode
	auto encodedURL = entry1->resolveMultiplePlaceholders(QStringLiteral("{T-CONV:/{URL}/uri/}"));
	QCOMPARE(encodedURL, QStringLiteral("https%3A%2F%2Fexample.com%2F%3Ftest%3D3423%26h%3Dsdsds"));
	QCOMPARE(entry1->resolveMultiplePlaceholders(QStringLiteral("{T-CONV:/https%3A%2F%2Fexample.com%2F%3Ftest%3D3423%26h%3Dsdsds/uri-dec/}")),
	         entry1->url());
	// Test invalid syntax
	QString error;
	entry1->resolveConversionPlaceholder(QStringLiteral("{T-CONV:/{USERNAME}/junk/}"), &error);
	QVERIFY(!error.isEmpty());
	entry1->resolveConversionPlaceholder(QStringLiteral("{T-CONV:}"), &error);
	QVERIFY(!error.isEmpty());
	// Check that error gets cleared
	entry1->resolveConversionPlaceholder(QStringLiteral("{T-CONV:/a/upper/}"), &error);
	QVERIFY(error.isEmpty());
}

void TestEntry::testResolveReplacePlaceholders()
{
	Database db;
	auto *root = db.rootGroup();

	auto *entry1 = new Entry();
	entry1->setGroup(root);
	entry1->setUuid(QUuid::createUuid());
	entry1->setTitle(QStringLiteral("Title1"));
	entry1->setUsername(QStringLiteral("Username1"));
	entry1->setPassword(QStringLiteral("Password1"));

	auto *entry2 = new Entry();
	entry2->setGroup(root);
	entry2->setUuid(QUuid::createUuid());
	entry2->setTitle(QStringLiteral("SAP server1 12345"));
	entry2->setUsername(QStringLiteral("{T-REPLACE-RX:/{REF:U@I:%1}/\\d$/2/}").arg(entry1->uuidToHex()));
	entry2->setPassword(QStringLiteral("{REF:P@I:%1}").arg(entry1->uuidToHex()));
	entry2->setUrl(QStringLiteral(R"(cmd://sap.exe -system={T-REPLACE-RX:/{Title}/(?i)^(.* )?(\w+(?=(\s* \d+$)))\3/$2/} -client={T-REPLACE-RX:/{Title}/(?i)^.* (?=\d+$)//} -user={USERNAME} -pw={PASSWORD})"));

	// Test complicated and nested replacements
	QCOMPARE(entry2->resolveMultiplePlaceholders(entry2->url()),
	         QStringLiteral("cmd://sap.exe -system=server1 -client=12345 -user=Username2 -pw=Password1"));

	auto *entry3 = new Entry();
	entry3->setGroup(root);
	entry3->setUuid(QUuid::createUuid());
	entry3->setTitle(QStringLiteral("Entry 3"));
	entry3->setUsername(QStringLiteral("HMAC-SHA-256"));
	entry3->setUrl(QStringLiteral("{T-REPLACE-RX:!{USERNAME}!\\{USERNAME\\}!!}"));

	// Test escaped enclosures
	QCOMPARE(entry3->resolveMultiplePlaceholders(entry3->url()), entry3->username());

	// Test invalid syntax
	QString error;
	entry1->resolveRegexPlaceholder(QStringLiteral("{T-REPLACE-RX:/{USERNAME}/.*+?/test/}"), &error); // invalid regex
	QVERIFY(!error.isEmpty());
	entry1->resolveRegexPlaceholder(QStringLiteral("{T-REPLACE-RX:/{USERNAME}/.*/}"), &error); // no replacement
	QVERIFY(!error.isEmpty());
	// Check that error gets cleared
	entry1->resolveRegexPlaceholder(QStringLiteral("{T-REPLACE-RX:/{USERNAME}/\\d/2/}"), &error);
	QVERIFY(error.isEmpty());
}

void TestEntry::testResolveClonedEntry()
{
	Database db;
	auto *root = db.rootGroup();

	auto *original = new Entry();
	original->setGroup(root);
	original->setUuid(QUuid::createUuid());
	original->setTitle(QStringLiteral("Title"));
	original->setUsername(QStringLiteral("SomeUsername"));
	original->setPassword(QStringLiteral("SomePassword"));

	QCOMPARE(original->resolveMultiplePlaceholders(original->username()), original->username());
	QCOMPARE(original->resolveMultiplePlaceholders(original->password()), original->password());

	// Top-level clones.
	Entry *clone1 = original->clone(Entry::CloneNewUuid);
	clone1->setGroup(root);
	Entry *clone2 = original->clone(Entry::CloneUserAsRef | Entry::CloneNewUuid);
	clone2->setGroup(root);
	Entry *clone3 = original->clone(Entry::ClonePassAsRef | Entry::CloneNewUuid);
	clone3->setGroup(root);
	Entry *clone4 = original->clone(Entry::CloneUserAsRef | Entry::ClonePassAsRef | Entry::CloneNewUuid);
	clone4->setGroup(root);

	QCOMPARE(clone1->resolveMultiplePlaceholders(clone1->username()), original->username());
	QCOMPARE(clone1->resolveMultiplePlaceholders(clone1->password()), original->password());
	QCOMPARE(clone2->resolveMultiplePlaceholders(clone2->username()), original->username());
	QCOMPARE(clone2->resolveMultiplePlaceholders(clone2->password()), original->password());
	QCOMPARE(clone3->resolveMultiplePlaceholders(clone3->username()), original->username());
	QCOMPARE(clone3->resolveMultiplePlaceholders(clone3->password()), original->password());
	QCOMPARE(clone4->resolveMultiplePlaceholders(clone4->username()), original->username());
	QCOMPARE(clone4->resolveMultiplePlaceholders(clone4->password()), original->password());

	// Second-level clones.
	Entry *cclone1 = clone4->clone(Entry::CloneNewUuid);
	cclone1->setGroup(root);
	Entry *cclone2 = clone4->clone(Entry::CloneUserAsRef | Entry::CloneNewUuid);
	cclone2->setGroup(root);
	Entry *cclone3 = clone4->clone(Entry::ClonePassAsRef | Entry::CloneNewUuid);
	cclone3->setGroup(root);
	Entry *cclone4 = clone4->clone(Entry::CloneUserAsRef | Entry::ClonePassAsRef | Entry::CloneNewUuid);
	cclone4->setGroup(root);

	QCOMPARE(cclone1->resolveMultiplePlaceholders(cclone1->username()), original->username());
	QCOMPARE(cclone1->resolveMultiplePlaceholders(cclone1->password()), original->password());
	QCOMPARE(cclone2->resolveMultiplePlaceholders(cclone2->username()), original->username());
	QCOMPARE(cclone2->resolveMultiplePlaceholders(cclone2->password()), original->password());
	QCOMPARE(cclone3->resolveMultiplePlaceholders(cclone3->username()), original->username());
	QCOMPARE(cclone3->resolveMultiplePlaceholders(cclone3->password()), original->password());
	QCOMPARE(cclone4->resolveMultiplePlaceholders(cclone4->username()), original->username());
	QCOMPARE(cclone4->resolveMultiplePlaceholders(cclone4->password()), original->password());

	// Change the original's attributes and make sure that the changes are tracked.
	QString oldUsername = original->username();
	QString oldPassword = original->password();
	original->setUsername(QStringLiteral("DifferentUsername"));
	original->setPassword(QStringLiteral("DifferentPassword"));

	QCOMPARE(clone1->resolveMultiplePlaceholders(clone1->username()), oldUsername);
	QCOMPARE(clone1->resolveMultiplePlaceholders(clone1->password()), oldPassword);
	QCOMPARE(clone2->resolveMultiplePlaceholders(clone2->username()), original->username());
	QCOMPARE(clone2->resolveMultiplePlaceholders(clone2->password()), oldPassword);
	QCOMPARE(clone3->resolveMultiplePlaceholders(clone3->username()), oldUsername);
	QCOMPARE(clone3->resolveMultiplePlaceholders(clone3->password()), original->password());
	QCOMPARE(clone4->resolveMultiplePlaceholders(clone4->username()), original->username());
	QCOMPARE(clone4->resolveMultiplePlaceholders(clone4->password()), original->password());

	QCOMPARE(cclone1->resolveMultiplePlaceholders(cclone1->username()), original->username());
	QCOMPARE(cclone1->resolveMultiplePlaceholders(cclone1->password()), original->password());
	QCOMPARE(cclone2->resolveMultiplePlaceholders(cclone2->username()), original->username());
	QCOMPARE(cclone2->resolveMultiplePlaceholders(cclone2->password()), original->password());
	QCOMPARE(cclone3->resolveMultiplePlaceholders(cclone3->username()), original->username());
	QCOMPARE(cclone3->resolveMultiplePlaceholders(cclone3->password()), original->password());
	QCOMPARE(cclone4->resolveMultiplePlaceholders(cclone4->username()), original->username());
	QCOMPARE(cclone4->resolveMultiplePlaceholders(cclone4->password()), original->password());
}

void TestEntry::testIsRecycled()
{
	Entry *entry = new Entry();
	QVERIFY(!entry->isRecycled());

	Database db;
	Group *root = db.rootGroup();
	QVERIFY(root);
	entry->setGroup(root);
	QVERIFY(!entry->isRecycled());

	QVERIFY(db.metadata()->recycleBinEnabled());
	db.recycleEntry(entry);
	QVERIFY(entry->isRecycled());

	Group *group1 = new Group();
	group1->setParent(root);

	Entry *entry1 = new Entry();
	entry1->setGroup(group1);
	QVERIFY(!entry1->isRecycled());
	db.recycleGroup(group1);
	QVERIFY(entry1->isRecycled());
}

void TestEntry::testMoveUpDown()
{
	Database db;
	Group *root = db.rootGroup();
	QVERIFY(root);

	Entry *entry0 = new Entry();
	QVERIFY(entry0);
	entry0->setGroup(root);
	Entry *entry1 = new Entry();
	QVERIFY(entry1);
	entry1->setGroup(root);
	Entry *entry2 = new Entry();
	QVERIFY(entry2);
	entry2->setGroup(root);
	Entry *entry3 = new Entry();
	QVERIFY(entry3);
	entry3->setGroup(root);
	// default order, straight
	QCOMPARE(root->entries().at(0), entry0);
	QCOMPARE(root->entries().at(1), entry1);
	QCOMPARE(root->entries().at(2), entry2);
	QCOMPARE(root->entries().at(3), entry3);

	entry0->moveDown();
	QCOMPARE(root->entries().at(0), entry1);
	QCOMPARE(root->entries().at(1), entry0);
	QCOMPARE(root->entries().at(2), entry2);
	QCOMPARE(root->entries().at(3), entry3);

	entry0->moveDown();
	QCOMPARE(root->entries().at(0), entry1);
	QCOMPARE(root->entries().at(1), entry2);
	QCOMPARE(root->entries().at(2), entry0);
	QCOMPARE(root->entries().at(3), entry3);

	entry0->moveDown();
	QCOMPARE(root->entries().at(0), entry1);
	QCOMPARE(root->entries().at(1), entry2);
	QCOMPARE(root->entries().at(2), entry3);
	QCOMPARE(root->entries().at(3), entry0);

	// no effect
	entry0->moveDown();
	QCOMPARE(root->entries().at(0), entry1);
	QCOMPARE(root->entries().at(1), entry2);
	QCOMPARE(root->entries().at(2), entry3);
	QCOMPARE(root->entries().at(3), entry0);

	entry0->moveUp();
	QCOMPARE(root->entries().at(0), entry1);
	QCOMPARE(root->entries().at(1), entry2);
	QCOMPARE(root->entries().at(2), entry0);
	QCOMPARE(root->entries().at(3), entry3);

	entry0->moveUp();
	QCOMPARE(root->entries().at(0), entry1);
	QCOMPARE(root->entries().at(1), entry0);
	QCOMPARE(root->entries().at(2), entry2);
	QCOMPARE(root->entries().at(3), entry3);

	entry0->moveUp();
	QCOMPARE(root->entries().at(0), entry0);
	QCOMPARE(root->entries().at(1), entry1);
	QCOMPARE(root->entries().at(2), entry2);
	QCOMPARE(root->entries().at(3), entry3);

	// no effect
	entry0->moveUp();
	QCOMPARE(root->entries().at(0), entry0);
	QCOMPARE(root->entries().at(1), entry1);
	QCOMPARE(root->entries().at(2), entry2);
	QCOMPARE(root->entries().at(3), entry3);

	entry2->moveUp();
	QCOMPARE(root->entries().at(0), entry0);
	QCOMPARE(root->entries().at(1), entry2);
	QCOMPARE(root->entries().at(2), entry1);
	QCOMPARE(root->entries().at(3), entry3);

	entry0->moveDown();
	QCOMPARE(root->entries().at(0), entry2);
	QCOMPARE(root->entries().at(1), entry0);
	QCOMPARE(root->entries().at(2), entry1);
	QCOMPARE(root->entries().at(3), entry3);

	entry3->moveUp();
	QCOMPARE(root->entries().at(0), entry2);
	QCOMPARE(root->entries().at(1), entry0);
	QCOMPARE(root->entries().at(2), entry3);
	QCOMPARE(root->entries().at(3), entry1);

	entry3->moveUp();
	QCOMPARE(root->entries().at(0), entry2);
	QCOMPARE(root->entries().at(1), entry3);
	QCOMPARE(root->entries().at(2), entry0);
	QCOMPARE(root->entries().at(3), entry1);

	entry2->moveDown();
	QCOMPARE(root->entries().at(0), entry3);
	QCOMPARE(root->entries().at(1), entry2);
	QCOMPARE(root->entries().at(2), entry0);
	QCOMPARE(root->entries().at(3), entry1);

	entry1->moveUp();
	QCOMPARE(root->entries().at(0), entry3);
	QCOMPARE(root->entries().at(1), entry2);
	QCOMPARE(root->entries().at(2), entry1);
	QCOMPARE(root->entries().at(3), entry0);
}

void TestEntry::testPreviousParentGroup()
{
	Database db;
	auto *root = db.rootGroup();
	root->setUuid(QUuid::createUuid());
	QVERIFY(!root->uuid().isNull());

	auto *group1 = new Group();
	group1->setUuid(QUuid::createUuid());
	group1->setParent(root);
	QVERIFY(!group1->uuid().isNull());

	auto *group2 = new Group();
	group2->setParent(root);
	group2->setUuid(QUuid::createUuid());
	QVERIFY(!group2->uuid().isNull());

	auto *entry = new Entry();
	QVERIFY(entry);
	QVERIFY(entry->previousParentGroupUuid().isNull());
	QVERIFY(!entry->previousParentGroup());

	entry->setGroup(root);
	QVERIFY(entry->previousParentGroupUuid().isNull());
	QVERIFY(!entry->previousParentGroup());

	// Previous parent shouldn't be recorded if new and old parent are the same
	entry->setGroup(root);
	QVERIFY(entry->previousParentGroupUuid().isNull());
	QVERIFY(!entry->previousParentGroup());

	entry->setGroup(group1);
	QVERIFY(entry->previousParentGroupUuid() == root->uuid());
	QVERIFY(entry->previousParentGroup() == root);

	entry->setGroup(group2);
	QVERIFY(entry->previousParentGroupUuid() == group1->uuid());
	QVERIFY(entry->previousParentGroup() == group1);

	entry->setGroup(group2);
	QVERIFY(entry->previousParentGroupUuid() == group1->uuid());
	QVERIFY(entry->previousParentGroup() == group1);
}
