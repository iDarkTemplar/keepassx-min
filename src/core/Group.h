/*
 *  Copyright (C) 2025 KeePassXC Team <team@keepassxc.org>
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

#ifndef KEEPASSX_GROUP_H
#define KEEPASSX_GROUP_H

#include <QPointer>

#include "core/CustomData.h"
#include "core/Database.h"
#include "core/Entry.h"

class Group: public ModifiableObject
{
	Q_OBJECT

public:
	enum TriState
	{
		Inherit,
		Enable,
		Disable
	};

	enum CloneFlag
	{
		CloneNoFlags = 0,
		CloneNewUuid = 1, // generate a random uuid for the clone
		CloneResetTimeInfo = 2, // set all TimeInfo attributes to the current time
		CloneIncludeEntries = 4, // clone the group entries
		CloneDefault = CloneNewUuid | CloneResetTimeInfo | CloneIncludeEntries,
		CloneRenameTitle = 8, // add "- Clone" after the original title
	};

	Q_DECLARE_FLAGS(CloneFlags, CloneFlag)

	struct GroupData
	{
		QString name;
		QString notes;
		int iconNumber;
		QUuid customIcon;
		TimeInfo timeInfo;
		bool isExpanded;
		Group::TriState searchingEnabled;
		QString tags;
		QUuid previousParentGroupUuid;

		bool operator==(const GroupData &other) const;
		bool operator!=(const GroupData &other) const;
		bool equals(const GroupData &other, CompareItemOptions options) const;
	};

	Group();
	~Group();

	const QUuid& uuid() const;
	const QString uuidToHex() const;
	QString name() const;
	QString notes() const;
	QString tags() const;
	QString fullPath() const;
	int iconNumber() const;
	const QUuid& iconUuid() const;
	const TimeInfo& timeInfo() const;
	bool isExpanded() const;
	Group::TriState searchingEnabled() const;
	bool resolveSearchingEnabled() const;
	Entry* lastTopVisibleEntry() const;
	bool isExpired() const;
	bool isRecycled() const;
	bool isEmpty() const;
	CustomData* customData();
	const CustomData* customData() const;
	const Group* previousParentGroup() const;
	QUuid previousParentGroupUuid() const;

	bool equals(const Group *other, CompareItemOptions options) const;

	static const int DefaultIconNumber;
	static const int OpenFolderIconNumber;
	static const int RecycleBinIconNumber;

	Group* findChildByName(const QString &name);
	Entry* findEntryByUuid(const QUuid &uuid, bool recursive = true) const;
	Entry* findEntryByPath(const QString &entryPath) const;
	Group* findGroupByUuid(const QUuid &uuid);
	const Group* findGroupByUuid(const QUuid &uuid) const;
	Group* findGroupByPath(const QString &groupPath);
	Entry* addEntryWithPath(const QString &entryPath);
	void setUuid(const QUuid &uuid);
	void setName(const QString &name);
	void setNotes(const QString &notes);
	void setTags(const QString &tags);
	void setIcon(int iconNumber);
	void setIcon(const QUuid &uuid);
	void setTimeInfo(const TimeInfo &timeInfo);
	void setExpanded(bool expanded);
	void setSearchingEnabled(TriState enable);
	void setLastTopVisibleEntry(Entry *entry);
	void setExpires(bool value);
	void setExpiryTime(const QDateTime &dateTime);
	void setPreviousParentGroup(const Group *group);
	void setPreviousParentGroupUuid(const QUuid &uuid);

	bool canUpdateTimeinfo() const;
	void setUpdateTimeinfo(bool value);

	Group* parentGroup();
	const Group* parentGroup() const;
	void setParent(Group *parent, int index = -1, bool trackPrevious = true);
	QStringList hierarchy(int height = -1) const;
	bool hasChildren() const;

	Database* database();
	const Database* database() const;
	QList<Group*> children();
	const QList<Group*> &children() const;
	QList<Entry*> entries();
	const QList<Entry*>& entries() const;
	QList<Entry*> entriesRecursive(bool includeHistoryItems = false) const;
	QList<const Group*> groupsRecursive(bool includeSelf) const;
	QList<Group*> groupsRecursive(bool includeSelf);
	QSet<QUuid> customIconsRecursive() const;
	QList<QString> usernamesRecursive(qsizetype topN = -1) const;

	Group* clone(
		Entry::CloneFlags entryFlags = Entry::CloneDefault,
		Group::CloneFlags groupFlags = Group::CloneDefault) const;

	void copyDataFrom(const Group *other);
	QString print(bool recursive = false, bool flatten = false, int depth = 0);

	void addEntry(Entry *entry);
	void removeEntry(Entry *entry);
	void moveEntryUp(Entry *entry);
	void moveEntryDown(Entry *entry);

	void applyGroupIconOnCreateTo(Entry *entry);
	void applyGroupIconTo(Entry *entry);
	void applyGroupIconTo(Group *other);
	void applyGroupIconToChildGroups();
	void applyGroupIconToChildEntries();

	void sortChildrenRecursively(bool reverse = false);

Q_SIGNALS:
	void groupDataChanged(Group *group);
	void groupAboutToAdd(Group *group, int index);
	void groupAdded();
	void groupAboutToRemove(Group *group);
	void groupRemoved();
	void aboutToMove(Group *group, Group *toGroup, int index);
	void groupMoved();
	void groupNonDataChange();
	void entryAboutToAdd(Entry *entry);
	void entryAdded(Entry *entry);
	void entryAboutToRemove(Entry *entry);
	void entryRemoved(Entry *entry);
	void entryAboutToMoveUp(int row);
	void entryMovedUp();
	void entryAboutToMoveDown(int row);
	void entryMovedDown();
	void entryDataChanged(Entry *entry);

private Q_SLOTS:
	void updateTimeinfo();

private:
	template <class P, class V>
	bool set(P &property, const V &value);

	void setParent(Database *db);

	void connectDatabaseSignalsRecursive(Database *db);
	void cleanupParent();
	void recCreateDelObjects();

	Entry* findEntryByPathRecursive(const QString &entryPath, const QString &basePath) const;
	Group* findGroupByPathRecursive(const QString &groupPath, const QString &basePath);

	QPointer<Database> m_db;
	QUuid m_uuid;
	GroupData m_data;
	QPointer<Entry> m_lastTopVisibleEntry;
	QList<Group*> m_children;
	QList<Entry*> m_entries;

	QPointer<CustomData> m_customData;

	QPointer<Group> m_parent;

	bool m_updateTimeinfo;

	friend Group* Database::setRootGroup(Group *group);
};

Q_DECLARE_OPERATORS_FOR_FLAGS(Group::CloneFlags)

#endif // KEEPASSX_GROUP_H
