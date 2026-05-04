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

#include "SignalMultiplexer.h"

#include "core/Global.h"

#include "gui/MainWindow.h"
#include "gui/SearchWidget.h"

SignalMultiplexer::SignalMultiplexer()
	: QObject()
{
}

SignalMultiplexer::~SignalMultiplexer()
{
}

void SignalMultiplexer::setCurrentDatabaseWidget(DatabaseWidget *object)
{
	if (m_currentDatabaseWidget)
	{
		disconnect(this, nullptr, m_currentDatabaseWidget.get(), nullptr);
		disconnect(m_currentDatabaseWidget.get(), nullptr, this, nullptr);
		disconnect(this, nullptr, this, nullptr);
	}

	m_currentDatabaseWidget = object;

	if (m_currentDatabaseWidget)
	{
		connect(this, &SignalMultiplexer::search,                                  m_currentDatabaseWidget.get(), &DatabaseWidget::search);
		connect(this, &SignalMultiplexer::saveSearch,                              m_currentDatabaseWidget.get(), &DatabaseWidget::saveSearch);
		connect(this, &SignalMultiplexer::caseSensitiveChanged,                    m_currentDatabaseWidget.get(), &DatabaseWidget::setSearchCaseSensitive);
		connect(this, &SignalMultiplexer::limitGroupChanged,                       m_currentDatabaseWidget.get(), &DatabaseWidget::setSearchLimitGroup);
		connect(this, &SignalMultiplexer::downPressed,                             m_currentDatabaseWidget.get(), [this]() { if (m_currentDatabaseWidget) { m_currentDatabaseWidget->focusOnEntries(); }; });
		connect(this, &SignalMultiplexer::enterPressed,                            m_currentDatabaseWidget.get(), qOverload<>(&DatabaseWidget::switchToEntryEdit));
		connect(this, &SignalMultiplexer::copyAdditionalAttributeActionsTriggered, m_currentDatabaseWidget.get(), &DatabaseWidget::copyAttribute);
		connect(this, &SignalMultiplexer::setTagsMenuActionsTriggered,             m_currentDatabaseWidget.get(), &DatabaseWidget::setTag);
		connect(this, &SignalMultiplexer::actionEntryNewTriggered,                 m_currentDatabaseWidget.get(), &DatabaseWidget::createEntry);
		connect(this, &SignalMultiplexer::actionEntryEditTriggered,                m_currentDatabaseWidget.get(), qOverload<>(&DatabaseWidget::switchToEntryEdit));
		connect(this, &SignalMultiplexer::actionEntryExpireTriggered,              m_currentDatabaseWidget.get(), &DatabaseWidget::expireSelectedEntries);
		connect(this, &SignalMultiplexer::actionEntryCloneTriggered,               m_currentDatabaseWidget.get(), &DatabaseWidget::cloneEntry);
		connect(this, &SignalMultiplexer::actionEntryDeleteTriggered,              m_currentDatabaseWidget.get(), &DatabaseWidget::deleteSelectedEntries);
		connect(this, &SignalMultiplexer::actionEntryRestoreTriggered,             m_currentDatabaseWidget.get(), &DatabaseWidget::restoreSelectedEntries);
		connect(this, &SignalMultiplexer::actionEntryTotpTriggered,                m_currentDatabaseWidget.get(), &DatabaseWidget::showTotp);
		connect(this, &SignalMultiplexer::actionEntrySetupTotpTriggered,           m_currentDatabaseWidget.get(), &DatabaseWidget::setupTotp);
		connect(this, &SignalMultiplexer::actionEntryCopyTotpTriggered,            m_currentDatabaseWidget.get(), &DatabaseWidget::copyTotp);
		connect(this, &SignalMultiplexer::actionEntryCopyPasswordTotpTriggered,    m_currentDatabaseWidget.get(), &DatabaseWidget::copyPasswordTotp);
		connect(this, &SignalMultiplexer::actionEntryTotpQRCodeTriggered,          m_currentDatabaseWidget.get(), &DatabaseWidget::showTotpKeyQrCode);
		connect(this, &SignalMultiplexer::actionEntryCopyTitleTriggered,           m_currentDatabaseWidget.get(), &DatabaseWidget::copyTitle);
		connect(this, &SignalMultiplexer::actionEntryMoveUpTriggered,              m_currentDatabaseWidget.get(), &DatabaseWidget::moveEntryUp);
		connect(this, &SignalMultiplexer::actionEntryMoveDownTriggered,            m_currentDatabaseWidget.get(), &DatabaseWidget::moveEntryDown);
		connect(this, &SignalMultiplexer::actionEntryCopyUsernameTriggered,        m_currentDatabaseWidget.get(), &DatabaseWidget::copyUsername);
		connect(this, &SignalMultiplexer::actionEntryCopyPasswordTriggered,        m_currentDatabaseWidget.get(), &DatabaseWidget::copyPassword);
		connect(this, &SignalMultiplexer::actionEntryCopyURLTriggered,             m_currentDatabaseWidget.get(), &DatabaseWidget::copyURL);
		connect(this, &SignalMultiplexer::actionEntryCopyNotesTriggered,           m_currentDatabaseWidget.get(), &DatabaseWidget::copyNotes);
		connect(this, &SignalMultiplexer::actionGroupNewTriggered,                 m_currentDatabaseWidget.get(), &DatabaseWidget::createGroup);
		connect(this, &SignalMultiplexer::actionGroupEditTriggered,                m_currentDatabaseWidget.get(), qOverload<>(&DatabaseWidget::switchToGroupEdit));
		connect(this, &SignalMultiplexer::actionGroupCloneTriggered,               m_currentDatabaseWidget.get(), &DatabaseWidget::cloneGroup);
		connect(this, &SignalMultiplexer::actionGroupDeleteTriggered,              m_currentDatabaseWidget.get(), &DatabaseWidget::deleteGroup);
		connect(this, &SignalMultiplexer::actionGroupEmptyRecycleBinTriggered,     m_currentDatabaseWidget.get(), &DatabaseWidget::emptyRecycleBin);
		connect(this, &SignalMultiplexer::actionGroupSortAscTriggered,             m_currentDatabaseWidget.get(), &DatabaseWidget::sortGroupsAsc);
		connect(this, &SignalMultiplexer::actionGroupSortDescTriggered,            m_currentDatabaseWidget.get(), &DatabaseWidget::sortGroupsDesc);

		connect(m_currentDatabaseWidget.get(), &DatabaseWidget::currentModeChanged,        this, &SignalMultiplexer::databaseCurrentModeChanged);
		connect(m_currentDatabaseWidget.get(), &DatabaseWidget::groupChanged,              this, &SignalMultiplexer::databaseGroupChanged);
		connect(m_currentDatabaseWidget.get(), &DatabaseWidget::entrySelectionChanged,     this, &SignalMultiplexer::databaseEntrySelectionChanged);
		connect(m_currentDatabaseWidget.get(), &DatabaseWidget::databaseNonDataChanged,    this, &SignalMultiplexer::databaseNonDataChanged);
		connect(m_currentDatabaseWidget.get(), &DatabaseWidget::groupContextMenuRequested, this, &SignalMultiplexer::databaseGroupContextMenuRequested);
		connect(m_currentDatabaseWidget.get(), &DatabaseWidget::entryContextMenuRequested, this, &SignalMultiplexer::databaseEntryContextMenuRequested);
		connect(m_currentDatabaseWidget.get(), &DatabaseWidget::databaseUnlocked,          this, &SignalMultiplexer::databaseUnlocked);
		connect(m_currentDatabaseWidget.get(), &DatabaseWidget::databaseModified,          this, &SignalMultiplexer::databaseModified);
		connect(m_currentDatabaseWidget.get(), &DatabaseWidget::searchModeActivated,       this, &SignalMultiplexer::databaseSearchModeActivated);
		connect(m_currentDatabaseWidget.get(), &DatabaseWidget::listModeActivated,         this, &SignalMultiplexer::databaseListModeActivated);
		connect(m_currentDatabaseWidget.get(), &DatabaseWidget::updateSyncProgress,        this, &SignalMultiplexer::databaseUpdateSyncProgress);
		connect(m_currentDatabaseWidget.get(), &DatabaseWidget::requestSearch,             this, &SignalMultiplexer::databaseRequestSearch);
		connect(m_currentDatabaseWidget.get(), &DatabaseWidget::clearSearch,               this, &SignalMultiplexer::databaseClearSearch);
	}
}
