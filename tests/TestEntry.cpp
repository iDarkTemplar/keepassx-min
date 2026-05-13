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
