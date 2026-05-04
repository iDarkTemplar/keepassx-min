/*
 *  Copyright (C) 2026 i.Dark_Templar <darktemplar@dark-templar-archives.net>
 *  Copyright (C) 2012 Felix Geyer <debfx@fobos.de>
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

#ifndef KEEPASSX_SIGNALMULTIPLEXER_H
#define KEEPASSX_SIGNALMULTIPLEXER_H

#include <QObject>
#include <QPointer>

#include "gui/DatabaseWidget.h"

class SignalMultiplexer: public QObject
{
	Q_OBJECT

public:
	SignalMultiplexer();
	~SignalMultiplexer();

	void setCurrentDatabaseWidget(DatabaseWidget *object);

signals:
	// Direction: from object to database

	// SearchWidget
	void search(const QString &searchtext);
	void saveSearch(const QString &searchtext);
	void caseSensitiveChanged(bool state);
	void limitGroupChanged(bool state);
	void downPressed();
	void enterPressed();

	// action groups and actions
	void copyAdditionalAttributeActionsTriggered(QAction *action);
	void setTagsMenuActionsTriggered(QAction *action);
	void actionEntryNewTriggered();
	void actionEntryEditTriggered();
	void actionEntryExpireTriggered();
	void actionEntryCloneTriggered();
	void actionEntryDeleteTriggered();
	void actionEntryRestoreTriggered();
	void actionEntryTotpTriggered();
	void actionEntrySetupTotpTriggered();
	void actionEntryCopyTotpTriggered();
	void actionEntryCopyPasswordTotpTriggered();
	void actionEntryTotpQRCodeTriggered();
	void actionEntryCopyTitleTriggered();
	void actionEntryMoveUpTriggered();
	void actionEntryMoveDownTriggered();
	void actionEntryCopyUsernameTriggered();
	void actionEntryCopyPasswordTriggered();
	void actionEntryCopyURLTriggered();
	void actionEntryCopyNotesTriggered();
	void actionGroupNewTriggered();
	void actionGroupEditTriggered();
	void actionGroupCloneTriggered();
	void actionGroupDeleteTriggered();
	void actionGroupEmptyRecycleBinTriggered();
	void actionGroupSortAscTriggered();
	void actionGroupSortDescTriggered();

	// Direction: from database to object
	void databaseCurrentModeChanged(DatabaseWidget::Mode mode);
	void databaseGroupChanged();
	void databaseEntrySelectionChanged();
	void databaseNonDataChanged();
	void databaseGroupContextMenuRequested(const QPoint &globalPos);
	void databaseEntryContextMenuRequested(const QPoint &globalPos);
	void databaseUnlocked();
	void databaseModified();
	void databaseSearchModeActivated();
	void databaseListModeActivated();
	void databaseUpdateSyncProgress(int progress, const QString &message);
	void databaseRequestSearch(const QString &search);
	void databaseClearSearch();

private:
	QPointer<DatabaseWidget> m_currentDatabaseWidget;

	Q_DISABLE_COPY(SignalMultiplexer)
};

#endif // KEEPASSX_SIGNALMULTIPLEXER_H
