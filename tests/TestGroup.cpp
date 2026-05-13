/*
 *  Copyright (C) 2023 KeePassXC Team <team@keepassxc.org>
 *  Copyright (C) 2010 Felix Geyer <debfx@fobos.de>
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

#include "TestGroup.h"
#include "mock/MockClock.h"

#include <QSet>
#include <QSignalSpy>
#include <QtTestGui>

#include "core/Group.h"
#include "core/Metadata.h"
#include "crypto/Crypto.h"

QTEST_GUILESS_MAIN(TestGroup)

namespace
{
	MockClock *m_clock = nullptr;
}

void TestGroup::initTestCase()
{
	qRegisterMetaType<Entry*>("Entry*");
	qRegisterMetaType<Group*>("Group*");
	QVERIFY(Crypto::init());
}

void TestGroup::init()
{
	Q_ASSERT(m_clock == nullptr);
	m_clock = new MockClock(2010, 5, 5, 10, 30, 10);
	MockClock::setup(m_clock);
}

void TestGroup::cleanup()
{
	MockClock::teardown();
	m_clock = nullptr;
}

void TestGroup::testParenting()
{
	Database *db = new Database();
	QPointer<Group> rootGroup = db->rootGroup();
	Group *tmpRoot = new Group();

	QPointer<Group> g1 = new Group();
	QPointer<Group> g2 = new Group();
	QPointer<Group> g3 = new Group();
	QPointer<Group> g4 = new Group();

	g1->setParent(tmpRoot);
	g2->setParent(tmpRoot);
	g3->setParent(tmpRoot);
	g4->setParent(tmpRoot);

	g2->setParent(g1);
	g4->setParent(g3);
	g3->setParent(g1);
	g1->setParent(db->rootGroup());

	QVERIFY(g1->parent() == rootGroup);
	QVERIFY(g2->parent() == g1);
	QVERIFY(g3->parent() == g1);
	QVERIFY(g4->parent() == g3);

	QVERIFY(g1->database() == db);
	QVERIFY(g2->database() == db);
	QVERIFY(g3->database() == db);
	QVERIFY(g4->database() == db);

	QCOMPARE(tmpRoot->children().size(), 0);
	QCOMPARE(rootGroup->children().size(), 1);
	QCOMPARE(g1->children().size(), 2);
	QCOMPARE(g2->children().size(), 0);
	QCOMPARE(g3->children().size(), 1);
	QCOMPARE(g4->children().size(), 0);

	QVERIFY(rootGroup->children().at(0) == g1);
	QVERIFY(rootGroup->children().at(0) == g1);
	QVERIFY(g1->children().at(0) == g2);
	QVERIFY(g1->children().at(1) == g3);
	QVERIFY(g3->children().contains(g4));

	Group *g5 = new Group();
	Group *g6 = new Group();
	g5->setParent(db->rootGroup());
	g6->setParent(db->rootGroup());
	QVERIFY(db->rootGroup()->children().at(1) == g5);
	QVERIFY(db->rootGroup()->children().at(2) == g6);

	g5->setParent(db->rootGroup());
	QVERIFY(db->rootGroup()->children().at(1) == g6);
	QVERIFY(db->rootGroup()->children().at(2) == g5);

	QSignalSpy spy(db, &Database::groupDataChanged);
	g2->setName(QStringLiteral("test"));
	g4->setName(QStringLiteral("test"));
	g3->setName(QStringLiteral("test"));
	g1->setName(QStringLiteral("test"));
	g3->setIcon(QUuid::createUuid());
	g1->setIcon(2);
	QCOMPARE(spy.count(), 6);
	delete db;

	QVERIFY(rootGroup.isNull());
	QVERIFY(g1.isNull());
	QVERIFY(g2.isNull());
	QVERIFY(g3.isNull());
	QVERIFY(g4.isNull());
	delete tmpRoot;
}

void TestGroup::testSignals()
{
	Database *db = new Database();
	Database *db2 = new Database();
	QPointer<Group> root = db->rootGroup();

	QSignalSpy spyAboutToAdd(db, &Database::groupAboutToAdd);
	QSignalSpy spyAdded(db, &Database::groupAdded);
	QSignalSpy spyAboutToRemove(db, &Database::groupAboutToRemove);
	QSignalSpy spyRemoved(db, &Database::groupRemoved);
	QSignalSpy spyAboutToMove(db, &Database::groupAboutToMove);
	QSignalSpy spyMoved(db, &Database::groupMoved);

	QSignalSpy spyAboutToAdd2(db2, &Database::groupAboutToAdd);
	QSignalSpy spyAdded2(db2, &Database::groupAdded);
	QSignalSpy spyAboutToRemove2(db2, &Database::groupAboutToRemove);
	QSignalSpy spyRemoved2(db2, &Database::groupRemoved);
	QSignalSpy spyAboutToMove2(db2, &Database::groupAboutToMove);
	QSignalSpy spyMoved2(db2, &Database::groupMoved);

	Group *g1 = new Group();
	Group *g2 = new Group();

	g1->setParent(root);
	QCOMPARE(spyAboutToAdd.count(), 1);
	QCOMPARE(spyAdded.count(), 1);
	QCOMPARE(spyAboutToRemove.count(), 0);
	QCOMPARE(spyRemoved.count(), 0);
	QCOMPARE(spyAboutToMove.count(), 0);
	QCOMPARE(spyMoved.count(), 0);

	g2->setParent(root);
	QCOMPARE(spyAboutToAdd.count(), 2);
	QCOMPARE(spyAdded.count(), 2);
	QCOMPARE(spyAboutToRemove.count(), 0);
	QCOMPARE(spyRemoved.count(), 0);
	QCOMPARE(spyAboutToMove.count(), 0);
	QCOMPARE(spyMoved.count(), 0);

	g2->setParent(root);
	QCOMPARE(spyAboutToAdd.count(), 2);
	QCOMPARE(spyAdded.count(), 2);
	QCOMPARE(spyAboutToRemove.count(), 0);
	QCOMPARE(spyRemoved.count(), 0);
	QCOMPARE(spyAboutToMove.count(), 0);
	QCOMPARE(spyMoved.count(), 0);

	g2->setParent(root, 0);
	QCOMPARE(spyAboutToAdd.count(), 2);
	QCOMPARE(spyAdded.count(), 2);
	QCOMPARE(spyAboutToRemove.count(), 0);
	QCOMPARE(spyRemoved.count(), 0);
	QCOMPARE(spyAboutToMove.count(), 1);
	QCOMPARE(spyMoved.count(), 1);

	g1->setParent(g2);
	QCOMPARE(spyAboutToAdd.count(), 2);
	QCOMPARE(spyAdded.count(), 2);
	QCOMPARE(spyAboutToRemove.count(), 0);
	QCOMPARE(spyRemoved.count(), 0);
	QCOMPARE(spyAboutToMove.count(), 2);
	QCOMPARE(spyMoved.count(), 2);

	delete g1;
	QCOMPARE(spyAboutToAdd.count(), 2);
	QCOMPARE(spyAdded.count(), 2);
	QCOMPARE(spyAboutToRemove.count(), 1);
	QCOMPARE(spyRemoved.count(), 1);
	QCOMPARE(spyAboutToMove.count(), 2);
	QCOMPARE(spyMoved.count(), 2);

	g2->setParent(db2->rootGroup());
	QCOMPARE(spyAboutToAdd.count(), 2);
	QCOMPARE(spyAdded.count(), 2);
	QCOMPARE(spyAboutToRemove.count(), 2);
	QCOMPARE(spyRemoved.count(), 2);
	QCOMPARE(spyAboutToMove.count(), 2);
	QCOMPARE(spyMoved.count(), 2);
	QCOMPARE(spyAboutToAdd2.count(), 1);
	QCOMPARE(spyAdded2.count(), 1);
	QCOMPARE(spyAboutToRemove2.count(), 0);
	QCOMPARE(spyRemoved2.count(), 0);
	QCOMPARE(spyAboutToMove2.count(), 0);
	QCOMPARE(spyMoved2.count(), 0);

	Group *g3 = new Group();
	Group *g4 = new Group();

	g3->setParent(root);
	QCOMPARE(spyAboutToAdd.count(), 3);
	QCOMPARE(spyAdded.count(), 3);
	QCOMPARE(spyAboutToRemove.count(), 2);
	QCOMPARE(spyRemoved.count(), 2);
	QCOMPARE(spyAboutToMove.count(), 2);
	QCOMPARE(spyMoved.count(), 2);

	g4->setParent(root);
	QCOMPARE(spyAboutToAdd.count(), 4);
	QCOMPARE(spyAdded.count(), 4);
	QCOMPARE(spyAboutToRemove.count(), 2);
	QCOMPARE(spyRemoved.count(), 2);
	QCOMPARE(spyAboutToMove.count(), 2);
	QCOMPARE(spyMoved.count(), 2);

	g3->setParent(root);
	QCOMPARE(spyAboutToAdd.count(), 4);
	QCOMPARE(spyAdded.count(), 4);
	QCOMPARE(spyAboutToRemove.count(), 2);
	QCOMPARE(spyRemoved.count(), 2);
	QCOMPARE(spyAboutToMove.count(), 3);
	QCOMPARE(spyMoved.count(), 3);

	delete db;
	delete db2;

	QVERIFY(root.isNull());
}

void TestGroup::testEntries()
{
	Group *group = new Group();

	QPointer<Entry> entry1 = new Entry();
	entry1->setGroup(group);

	QPointer<Entry> entry2 = new Entry();
	entry2->setGroup(group);

	QCOMPARE(group->entries().size(), 2);
	QVERIFY(group->entries().at(0) == entry1);
	QVERIFY(group->entries().at(1) == entry2);

	delete group;

	QVERIFY(entry1.isNull());
	QVERIFY(entry2.isNull());
}

void TestGroup::testDeleteSignals()
{
	QScopedPointer<Database> db(new Database());
	Group *groupRoot = db->rootGroup();
	Group *groupChild = new Group();
	Group *groupChildChild = new Group();
	groupRoot->setObjectName("groupRoot");
	groupChild->setObjectName("groupChild");
	groupChildChild->setObjectName("groupChildChild");
	groupChild->setParent(groupRoot);
	groupChildChild->setParent(groupChild);
	QSignalSpy spyAboutToRemove(db.data(), &Database::groupAboutToRemove);
	QSignalSpy spyRemoved(db.data(), &Database::groupRemoved);

	delete groupChild;
	QVERIFY(groupRoot->children().isEmpty());
	QCOMPARE(spyAboutToRemove.count(), 2);
	QCOMPARE(spyRemoved.count(), 2);

	Group *group = new Group();
	Entry *entry = new Entry();
	entry->setGroup(group);
	QSignalSpy spyEntryAboutToRemove(group, &Group::entryAboutToRemove);
	QSignalSpy spyEntryRemoved(group, &Group::entryRemoved);

	delete entry;
	QVERIFY(group->entries().isEmpty());
	QCOMPARE(spyEntryAboutToRemove.count(), 1);
	QCOMPARE(spyEntryRemoved.count(), 1);
	delete group;

	QScopedPointer<Database> db2(new Database());
	Group *groupRoot2 = db2->rootGroup();
	Group *group2 = new Group();
	group2->setParent(groupRoot2);
	Entry *entry2 = new Entry();
	entry2->setGroup(group2);
	QSignalSpy spyEntryAboutToRemove2(group2, &Group::entryAboutToRemove);
	QSignalSpy spyEntryRemoved2(group2, &Group::entryRemoved);

	delete group2;
	QCOMPARE(spyEntryAboutToRemove2.count(), 1);
	QCOMPARE(spyEntryRemoved2.count(), 1);
}

void TestGroup::testCopyCustomIcon()
{
	QScopedPointer<Database> dbSource(new Database());

	QUuid groupIconUuid = QUuid::createUuid();
	QByteArray groupIcon("group icon");
	QString groupIconName(QStringLiteral("group icon"));
	dbSource->metadata()->addCustomIcon(groupIconUuid, groupIcon, groupIconName);

	QUuid entryIconUuid = QUuid::createUuid();
	QByteArray entryIcon("entry icon");
	QString entryIconName(QStringLiteral("entry icon"));
	dbSource->metadata()->addCustomIcon(entryIconUuid, entryIcon, entryIconName);

	auto *group = new Group();
	group->setParent(dbSource->rootGroup());
	group->setIcon(groupIconUuid);
	QCOMPARE(group->database()->metadata()->customIcon(groupIconUuid).data, groupIcon);
	QCOMPARE(group->database()->metadata()->customIcon(groupIconUuid).name, groupIconName);

	auto *entry = new Entry();
	entry->setGroup(dbSource->rootGroup());
	entry->setIcon(entryIconUuid);
	QCOMPARE(entry->database()->metadata()->customIcon(entryIconUuid).data, entryIcon);
	QCOMPARE(entry->database()->metadata()->customIcon(entryIconUuid).name, entryIconName);

	QScopedPointer<Database> dbTarget(new Database());

	group->setParent(dbTarget->rootGroup());
	QVERIFY(dbTarget->metadata()->hasCustomIcon(groupIconUuid));
	QCOMPARE(dbTarget->metadata()->customIcon(groupIconUuid).data, groupIcon);
	QCOMPARE(dbTarget->metadata()->customIcon(groupIconUuid).name, groupIconName);

	entry->setGroup(dbTarget->rootGroup());
	QVERIFY(dbTarget->metadata()->hasCustomIcon(entryIconUuid));
	QCOMPARE(dbTarget->metadata()->customIcon(entryIconUuid).data, entryIcon);
	QCOMPARE(dbTarget->metadata()->customIcon(entryIconUuid).name, entryIconName);
}

void TestGroup::testClone()
{
	QScopedPointer<Database> db(new Database());

	QScopedPointer<Group> originalGroup(new Group());
	originalGroup->setParent(db->rootGroup());
	originalGroup->setName(QStringLiteral("Group"));
	originalGroup->setIcon(42);

	QScopedPointer<Entry> originalGroupEntry(new Entry());
	originalGroupEntry->setGroup(originalGroup.data());
	originalGroupEntry->setTitle(QStringLiteral("GroupEntryOld"));
	originalGroupEntry->setIcon(43);
	originalGroupEntry->beginUpdate();
	originalGroupEntry->setTitle(QStringLiteral("GroupEntry"));
	originalGroupEntry->endUpdate();

	QScopedPointer<Group> subGroup(new Group());
	subGroup->setParent(originalGroup.data());
	subGroup->setName(QStringLiteral("SubGroup"));

	QScopedPointer<Entry> subGroupEntry(new Entry());
	subGroupEntry->setGroup(subGroup.data());
	subGroupEntry->setTitle(QStringLiteral("SubGroupEntry"));

	QScopedPointer<Group> clonedGroup(originalGroup->clone());
	QVERIFY(!clonedGroup->parentGroup());
	QVERIFY(!clonedGroup->database());
	QVERIFY(clonedGroup->uuid() != originalGroup->uuid());
	QCOMPARE(clonedGroup->name(), QStringLiteral("Group"));
	QCOMPARE(clonedGroup->iconNumber(), 42);
	QCOMPARE(clonedGroup->children().size(), 1);
	QCOMPARE(clonedGroup->entries().size(), 1);

	Entry *clonedGroupEntry = clonedGroup->entries().at(0);
	QVERIFY(clonedGroupEntry->uuid() != originalGroupEntry->uuid());
	QCOMPARE(clonedGroupEntry->title(), QStringLiteral("GroupEntry"));
	QCOMPARE(clonedGroupEntry->iconNumber(), 43);
	QCOMPARE(clonedGroupEntry->historyItems().size(), 0);

	Group *clonedSubGroup = clonedGroup->children().at(0);
	QVERIFY(clonedSubGroup->uuid() != subGroup->uuid());
	QCOMPARE(clonedSubGroup->name(), QStringLiteral("SubGroup"));
	QCOMPARE(clonedSubGroup->children().size(), 0);
	QCOMPARE(clonedSubGroup->entries().size(), 1);

	Entry *clonedSubGroupEntry = clonedSubGroup->entries().at(0);
	QVERIFY(clonedSubGroupEntry->uuid() != subGroupEntry->uuid());
	QCOMPARE(clonedSubGroupEntry->title(), QStringLiteral("SubGroupEntry"));

	QScopedPointer<Group> clonedGroupKeepUuid(originalGroup->clone(Entry::CloneNoFlags));
	QCOMPARE(clonedGroupKeepUuid->entries().at(0)->uuid(), originalGroupEntry->uuid());
	QCOMPARE(clonedGroupKeepUuid->children().at(0)->entries().at(0)->uuid(), subGroupEntry->uuid());

	QScopedPointer<Group> clonedGroupNoFlags(originalGroup->clone(Entry::CloneNoFlags, Group::CloneNoFlags));
	QCOMPARE(clonedGroupNoFlags->entries().size(), 0);
	QVERIFY(clonedGroupNoFlags->uuid() == originalGroup->uuid());

	QScopedPointer<Group> clonedGroupNewUuid(originalGroup->clone(Entry::CloneNoFlags, Group::CloneNewUuid));
	QCOMPARE(clonedGroupNewUuid->entries().size(), 0);
	QVERIFY(clonedGroupNewUuid->uuid() != originalGroup->uuid());

	// Making sure the new modification date is not the same.
	m_clock->advanceSecond(1);

	QScopedPointer<Group> clonedGroupResetTimeInfo(
		originalGroup->clone(Entry::CloneNoFlags, Group::CloneNewUuid | Group::CloneResetTimeInfo));
	QCOMPARE(clonedGroupResetTimeInfo->entries().size(), 0);
	QVERIFY(clonedGroupResetTimeInfo->uuid() != originalGroup->uuid());
	QVERIFY(clonedGroupResetTimeInfo->timeInfo().lastModificationTime()
	        != originalGroup->timeInfo().lastModificationTime());
}

void TestGroup::testCopyCustomIcons()
{
	QScopedPointer<Database> dbSource(new Database());
	QScopedPointer<Database> dbTarget(new Database());

	Metadata::CustomIconData icon1 = {QByteArray("icon 1"), QStringLiteral("icon 1"), Clock::currentDateTimeUtc()};
	Metadata::CustomIconData icon2 = {QByteArray("icon 2"), QStringLiteral("icon 2"), Clock::currentDateTimeUtc()};

	QScopedPointer<Group> group1(new Group());
	group1->setParent(dbSource->rootGroup());
	QUuid group1Icon = QUuid::createUuid();
	dbSource->metadata()->addCustomIcon(group1Icon, icon1);
	group1->setIcon(group1Icon);

	QScopedPointer<Group> group2(new Group());
	group2->setParent(group1.data());
	QUuid group2Icon = QUuid::createUuid();
	dbSource->metadata()->addCustomIcon(group2Icon, icon1);
	group2->setIcon(group2Icon);

	QScopedPointer<Entry> entry1(new Entry());
	entry1->setGroup(group2.data());
	QUuid entry1IconOld = QUuid::createUuid();
	dbSource->metadata()->addCustomIcon(entry1IconOld, icon1);
	entry1->setIcon(entry1IconOld);

	// add history item
	entry1->beginUpdate();
	QUuid entry1IconNew = QUuid::createUuid();
	dbSource->metadata()->addCustomIcon(entry1IconNew, icon1);
	entry1->setIcon(entry1IconNew);
	entry1->endUpdate();

	// test that we don't overwrite icons
	dbTarget->metadata()->addCustomIcon(group2Icon, icon1);

	dbTarget->metadata()->copyCustomIcons(group1->customIconsRecursive(), dbSource->metadata());

	Metadata *metaTarget = dbTarget->metadata();

	QCOMPARE(metaTarget->customIconsOrder().size(), 4);
	QVERIFY(metaTarget->hasCustomIcon(group1Icon));
	QVERIFY(metaTarget->hasCustomIcon(group2Icon));
	QVERIFY(metaTarget->hasCustomIcon(entry1IconOld));
	QVERIFY(metaTarget->hasCustomIcon(entry1IconNew));

	QCOMPARE(metaTarget->customIcon(group1Icon), icon1);
	QCOMPARE(metaTarget->customIcon(group2Icon), icon1);
}

void TestGroup::testFindEntry()
{
	QScopedPointer<Database> db(new Database());

	Entry *entry1 = new Entry();
	entry1->setTitle(QStringLiteral("entry1"));
	entry1->setGroup(db->rootGroup());
	entry1->setUuid(QUuid::createUuid());

	Group *group1 = new Group();
	group1->setName(QStringLiteral("group1"));

	Entry *entry2 = new Entry();

	entry2->setTitle(QStringLiteral("entry2"));
	entry2->setGroup(group1);
	entry2->setUuid(QUuid::createUuid());

	group1->setParent(db->rootGroup());

	Entry *entry;

	entry = db->rootGroup()->findEntryByUuid(entry1->uuid());
	QVERIFY(entry);
	QCOMPARE(entry->title(), QStringLiteral("entry1"));

	entry = db->rootGroup()->findEntryByPath(QStringLiteral("entry1"));
	QVERIFY(entry);
	QCOMPARE(entry->title(), QStringLiteral("entry1"));

	// We also can find the entry with the leading slash.
	entry = db->rootGroup()->findEntryByPath(QStringLiteral("/entry1"));
	QVERIFY(entry);
	QCOMPARE(entry->title(), QStringLiteral("entry1"));

	// But two slashes should not be accepted.
	entry = db->rootGroup()->findEntryByPath(QStringLiteral("//entry1"));
	QVERIFY(!entry);

	entry = db->rootGroup()->findEntryByUuid(entry2->uuid());
	QVERIFY(entry);
	QCOMPARE(entry->title(), QStringLiteral("entry2"));

	entry = db->rootGroup()->findEntryByPath(QStringLiteral("group1/entry2"));
	QVERIFY(entry);
	QCOMPARE(entry->title(), QStringLiteral("entry2"));

	entry = db->rootGroup()->findEntryByPath(QStringLiteral("/entry2"));
	QVERIFY(!entry);

	// We also can find the entry with the leading slash.
	entry = db->rootGroup()->findEntryByPath(QStringLiteral("/group1/entry2"));
	QVERIFY(entry);
	QCOMPARE(entry->title(), QStringLiteral("entry2"));

	// Should also find the entry only by title.
	entry = db->rootGroup()->findEntryByPath(QStringLiteral("entry2"));
	QVERIFY(entry);
	QCOMPARE(entry->title(), QStringLiteral("entry2"));

	entry = db->rootGroup()->findEntryByPath(QStringLiteral("invalid/path/to/entry2"));
	QVERIFY(!entry);

	entry = db->rootGroup()->findEntryByPath(QStringLiteral("entry27"));
	QVERIFY(!entry);

	// A valid UUID that does not exist in this database.
	entry = db->rootGroup()->findEntryByUuid(QUuid("febfb01ebcdf9dbd90a3f1579dc75281"));
	QVERIFY(!entry);

	// An invalid UUID.
	entry = db->rootGroup()->findEntryByUuid(QUuid("febfb01ebcdf9dbd90a3f1579dc"));
	QVERIFY(!entry);

	// Empty strings
	entry = db->rootGroup()->findEntryByUuid({});
	QVERIFY(!entry);

	entry = db->rootGroup()->findEntryByPath({});
	QVERIFY(!entry);
}

void TestGroup::testFindGroupByPath()
{
	QScopedPointer<Database> db(new Database());

	Group *group1 = new Group();
	group1->setName(QStringLiteral("group1"));
	group1->setParent(db->rootGroup());

	Group *group2 = new Group();
	group2->setName(QStringLiteral("group2"));
	group2->setParent(group1);

	Group *group;

	group = db->rootGroup()->findGroupByPath(QStringLiteral("/"));
	QVERIFY(group);
	QCOMPARE(group->uuid(), db->rootGroup()->uuid());

	// We also accept it if the leading slash is missing.
	group = db->rootGroup()->findGroupByPath(QString());
	QVERIFY(group);
	QCOMPARE(group->uuid(), db->rootGroup()->uuid());

	group = db->rootGroup()->findGroupByPath(QStringLiteral("/group1/"));
	QVERIFY(group);
	QCOMPARE(group->uuid(), group1->uuid());

	// We also accept it if the leading slash is missing.
	group = db->rootGroup()->findGroupByPath(QStringLiteral("group1/"));
	QVERIFY(group);
	QCOMPARE(group->uuid(), group1->uuid());

	// Too many slashes at the end
	group = db->rootGroup()->findGroupByPath(QStringLiteral("group1//"));
	QVERIFY(!group);

	// Missing a slash at the end.
	group = db->rootGroup()->findGroupByPath(QStringLiteral("/group1"));
	QVERIFY(group);
	QCOMPARE(group->uuid(), group1->uuid());

	// Too many slashes at the start
	group = db->rootGroup()->findGroupByPath(QStringLiteral("//group1"));
	QVERIFY(!group);

	group = db->rootGroup()->findGroupByPath(QStringLiteral("/group1/group2/"));
	QVERIFY(group);
	QCOMPARE(group->uuid(), group2->uuid());

	// We also accept it if the leading slash is missing.
	group = db->rootGroup()->findGroupByPath(QStringLiteral("group1/group2/"));
	QVERIFY(group);
	QCOMPARE(group->uuid(), group2->uuid());

	group = db->rootGroup()->findGroupByPath(QStringLiteral("group1/group2"));
	QVERIFY(group);
	QCOMPARE(group->uuid(), group2->uuid());

	group = db->rootGroup()->findGroupByPath(QStringLiteral("invalid"));
	QVERIFY(!group);
}

void TestGroup::testPrint()
{
	QScopedPointer<Database> db(new Database());

	QString output = db->rootGroup()->print();
	QCOMPARE(output, QStringLiteral("[empty]\n"));

	output = db->rootGroup()->print(true);
	QCOMPARE(output, QStringLiteral("[empty]\n"));

	Entry *entry1 = new Entry();
	entry1->setTitle(QStringLiteral("entry1"));
	entry1->setGroup(db->rootGroup());
	entry1->setUuid(QUuid::createUuid());

	output = db->rootGroup()->print();
	QCOMPARE(output, QStringLiteral("entry1\n"));

	Group *group1 = new Group();
	group1->setName(QStringLiteral("group1"));
	group1->setParent(db->rootGroup());

	Entry *entry2 = new Entry();
	entry2->setTitle(QStringLiteral("entry2"));
	entry2->setGroup(group1);
	entry2->setUuid(QUuid::createUuid());

	Group *group2 = new Group();
	group2->setName(QStringLiteral("group2"));
	group2->setParent(db->rootGroup());

	Group *subGroup = new Group();
	subGroup->setName(QStringLiteral("subgroup"));
	subGroup->setParent(group2);

	Entry *entry3 = new Entry();
	entry3->setTitle(QStringLiteral("entry3"));
	entry3->setGroup(subGroup);
	entry3->setUuid(QUuid::createUuid());

	output = db->rootGroup()->print();
	QVERIFY(output.contains(QStringLiteral("entry1\n")));
	QVERIFY(output.contains(QStringLiteral("group1/\n")));
	QVERIFY(!output.contains(QStringLiteral("  entry2\n")));
	QVERIFY(output.contains(QStringLiteral("group2/\n")));
	QVERIFY(!output.contains(QStringLiteral("  subgroup\n")));

	output = db->rootGroup()->print(true);
	QVERIFY(output.contains(QStringLiteral("entry1\n")));
	QVERIFY(output.contains(QStringLiteral("group1/\n")));
	QVERIFY(output.contains(QStringLiteral("  entry2\n")));
	QVERIFY(output.contains(QStringLiteral("group2/\n")));
	QVERIFY(output.contains(QStringLiteral("  subgroup/\n")));
	QVERIFY(output.contains(QStringLiteral("    entry3\n")));

	output = db->rootGroup()->print(true, true);
	QVERIFY(output.contains(QStringLiteral("entry1\n")));
	QVERIFY(output.contains(QStringLiteral("group1/\n")));
	QVERIFY(output.contains(QStringLiteral("group1/entry2\n")));
	QVERIFY(output.contains(QStringLiteral("group2/\n")));
	QVERIFY(output.contains(QStringLiteral("group2/subgroup/\n")));
	QVERIFY(output.contains(QStringLiteral("group2/subgroup/entry3\n")));

	output = group1->print();
	QVERIFY(!output.contains(QStringLiteral("group1/\n")));
	QVERIFY(output.contains(QStringLiteral("entry2\n")));

	output = group2->print(true, true);
	QVERIFY(!output.contains(QStringLiteral("group2/\n")));
	QVERIFY(output.contains(QStringLiteral("subgroup/\n")));
	QVERIFY(output.contains(QStringLiteral("subgroup/entry3\n")));
}

void TestGroup::testAddEntryWithPath()
{
	Database *db = new Database();

	Group *group1 = new Group();
	group1->setName(QStringLiteral("group1"));
	group1->setParent(db->rootGroup());

	Group *group2 = new Group();
	group2->setName(QStringLiteral("group2"));
	group2->setParent(group1);

	Entry *entry = db->rootGroup()->addEntryWithPath(QStringLiteral("entry1"));
	QVERIFY(entry);
	QVERIFY(!entry->uuid().isNull());

	entry = db->rootGroup()->addEntryWithPath(QStringLiteral("entry1"));
	QVERIFY(!entry);

	entry = db->rootGroup()->addEntryWithPath(QStringLiteral("/entry1"));
	QVERIFY(!entry);

	entry = db->rootGroup()->addEntryWithPath(QStringLiteral("entry2"));
	QVERIFY(entry);
	QVERIFY(entry->title() == QStringLiteral("entry2"));
	QVERIFY(!entry->uuid().isNull());

	entry = db->rootGroup()->addEntryWithPath(QStringLiteral("/entry3"));
	QVERIFY(entry);
	QVERIFY(entry->title() == QStringLiteral("entry3"));
	QVERIFY(!entry->uuid().isNull());

	entry = db->rootGroup()->addEntryWithPath(QStringLiteral("/group1/entry4"));
	QVERIFY(entry);
	QVERIFY(entry->title() == QStringLiteral("entry4"));
	QVERIFY(!entry->uuid().isNull());

	entry = db->rootGroup()->addEntryWithPath(QStringLiteral("/group1/group2/entry5"));
	QVERIFY(entry);
	QVERIFY(entry->title() == QStringLiteral("entry5"));
	QVERIFY(!entry->uuid().isNull());

	entry = db->rootGroup()->addEntryWithPath(QStringLiteral("/group1/invalid_group/entry6"));
	QVERIFY(!entry);

	delete db;
}

void TestGroup::testIsRecycled()
{
	Database db;
	db.metadata()->setRecycleBinEnabled(true);

	Group *group1 = new Group();
	group1->setName(QStringLiteral("group1"));
	group1->setParent(db.rootGroup());

	Group *group2 = new Group();
	group2->setName(QStringLiteral("group2"));
	group2->setParent(db.rootGroup());

	Group *group3 = new Group();
	group3->setName(QStringLiteral("group3"));
	group3->setParent(group2);

	Group *group4 = new Group();
	group4->setName(QStringLiteral("group4"));
	group4->setParent(db.rootGroup());

	db.recycleGroup(group2);

	QVERIFY(!group1->isRecycled());
	QVERIFY(group2->isRecycled());
	QVERIFY(group3->isRecycled());
	QVERIFY(!group4->isRecycled());

	db.recycleGroup(group4);
	QVERIFY(group4->isRecycled());
}

void TestGroup::testCopyDataFrom()
{
	QScopedPointer<Group> group(new Group());
	group->setName(QStringLiteral("TestGroup"));

	QScopedPointer<Group> group2(new Group());
	group2->setName(QStringLiteral("TestGroup2"));

	QScopedPointer<Group> group3(new Group());
	group3->setName(QStringLiteral("TestGroup3"));
	group3->customData()->set(QStringLiteral("testKey"), QStringLiteral("value"));

	QSignalSpy spyGroupModified(group.data(), &Group::modified);
	QSignalSpy spyGroupDataChanged(group.data(), &Group::groupDataChanged);

	group->copyDataFrom(group2.data());
	QCOMPARE(spyGroupModified.count(), 1);
	QCOMPARE(spyGroupDataChanged.count(), 1);

	// if no change, no signals
	spyGroupModified.clear();
	spyGroupDataChanged.clear();
	group->copyDataFrom(group2.data());
	QCOMPARE(spyGroupModified.count(), 0);
	QCOMPARE(spyGroupDataChanged.count(), 0);

	// custom data change triggers a separate modified signal
	spyGroupModified.clear();
	spyGroupDataChanged.clear();
	group->copyDataFrom(group3.data());
	QCOMPARE(spyGroupDataChanged.count(), 1);
	QCOMPARE(spyGroupModified.count(), 2);
}

void TestGroup::testEquals()
{
	QScopedPointer<Group> group(new Group());
	group->setName(QStringLiteral("TestGroup"));

	QVERIFY(group->equals(group.data(), CompareItemDefault));
}

void TestGroup::testChildrenSort()
{
	auto createTestGroupWithUnorderedChildren = []() -> Group * {
		Group *parent = new Group();

		Group *group1 = new Group();
		group1->setName(QStringLiteral("B"));
		group1->setParent(parent);
		Group *group2 = new Group();
		group2->setName(QStringLiteral("e"));
		group2->setParent(parent);
		Group *group3 = new Group();
		group3->setName(QStringLiteral("Test999"));
		group3->setParent(parent);
		Group *group4 = new Group();
		group4->setName(QStringLiteral("A"));
		group4->setParent(parent);
		Group *group5 = new Group();
		group5->setName(QStringLiteral("z"));
		group5->setParent(parent);
		Group *group6 = new Group();
		group6->setName(QStringLiteral("045"));
		group6->setParent(parent);
		Group *group7 = new Group();
		group7->setName(QStringLiteral("60"));
		group7->setParent(parent);
		Group *group8 = new Group();
		group8->setName(QStringLiteral("04test"));
		group8->setParent(parent);
		Group *group9 = new Group();
		group9->setName(QStringLiteral("Test12"));
		group9->setParent(parent);
		Group *group10 = new Group();
		group10->setName(QStringLiteral("i"));
		group10->setParent(parent);

		Group *subGroup1 = new Group();
		subGroup1->setName(QStringLiteral("sub_xte"));
		subGroup1->setParent(group10);
		Group *subGroup2 = new Group();
		subGroup2->setName(QStringLiteral("sub_010"));
		subGroup2->setParent(group10);
		Group *subGroup3 = new Group();
		subGroup3->setName(QStringLiteral("sub_000"));
		subGroup3->setParent(group10);
		Group *subGroup4 = new Group();
		subGroup4->setName(QStringLiteral("sub_M"));
		subGroup4->setParent(group10);
		Group *subGroup5 = new Group();
		subGroup5->setName(QStringLiteral("sub_p"));
		subGroup5->setParent(group10);
		Group *subGroup6 = new Group();
		subGroup6->setName(QStringLiteral("sub_45p"));
		subGroup6->setParent(group10);
		Group *subGroup7 = new Group();
		subGroup7->setName(QStringLiteral("sub_6p"));
		subGroup7->setParent(group10);
		Group *subGroup8 = new Group();
		subGroup8->setName(QStringLiteral("sub_tt"));
		subGroup8->setParent(group10);
		Group *subGroup9 = new Group();
		subGroup9->setName(QStringLiteral("sub_t0"));
		subGroup9->setParent(group10);

		return parent;
	};

	Group *parent = createTestGroupWithUnorderedChildren();
	Group *subParent = parent->children().last();
	parent->sortChildrenRecursively();
	QList<Group*> children = parent->children();
	QCOMPARE(children.size(), 10);
	QCOMPARE(children[0]->name(), QStringLiteral("045"));
	QCOMPARE(children[1]->name(), QStringLiteral("04test"));
	QCOMPARE(children[2]->name(), QStringLiteral("60"));
	QCOMPARE(children[3]->name(), QStringLiteral("A"));
	QCOMPARE(children[4]->name(), QStringLiteral("B"));
	QCOMPARE(children[5]->name(), QStringLiteral("e"));
	QCOMPARE(children[6]->name(), QStringLiteral("i"));
	QCOMPARE(children[7]->name(), QStringLiteral("Test12"));
	QCOMPARE(children[8]->name(), QStringLiteral("Test999"));
	QCOMPARE(children[9]->name(), QStringLiteral("z"));
	children = subParent->children();
	QCOMPARE(children.size(), 9);
	QCOMPARE(children[0]->name(), QStringLiteral("sub_000"));
	QCOMPARE(children[1]->name(), QStringLiteral("sub_010"));
	QCOMPARE(children[2]->name(), QStringLiteral("sub_45p"));
	QCOMPARE(children[3]->name(), QStringLiteral("sub_6p"));
	QCOMPARE(children[4]->name(), QStringLiteral("sub_M"));
	QCOMPARE(children[5]->name(), QStringLiteral("sub_p"));
	QCOMPARE(children[6]->name(), QStringLiteral("sub_t0"));
	QCOMPARE(children[7]->name(), QStringLiteral("sub_tt"));
	QCOMPARE(children[8]->name(), QStringLiteral("sub_xte"));
	delete parent;

	parent = createTestGroupWithUnorderedChildren();
	subParent = parent->children().last();
	parent->sortChildrenRecursively(true);
	children = parent->children();
	QCOMPARE(children.size(), 10);
	QCOMPARE(children[0]->name(), QStringLiteral("z"));
	QCOMPARE(children[1]->name(), QStringLiteral("Test999"));
	QCOMPARE(children[2]->name(), QStringLiteral("Test12"));
	QCOMPARE(children[3]->name(), QStringLiteral("i"));
	QCOMPARE(children[4]->name(), QStringLiteral("e"));
	QCOMPARE(children[5]->name(), QStringLiteral("B"));
	QCOMPARE(children[6]->name(), QStringLiteral("A"));
	QCOMPARE(children[7]->name(), QStringLiteral("60"));
	QCOMPARE(children[8]->name(), QStringLiteral("04test"));
	QCOMPARE(children[9]->name(), QStringLiteral("045"));
	children = subParent->children();
	QCOMPARE(children.size(), 9);
	QCOMPARE(children[0]->name(), QStringLiteral("sub_xte"));
	QCOMPARE(children[1]->name(), QStringLiteral("sub_tt"));
	QCOMPARE(children[2]->name(), QStringLiteral("sub_t0"));
	QCOMPARE(children[3]->name(), QStringLiteral("sub_p"));
	QCOMPARE(children[4]->name(), QStringLiteral("sub_M"));
	QCOMPARE(children[5]->name(), QStringLiteral("sub_6p"));
	QCOMPARE(children[6]->name(), QStringLiteral("sub_45p"));
	QCOMPARE(children[7]->name(), QStringLiteral("sub_010"));
	QCOMPARE(children[8]->name(), QStringLiteral("sub_000"));
	delete parent;

	parent = createTestGroupWithUnorderedChildren();
	subParent = parent->children().last();
	subParent->sortChildrenRecursively();
	children = parent->children();
	QCOMPARE(children.size(), 10);
	QCOMPARE(children[0]->name(), QStringLiteral("B"));
	QCOMPARE(children[1]->name(), QStringLiteral("e"));
	QCOMPARE(children[2]->name(), QStringLiteral("Test999"));
	QCOMPARE(children[3]->name(), QStringLiteral("A"));
	QCOMPARE(children[4]->name(), QStringLiteral("z"));
	QCOMPARE(children[5]->name(), QStringLiteral("045"));
	QCOMPARE(children[6]->name(), QStringLiteral("60"));
	QCOMPARE(children[7]->name(), QStringLiteral("04test"));
	QCOMPARE(children[8]->name(), QStringLiteral("Test12"));
	QCOMPARE(children[9]->name(), QStringLiteral("i"));
	children = subParent->children();
	QCOMPARE(children.size(), 9);
	QCOMPARE(children[0]->name(), QStringLiteral("sub_000"));
	QCOMPARE(children[1]->name(), QStringLiteral("sub_010"));
	QCOMPARE(children[2]->name(), QStringLiteral("sub_45p"));
	QCOMPARE(children[3]->name(), QStringLiteral("sub_6p"));
	QCOMPARE(children[4]->name(), QStringLiteral("sub_M"));
	QCOMPARE(children[5]->name(), QStringLiteral("sub_p"));
	QCOMPARE(children[6]->name(), QStringLiteral("sub_t0"));
	QCOMPARE(children[7]->name(), QStringLiteral("sub_tt"));
	QCOMPARE(children[8]->name(), QStringLiteral("sub_xte"));
	delete parent;

	parent = createTestGroupWithUnorderedChildren();
	subParent = parent->children().last();
	subParent->sortChildrenRecursively(true);
	children = parent->children();
	QCOMPARE(children.size(), 10);
	QCOMPARE(children[0]->name(), QStringLiteral("B"));
	QCOMPARE(children[1]->name(), QStringLiteral("e"));
	QCOMPARE(children[2]->name(), QStringLiteral("Test999"));
	QCOMPARE(children[3]->name(), QStringLiteral("A"));
	QCOMPARE(children[4]->name(), QStringLiteral("z"));
	QCOMPARE(children[5]->name(), QStringLiteral("045"));
	QCOMPARE(children[6]->name(), QStringLiteral("60"));
	QCOMPARE(children[7]->name(), QStringLiteral("04test"));
	QCOMPARE(children[8]->name(), QStringLiteral("Test12"));
	QCOMPARE(children[9]->name(), QStringLiteral("i"));
	children = subParent->children();
	QCOMPARE(children.size(), 9);
	QCOMPARE(children[0]->name(), QStringLiteral("sub_xte"));
	QCOMPARE(children[1]->name(), QStringLiteral("sub_tt"));
	QCOMPARE(children[2]->name(), QStringLiteral("sub_t0"));
	QCOMPARE(children[3]->name(), QStringLiteral("sub_p"));
	QCOMPARE(children[4]->name(), QStringLiteral("sub_M"));
	QCOMPARE(children[5]->name(), QStringLiteral("sub_6p"));
	QCOMPARE(children[6]->name(), QStringLiteral("sub_45p"));
	QCOMPARE(children[7]->name(), QStringLiteral("sub_010"));
	QCOMPARE(children[8]->name(), QStringLiteral("sub_000"));
	delete parent;
}

void TestGroup::testHierarchy()
{
	Group group1;
	group1.setName(QStringLiteral("group1"));

	Group *group2 = new Group();
	group2->setName(QStringLiteral("group2"));
	group2->setParent(&group1);

	Group *group3 = new Group();
	group3->setName(QStringLiteral("group3"));
	group3->setParent(group2);

	QStringList hierarchy = group3->hierarchy();
	QVERIFY(hierarchy.size() == 3);
	QVERIFY(hierarchy.contains(QStringLiteral("group1")));
	QVERIFY(hierarchy.contains(QStringLiteral("group2")));
	QVERIFY(hierarchy.contains(QStringLiteral("group3")));

	hierarchy = group3->hierarchy(0);
	QVERIFY(hierarchy.isEmpty());

	hierarchy = group3->hierarchy(1);
	QVERIFY(hierarchy.size() == 1);
	QVERIFY(hierarchy.contains(QStringLiteral("group3")));

	hierarchy = group3->hierarchy(2);
	QVERIFY(hierarchy.size() == 2);
	QVERIFY(hierarchy.contains(QStringLiteral("group2")));
	QVERIFY(hierarchy.contains(QStringLiteral("group3")));
}

void TestGroup::testApplyGroupIconRecursively()
{
	// Create a database with two nested groups with one entry each
	Database database;

	Group *subgroup = new Group();
	subgroup->setName(QStringLiteral("Subgroup"));
	subgroup->setParent(database.rootGroup());
	QVERIFY(subgroup);

	Group *subsubgroup = new Group();
	subsubgroup->setName(QStringLiteral("Subsubgroup"));
	subsubgroup->setParent(subgroup);
	QVERIFY(subsubgroup);

	Entry *subgroupEntry = subgroup->addEntryWithPath(QStringLiteral("Subgroup entry"));
	QVERIFY(subgroupEntry);
	subgroup->setIcon(1);

	Entry *subsubgroupEntry = subsubgroup->addEntryWithPath(QStringLiteral("Subsubgroup entry"));
	QVERIFY(subsubgroupEntry);
	subsubgroup->setIcon(2);

	// Set an icon per number to the root group and apply recursively
	// -> all groups and entries have the same icon
	const int rootIconNumber = 42;
	database.rootGroup()->setIcon(rootIconNumber);
	QVERIFY(database.rootGroup()->iconNumber() == rootIconNumber);
	database.rootGroup()->applyGroupIconToChildGroups();
	database.rootGroup()->applyGroupIconToChildEntries();
	QVERIFY(subgroup->iconNumber() == rootIconNumber);
	QVERIFY(subgroupEntry->iconNumber() == rootIconNumber);
	QVERIFY(subsubgroup->iconNumber() == rootIconNumber);
	QVERIFY(subsubgroupEntry->iconNumber() == rootIconNumber);

	// Set an icon per number to the subsubgroup and apply recursively
	// -> only the subsubgroup related groups and entries have updated icons
	const int subsubgroupIconNumber = 24;
	subsubgroup->setIcon(subsubgroupIconNumber);
	QVERIFY(subsubgroup->iconNumber() == subsubgroupIconNumber);
	subsubgroup->applyGroupIconToChildGroups();
	subsubgroup->applyGroupIconToChildEntries();
	QVERIFY(database.rootGroup()->iconNumber() == rootIconNumber);
	QVERIFY(subgroup->iconNumber() == rootIconNumber);
	QVERIFY(subgroupEntry->iconNumber() == rootIconNumber);
	QVERIFY(subsubgroup->iconNumber() == subsubgroupIconNumber);
	QVERIFY(subsubgroupEntry->iconNumber() == subsubgroupIconNumber);

	// Set an icon per UUID to the subgroup and apply recursively
	// -> all groups and entries except the root group have the same icon
	const QUuid subgroupIconUuid = QUuid::createUuid();
	QByteArray subgroupIcon("subgroup icon");

	database.metadata()->addCustomIcon(subgroupIconUuid, subgroupIcon);
	subgroup->setIcon(subgroupIconUuid);
	subgroup->applyGroupIconToChildGroups();
	subgroup->applyGroupIconToChildEntries();
	QVERIFY(database.rootGroup()->iconNumber() == rootIconNumber);
	QCOMPARE(subgroup->iconUuid(), subgroupIconUuid);
	QCOMPARE(subgroupEntry->iconUuid(), subgroupIconUuid);
	QCOMPARE(subsubgroup->iconUuid(), subgroupIconUuid);
	QCOMPARE(subsubgroupEntry->iconUuid(), subgroupIconUuid);
	QCOMPARE(subgroup->database()->metadata()->customIcon(subgroupIconUuid).data, subgroupIcon);

	// Reset all icons to root icon
	database.rootGroup()->setIcon(rootIconNumber);
	QVERIFY(database.rootGroup()->iconNumber() == rootIconNumber);
	database.rootGroup()->applyGroupIconToChildGroups();
	database.rootGroup()->applyGroupIconToChildEntries();
	QVERIFY(subgroup->iconNumber() == rootIconNumber);
	QVERIFY(subgroupEntry->iconNumber() == rootIconNumber);
	QVERIFY(subsubgroup->iconNumber() == rootIconNumber);
	QVERIFY(subsubgroupEntry->iconNumber() == rootIconNumber);

	// Apply only for child groups
	const int iconForGroups = 10;
	database.rootGroup()->setIcon(iconForGroups);
	QVERIFY(database.rootGroup()->iconNumber() == iconForGroups);
	database.rootGroup()->applyGroupIconToChildGroups();
	QVERIFY(database.rootGroup()->iconNumber() == iconForGroups);
	QVERIFY(subgroup->iconNumber() == iconForGroups);
	QVERIFY(subgroupEntry->iconNumber() == rootIconNumber);
	QVERIFY(subsubgroup->iconNumber() == iconForGroups);
	QVERIFY(subsubgroupEntry->iconNumber() == rootIconNumber);

	// Apply only for child entries
	const int iconForEntries = 20;
	database.rootGroup()->setIcon(iconForEntries);
	QVERIFY(database.rootGroup()->iconNumber() == iconForEntries);
	database.rootGroup()->applyGroupIconToChildEntries();
	QVERIFY(database.rootGroup()->iconNumber() == iconForEntries);
	QVERIFY(subgroup->iconNumber() == iconForGroups);
	QVERIFY(subgroupEntry->iconNumber() == iconForEntries);
	QVERIFY(subsubgroup->iconNumber() == iconForGroups);
	QVERIFY(subsubgroupEntry->iconNumber() == iconForEntries);
}

void TestGroup::testUsernamesRecursive()
{
	Database database;

	// Create a subgroup
	Group *subgroup = new Group();
	subgroup->setName(QStringLiteral("Subgroup"));
	subgroup->setParent(database.rootGroup());

	// Generate entries in the root group and the subgroup
	Entry *rootGroupEntry = database.rootGroup()->addEntryWithPath(QStringLiteral("Root group entry"));
	rootGroupEntry->setUsername(QStringLiteral("Name1"));

	Entry *subgroupEntry = subgroup->addEntryWithPath(QStringLiteral("Subgroup entry"));
	subgroupEntry->setUsername(QStringLiteral("Name2"));

	Entry *subgroupEntryReusingUsername = subgroup->addEntryWithPath(QStringLiteral("Another subgroup entry"));
	subgroupEntryReusingUsername->setUsername(QStringLiteral("Name2"));

	QList<QString> usernames = database.rootGroup()->usernamesRecursive();
	QCOMPARE(usernames.size(), 2);
	QVERIFY(usernames.contains(QStringLiteral("Name1")));
	QVERIFY(usernames.contains(QStringLiteral("Name2")));
	QVERIFY(usernames.indexOf(QStringLiteral("Name2")) < usernames.indexOf(QStringLiteral("Name1")));
}

void TestGroup::testMoveUpDown()
{
	Database database;
	Group *root = database.rootGroup();
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

	root->moveEntryDown(entry0);
	QCOMPARE(root->entries().at(0), entry1);
	QCOMPARE(root->entries().at(1), entry0);
	QCOMPARE(root->entries().at(2), entry2);
	QCOMPARE(root->entries().at(3), entry3);

	root->moveEntryDown(entry0);
	QCOMPARE(root->entries().at(0), entry1);
	QCOMPARE(root->entries().at(1), entry2);
	QCOMPARE(root->entries().at(2), entry0);
	QCOMPARE(root->entries().at(3), entry3);

	root->moveEntryDown(entry0);
	QCOMPARE(root->entries().at(0), entry1);
	QCOMPARE(root->entries().at(1), entry2);
	QCOMPARE(root->entries().at(2), entry3);
	QCOMPARE(root->entries().at(3), entry0);

	// no effect
	root->moveEntryDown(entry0);
	QCOMPARE(root->entries().at(0), entry1);
	QCOMPARE(root->entries().at(1), entry2);
	QCOMPARE(root->entries().at(2), entry3);
	QCOMPARE(root->entries().at(3), entry0);

	root->moveEntryUp(entry0);
	QCOMPARE(root->entries().at(0), entry1);
	QCOMPARE(root->entries().at(1), entry2);
	QCOMPARE(root->entries().at(2), entry0);
	QCOMPARE(root->entries().at(3), entry3);

	root->moveEntryUp(entry0);
	QCOMPARE(root->entries().at(0), entry1);
	QCOMPARE(root->entries().at(1), entry0);
	QCOMPARE(root->entries().at(2), entry2);
	QCOMPARE(root->entries().at(3), entry3);

	root->moveEntryUp(entry0);
	QCOMPARE(root->entries().at(0), entry0);
	QCOMPARE(root->entries().at(1), entry1);
	QCOMPARE(root->entries().at(2), entry2);
	QCOMPARE(root->entries().at(3), entry3);

	// no effect
	root->moveEntryUp(entry0);
	QCOMPARE(root->entries().at(0), entry0);
	QCOMPARE(root->entries().at(1), entry1);
	QCOMPARE(root->entries().at(2), entry2);
	QCOMPARE(root->entries().at(3), entry3);

	root->moveEntryUp(entry2);
	QCOMPARE(root->entries().at(0), entry0);
	QCOMPARE(root->entries().at(1), entry2);
	QCOMPARE(root->entries().at(2), entry1);
	QCOMPARE(root->entries().at(3), entry3);

	root->moveEntryDown(entry0);
	QCOMPARE(root->entries().at(0), entry2);
	QCOMPARE(root->entries().at(1), entry0);
	QCOMPARE(root->entries().at(2), entry1);
	QCOMPARE(root->entries().at(3), entry3);

	root->moveEntryUp(entry3);
	QCOMPARE(root->entries().at(0), entry2);
	QCOMPARE(root->entries().at(1), entry0);
	QCOMPARE(root->entries().at(2), entry3);
	QCOMPARE(root->entries().at(3), entry1);

	root->moveEntryUp(entry3);
	QCOMPARE(root->entries().at(0), entry2);
	QCOMPARE(root->entries().at(1), entry3);
	QCOMPARE(root->entries().at(2), entry0);
	QCOMPARE(root->entries().at(3), entry1);

	root->moveEntryDown(entry2);
	QCOMPARE(root->entries().at(0), entry3);
	QCOMPARE(root->entries().at(1), entry2);
	QCOMPARE(root->entries().at(2), entry0);
	QCOMPARE(root->entries().at(3), entry1);

	root->moveEntryUp(entry1);
	QCOMPARE(root->entries().at(0), entry3);
	QCOMPARE(root->entries().at(1), entry2);
	QCOMPARE(root->entries().at(2), entry1);
	QCOMPARE(root->entries().at(3), entry0);
}

void TestGroup::testPreviousParentGroup()
{
	Database db;
	auto *root = db.rootGroup();
	root->setUuid(QUuid::createUuid());
	QVERIFY(!root->uuid().isNull());
	QVERIFY(!root->previousParentGroup());
	QVERIFY(root->previousParentGroupUuid().isNull());

	auto *group1 = new Group();
	group1->setUuid(QUuid::createUuid());
	group1->setParent(root);
	QVERIFY(!group1->uuid().isNull());
	QVERIFY(!group1->previousParentGroup());
	QVERIFY(group1->previousParentGroupUuid().isNull());

	auto *group2 = new Group();
	group2->setParent(root);
	group2->setUuid(QUuid::createUuid());
	QVERIFY(!group2->uuid().isNull());
	QVERIFY(!group2->previousParentGroup());
	QVERIFY(group2->previousParentGroupUuid().isNull());

	group1->setParent(group2);
	QVERIFY(group1->previousParentGroupUuid() == root->uuid());
	QVERIFY(group1->previousParentGroup() == root);

	// Previous parent shouldn't be recorded if new and old parent are the same
	group1->setParent(group2);
	QVERIFY(group1->previousParentGroupUuid() == root->uuid());
	QVERIFY(group1->previousParentGroup() == root);

	group1->setParent(root);
	QVERIFY(group1->previousParentGroupUuid() == group2->uuid());
	QVERIFY(group1->previousParentGroup() == group2);
}
