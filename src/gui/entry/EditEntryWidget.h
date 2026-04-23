/*
 *  Copyright (C) 2010 Felix Geyer <debfx@fobos.de>
 *  Copyright (C) 2021 KeePassXC Team <team@keepassxc.org>
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

#ifndef KEEPASSX_EDITENTRYWIDGET_H
#define KEEPASSX_EDITENTRYWIDGET_H

#include <QButtonGroup>
#include <QCheckBox>
#include <QCompleter>
#include <QPointer>
#include <QTimer>

#include "config-keepassx.h"
#include "gui/EditWidget.h"

class AutoTypeAssociations;
class AutoTypeAssociationsModel;
class CustomData;
class Database;
class EditWidgetIcons;
class EditWidgetProperties;
class Entry;
class EntryAttributes;
class EntryAttachments;
class EntryAttributesModel;
class EntryHistoryModel;
class QButtonGroup;
class QMenu;
class QScrollArea;
class QSortFilterProxyModel;
class QStringListModel;

namespace Ui {

class EditEntryWidgetAdvanced;
class EditEntryWidgetAutoType;
class EditEntryWidgetBrowser;
class EditEntryWidgetSSHAgent;
class EditEntryWidgetMain;
class EditEntryWidgetHistory;
class EditWidget;

} // namespace Ui

class EditEntryWidget: public EditWidget
{
	Q_OBJECT

public:
	explicit EditEntryWidget(QWidget *parent = nullptr);
	~EditEntryWidget();

	void loadEntry(
		Entry *entry,
		bool create,
		bool history,
		const QString &parentName,
		QSharedPointer<Database> database);

	Entry* currentEntry() const;
	void clear();

	enum class Page
	{
		Main,
		Advanced,
		Icon,
		Properties,
		History
	};

	bool switchToPage(Page page);

signals:
	void editFinished(bool accepted);
	void historyEntryActivated(Entry *entry);

private slots:
	void acceptEntry();
	bool commitEntry();
	void cancel();
	void insertAttribute();
	void editCurrentAttribute();
	void removeCurrentAttribute();
	void updateCurrentAttribute();
	void protectCurrentAttribute(bool state);
	void toggleCurrentAttributeVisibility();
	void showHistoryEntry();
	void restoreHistoryEntry();
	void deleteHistoryEntry();
	void deleteAllHistoryEntries();
	void emitHistoryEntryActivated(const QModelIndex &index);
	void histEntryActivated(const QModelIndex &index);
	void updateHistoryButtons(const QModelIndex &current, const QModelIndex &previous);
	void useExpiryPreset(QAction *action);
	void toggleHideNotes(bool visible);
	void pickColor();

private:
	void setupMain();
	void setupAdvanced();
	void setupIcon();
	void setupProperties();
	void setupHistory();
	void setupEntryUpdate();
	void setupColorButton(bool foreground, const QColor &color);

	bool passwordsEqual();
	void setForms(Entry *entry, bool restore = false);
	QMenu* createPresetsMenu();
	void updateEntryData(Entry *entry) const;

	void displayAttribute(QModelIndex index, bool showProtected);

	QWidget* widgetForPage(Page page) const;

	QPointer<Entry> m_entry;
	QSharedPointer<Database> m_db;

	bool m_create;
	bool m_history;

	const QScopedPointer<Ui::EditEntryWidgetMain> m_mainUi;
	const QScopedPointer<Ui::EditEntryWidgetAdvanced> m_advancedUi;
	const QScopedPointer<Ui::EditEntryWidgetHistory> m_historyUi;
	const QScopedPointer<EntryAttachments> m_attachments;
	const QScopedPointer<CustomData> m_customData;

	QScrollArea *const m_mainWidget;
	QWidget *const m_advancedWidget;
	EditWidgetIcons *const m_iconsWidget;
	EditWidgetProperties *const m_editWidgetProperties;
	QWidget *const m_historyWidget;
	EntryAttributes *const m_entryAttributes;
	EntryAttributesModel *const m_attributesModel;
	EntryHistoryModel *const m_historyModel;
	QSortFilterProxyModel *const m_sortModel;
	QPersistentModelIndex m_currentAttribute;
	QCompleter *const m_usernameCompleter;
	QStringListModel *const m_usernameCompleterModel;
	QTimer m_entryModifiedTimer;

	Q_DISABLE_COPY(EditEntryWidget)
};

#endif // KEEPASSX_EDITENTRYWIDGET_H
