/*
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

#include "TestEntryModel.h"

#include <QSignalSpy>
#include <QTest>
#include <QAbstractItemModelTester>

#include "core/Entry.h"
#include "core/Group.h"
#include "crypto/Crypto.h"
#include "gui/DatabaseIcons.h"
#include "gui/IconModels.h"
#include "gui/SortFilterHideProxyModel.h"
#include "gui/entry/EntryAttachmentsModel.h"
#include "gui/entry/EntryAttributesModel.h"
#include "gui/entry/EntryModel.h"

QTEST_MAIN(TestEntryModel)

void TestEntryModel::initTestCase()
{
	qRegisterMetaType<QModelIndex>("QModelIndex");
	QVERIFY(Crypto::init());
	// EntryModel listens to config signals, and due to that instantiates config.
	// Use mock config for test.
	Config::createTempFileInstance();
}

void TestEntryModel::test()
{
	Group *group1 = new Group();
	Group *group2 = new Group();

	Entry *entry1 = new Entry();
	entry1->setGroup(group1);
	entry1->setTitle(QStringLiteral("testTitle1"));

	Entry *entry2 = new Entry();
	entry2->setGroup(group1);
	entry2->setTitle(QStringLiteral("testTitle2"));

	EntryModel *model = new EntryModel(this);

	QSignalSpy spyAboutToBeMoved(model, &EntryModel::rowsAboutToBeMoved);
	QSignalSpy spyMoved(model, &EntryModel::rowsMoved);

	QAbstractItemModelTester *modelTest = new QAbstractItemModelTester(model, this);

	model->setGroup(group1);

	QCOMPARE(model->rowCount(), 2);

	QSignalSpy spyDataChanged(model, &EntryModel::dataChanged);
	entry1->setTitle(QStringLiteral("changed"));
	QCOMPARE(spyDataChanged.count(), 1);

	QModelIndex index1 = model->index(0, 1);
	QModelIndex index2 = model->index(1, 1);

	QCOMPARE(model->data(index1).toString(), entry1->title());
	QCOMPARE(model->data(index2).toString(), entry2->title());

	QSignalSpy spyAboutToAdd(model, &EntryModel::rowsAboutToBeInserted);
	QSignalSpy spyAdded(model, &EntryModel::rowsInserted);
	QSignalSpy spyAboutToRemove(model, &EntryModel::rowsAboutToBeRemoved);
	QSignalSpy spyRemoved(model, &EntryModel::rowsRemoved);

	Entry *entry3 = new Entry();
	entry3->setGroup(group1);

	QCOMPARE(spyAboutToBeMoved.count(), 0);
	QCOMPARE(spyMoved.count(), 0);

	entry1->moveDown();
	QCOMPARE(spyAboutToBeMoved.count(), 1);
	QCOMPARE(spyMoved.count(), 1);

	entry1->moveDown();
	QCOMPARE(spyAboutToBeMoved.count(), 2);
	QCOMPARE(spyMoved.count(), 2);

	entry1->moveDown();
	QCOMPARE(spyAboutToBeMoved.count(), 2);
	QCOMPARE(spyMoved.count(), 2);

	entry3->moveUp();
	QCOMPARE(spyAboutToBeMoved.count(), 3);
	QCOMPARE(spyMoved.count(), 3);

	entry3->moveUp();
	QCOMPARE(spyAboutToBeMoved.count(), 3);
	QCOMPARE(spyMoved.count(), 3);

	QCOMPARE(spyAboutToAdd.count(), 1);
	QCOMPARE(spyAdded.count(), 1);
	QCOMPARE(spyAboutToRemove.count(), 0);
	QCOMPARE(spyRemoved.count(), 0);

	entry2->setGroup(group2);

	QCOMPARE(spyAboutToAdd.count(), 1);
	QCOMPARE(spyAdded.count(), 1);
	QCOMPARE(spyAboutToRemove.count(), 1);
	QCOMPARE(spyRemoved.count(), 1);

	QSignalSpy spyReset(model, &EntryModel::modelReset);
	model->setGroup(group2);
	QCOMPARE(spyReset.count(), 1);

	delete group1;
	delete group2;

	delete modelTest;
	delete model;
}

void TestEntryModel::testAttachmentsModel()
{
	EntryAttachments *entryAttachments = new EntryAttachments(this);

	EntryAttachmentsModel *model = new EntryAttachmentsModel(this);
	QAbstractItemModelTester *modelTest = new QAbstractItemModelTester(model, this);

	QCOMPARE(model->rowCount(), 0);
	model->setEntryAttachments(entryAttachments);
	QCOMPARE(model->rowCount(), 0);

	QSignalSpy spyDataChanged(model, &EntryModel::dataChanged);
	QSignalSpy spyAboutToAdd(model, &EntryModel::rowsAboutToBeInserted);
	QSignalSpy spyAdded(model, &EntryModel::rowsInserted);
	QSignalSpy spyAboutToRemove(model, &EntryModel::rowsAboutToBeRemoved);
	QSignalSpy spyRemoved(model, &EntryModel::rowsRemoved);

	entryAttachments->set(QStringLiteral("first"), QByteArray("123"));

	entryAttachments->set(QStringLiteral("2nd"), QByteArray("456"));
	entryAttachments->set(QStringLiteral("2nd"), QByteArray("7890"));

	const int firstRow = 0;
	QCOMPARE(model->data(model->index(firstRow, EntryAttachmentsModel::NameColumn)).toString(), QStringLiteral("2nd"));
	QCOMPARE(model->data(model->index(firstRow, EntryAttachmentsModel::SizeColumn), Qt::EditRole).toInt(), 4);

	entryAttachments->remove(QStringLiteral("first"));

	QCOMPARE(spyDataChanged.count(), 1);
	QCOMPARE(spyAboutToAdd.count(), 2);
	QCOMPARE(spyAdded.count(), 2);
	QCOMPARE(spyAboutToRemove.count(), 1);
	QCOMPARE(spyRemoved.count(), 1);

	QSignalSpy spyReset(model, &EntryModel::modelReset);
	entryAttachments->clear();
	model->setEntryAttachments(0);
	QCOMPARE(spyReset.count(), 2);
	QCOMPARE(model->rowCount(), 0);

	delete modelTest;
	delete model;
	delete entryAttachments;
}

void TestEntryModel::testAttributesModel()
{
	EntryAttributes *entryAttributes = new EntryAttributes(this);

	EntryAttributesModel *model = new EntryAttributesModel(this);
	QAbstractItemModelTester *modelTest = new QAbstractItemModelTester(model, this);

	QCOMPARE(model->rowCount(), 0);
	model->setEntryAttributes(entryAttributes);
	QCOMPARE(model->rowCount(), 0);

	QSignalSpy spyDataChanged(model, &EntryModel::dataChanged);
	QSignalSpy spyAboutToAdd(model, &EntryModel::rowsAboutToBeInserted);
	QSignalSpy spyAdded(model, &EntryModel::rowsInserted);
	QSignalSpy spyAboutToRemove(model, &EntryModel::rowsAboutToBeRemoved);
	QSignalSpy spyRemoved(model, &EntryModel::rowsRemoved);

	entryAttributes->set(QStringLiteral("first"), QStringLiteral("123"));

	entryAttributes->set(QStringLiteral("2nd"), QStringLiteral("456"));
	entryAttributes->set(QStringLiteral("2nd"), QStringLiteral("789"));

	QCOMPARE(model->data(model->index(0, 0)).toString(), QStringLiteral("2nd"));

	entryAttributes->remove(QStringLiteral("first"));

	// make sure these don't generate messages
	entryAttributes->set(QStringLiteral("Title"), QStringLiteral("test"));
	entryAttributes->set(QStringLiteral("UserName"), QStringLiteral("test"));
	entryAttributes->set(QStringLiteral("Password"), QStringLiteral("test"));
	entryAttributes->set(QStringLiteral("URL"), QStringLiteral("test"));
	entryAttributes->set(QStringLiteral("Notes"), QStringLiteral("test"));

	QCOMPARE(spyDataChanged.count(), 1);
	QCOMPARE(spyAboutToAdd.count(), 2);
	QCOMPARE(spyAdded.count(), 2);
	QCOMPARE(spyAboutToRemove.count(), 1);
	QCOMPARE(spyRemoved.count(), 1);

	// test attribute protection
	QString value = entryAttributes->value(QStringLiteral("2nd"));
	entryAttributes->set(QStringLiteral("2nd"), value, true);
	QVERIFY(entryAttributes->isProtected(QStringLiteral("2nd")));
	QCOMPARE(entryAttributes->value(QStringLiteral("2nd")), value);
	entryAttributes->clear();

	// test attribute sorting
	entryAttributes->set(QStringLiteral("Test1"), QStringLiteral("1"));
	entryAttributes->set(QStringLiteral("Test11"), QStringLiteral("11"));
	entryAttributes->set(QStringLiteral("Test2"), QStringLiteral("2"));
	QCOMPARE(model->rowCount(), 3);
	QCOMPARE(model->data(model->index(0, 0)).toString(), QStringLiteral("Test1"));
	QCOMPARE(model->data(model->index(1, 0)).toString(), QStringLiteral("Test2"));
	QCOMPARE(model->data(model->index(2, 0)).toString(), QStringLiteral("Test11"));

	QSignalSpy spyReset(model, &EntryModel::modelReset);
	entryAttributes->clear();
	model->setEntryAttributes(0);
	QCOMPARE(spyReset.count(), 2);
	QCOMPARE(model->rowCount(), 0);

	delete modelTest;
	delete model;
}

void TestEntryModel::testDefaultIconModel()
{
	DefaultIconModel *model = new DefaultIconModel(this);
	QAbstractItemModelTester *modelTest = new QAbstractItemModelTester(model, this);

	QCOMPARE(model->rowCount(), databaseIcons()->count());

	delete modelTest;
	delete model;
}

void TestEntryModel::testCustomIconModel()
{
	CustomIconModel *model = new CustomIconModel(this);
	QAbstractItemModelTester *modelTest = new QAbstractItemModelTester(model, this);

	QCOMPARE(model->rowCount(), 0);

	QHash<QUuid, QPixmap> icons;
	QList<QUuid> iconsOrder;

	QUuid iconUuid = QUuid::fromRfc4122(QByteArray(16, '2'));
	icons.insert(iconUuid, QPixmap());
	iconsOrder << iconUuid;

	QUuid iconUuid2 = QUuid::fromRfc4122(QByteArray(16, '1'));
	icons.insert(iconUuid2, QPixmap());
	iconsOrder << iconUuid2;

	model->setIcons(icons, iconsOrder);
	QCOMPARE(model->uuidFromIndex(model->index(0, 0)), iconUuid);
	QCOMPARE(model->uuidFromIndex(model->index(1, 0)), iconUuid2);

	delete modelTest;
	delete model;
}

void TestEntryModel::testProxyModel()
{
	EntryModel *modelSource = new EntryModel(this);
	SortFilterHideProxyModel *modelProxy = new SortFilterHideProxyModel(this);
	modelProxy->setSourceModel(modelSource);

	QAbstractItemModelTester *modelTest = new QAbstractItemModelTester(modelProxy, this);

	Database *db = new Database();
	Entry *entry = new Entry();
	entry->setTitle(QStringLiteral("Test Title"));
	entry->setGroup(db->rootGroup());

	modelSource->setGroup(db->rootGroup());

	// Test hiding and showing a column
	auto columnCount = modelProxy->columnCount();
	QSignalSpy spyColumnRemove(modelProxy, &EntryModel::columnsAboutToBeRemoved);
	modelProxy->hideColumn(0, true);
	QCOMPARE(modelProxy->columnCount(), columnCount - 1);
	QVERIFY(!spyColumnRemove.isEmpty());

	int oldSpyColumnRemoveSize = spyColumnRemove.size();
	modelProxy->hideColumn(0, true);
	QCOMPARE(spyColumnRemove.size(), oldSpyColumnRemoveSize);

	modelProxy->hideColumn(100, true);
	QCOMPARE(spyColumnRemove.size(), oldSpyColumnRemoveSize);

	QList<Entry*> entryList;
	entryList << entry;
	modelSource->setEntries(entryList);

	QSignalSpy spyColumnInsert(modelProxy, &EntryModel::columnsAboutToBeInserted);
	modelProxy->hideColumn(0, false);
	QCOMPARE(modelProxy->columnCount(), columnCount);
	QVERIFY(!spyColumnInsert.isEmpty());

	int oldSpyColumnInsertSize = spyColumnInsert.size();
	modelProxy->hideColumn(0, false);
	QCOMPARE(spyColumnInsert.size(), oldSpyColumnInsertSize);

	delete modelTest;
	delete modelProxy;
	delete modelSource;
	delete db;
}

void TestEntryModel::testDatabaseDelete()
{
	EntryModel *model = new EntryModel(this);
	QAbstractItemModelTester *modelTest = new QAbstractItemModelTester(model, this);

	Database *db1 = new Database();
	Group *group1 = new Group();
	group1->setParent(db1->rootGroup());

	Entry *entry1 = new Entry();
	entry1->setGroup(group1);

	Database *db2 = new Database();
	Entry *entry2 = new Entry();
	entry2->setGroup(db2->rootGroup());

	model->setEntries(QList<Entry*>() << entry1 << entry2);

	QCOMPARE(model->rowCount(), 2);

	delete db1;
	QCOMPARE(model->rowCount(), 1);

	delete entry2;
	QCOMPARE(model->rowCount(), 0);

	delete db2;
	delete modelTest;
	delete model;
}
