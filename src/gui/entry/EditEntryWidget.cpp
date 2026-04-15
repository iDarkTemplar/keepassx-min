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

#include "EditEntryWidget.h"
#include "ui_EditEntryWidgetAdvanced.h"
#include "ui_EditEntryWidgetHistory.h"
#include "ui_EditEntryWidgetMain.h"

#include <QColorDialog>
#include <QDesktopServices>
#include <QSortFilterProxyModel>
#include <QStringListModel>

#include "core/Clock.h"
#include "core/Config.h"
#include "core/Database.h"
#include "core/Entry.h"
#include "core/EntryAttributes.h"
#include "core/Group.h"
#include "core/Metadata.h"
#include "core/PasswordGenerator.h"
#include "core/TimeDelta.h"
#include "gui/Clipboard.h"
#include "gui/EditWidgetIcons.h"
#include "gui/EditWidgetProperties.h"
#include "gui/FileDialog.h"
#include "gui/Font.h"
#include "gui/GuiTools.h"
#include "gui/Icons.h"
#include "gui/MessageBox.h"
#include "gui/entry/EntryAttributesModel.h"
#include "gui/entry/EntryHistoryModel.h"

EditEntryWidget::EditEntryWidget(QWidget *parent)
	: EditWidget(parent)
	, m_entry(nullptr)
	, m_mainUi(new Ui::EditEntryWidgetMain())
	, m_advancedUi(new Ui::EditEntryWidgetAdvanced())
	, m_historyUi(new Ui::EditEntryWidgetHistory())
	, m_attachments(new EntryAttachments())
	, m_customData(new CustomData())
	, m_mainWidget(new QScrollArea(this))
	, m_advancedWidget(new QWidget(this))
	, m_iconsWidget(new EditWidgetIcons(this))
	, m_editWidgetProperties(new EditWidgetProperties(this))
	, m_historyWidget(new QWidget(this))
	, m_entryAttributes(new EntryAttributes(this))
	, m_attributesModel(new EntryAttributesModel(m_advancedWidget))
	, m_historyModel(new EntryHistoryModel(this))
	, m_sortModel(new QSortFilterProxyModel(this))
	, m_usernameCompleter(new QCompleter(this))
	, m_usernameCompleterModel(new QStringListModel(this))
{
	setupMain();
	setupAdvanced();
	setupIcon();
	setupProperties();
	setupHistory();
	setupEntryUpdate();

	m_entryModifiedTimer.setSingleShot(true);
	m_entryModifiedTimer.setInterval(0);
	connect(&m_entryModifiedTimer, &QTimer::timeout, this, [this] {
		// TODO: Upon refactor of this widget, this needs to merge unsaved changes in the UI
		if (isVisible() && m_entry)
		{
			setForms(m_entry);
		}
	});

	connect(this, SIGNAL(accepted()), SLOT(acceptEntry()));
	connect(this, SIGNAL(rejected()), SLOT(cancel()));
	connect(this, SIGNAL(apply()), SLOT(commitEntry()));
	// clang-format off
    connect(m_iconsWidget,
            SIGNAL(messageEditEntry(QString,MessageWidget::MessageType)),
            SLOT(showMessage(QString,MessageWidget::MessageType)));
	// clang-format on

	connect(m_iconsWidget, SIGNAL(messageEditEntryDismiss()), SLOT(hideMessage()));

	m_editWidgetProperties->setCustomData(m_customData.data());

	m_mainUi->passwordEdit->setQualityVisible(true);
}

EditEntryWidget::~EditEntryWidget()
{
}

bool EditEntryWidget::switchToPage(Page page)
{
	auto index = pageIndex(widgetForPage(page));
	if (index >= 0)
	{
		setCurrentPage(index);
		return true;
	}
	return false;
}

QWidget *EditEntryWidget::widgetForPage(Page page) const
{
	switch (page)
	{
	case Page::Main:
		return m_mainWidget;
	case Page::Advanced:
		return m_advancedWidget;
	case Page::Icon:
		return m_iconsWidget;
	case Page::Properties:
		return m_editWidgetProperties;
	case Page::History:
		return m_historyWidget;
	}
	return nullptr;
}

void EditEntryWidget::setupMain()
{
	m_mainUi->setupUi(m_mainWidget);
	addPage(tr("Entry"), icons()->icon("document-edit"), m_mainWidget);

	// Disable mouse wheel grab when scrolling
	m_mainUi->usernameComboBox->installEventFilter(new MouseWheelEventFilter(this));
	m_mainUi->usernameComboBox->setEditable(true);
	m_mainUi->usernameComboBox->lineEdit()->setFocusPolicy(Qt::StrongFocus);
	m_usernameCompleter->setCompletionMode(QCompleter::InlineCompletion);
	m_usernameCompleter->setCaseSensitivity(Qt::CaseSensitive);
	m_usernameCompleter->setModel(m_usernameCompleterModel);
	m_mainUi->usernameComboBox->setCompleter(m_usernameCompleter);

	connect(m_mainUi->expireCheck, &QCheckBox::toggled, [&](bool enabled) {
		m_mainUi->expireDatePicker->setEnabled(enabled);
		if (enabled)
		{
			m_mainUi->expireDatePicker->setDateTime(Clock::currentDateTime());
		}
	});

	connect(m_mainUi->revealNotesButton, &QToolButton::clicked, this, &EditEntryWidget::toggleHideNotes);

	m_mainUi->expirePresets->setMenu(createPresetsMenu());
	connect(m_mainUi->expirePresets->menu(), SIGNAL(triggered(QAction *)), this, SLOT(useExpiryPreset(QAction *)));
}

void EditEntryWidget::setupAdvanced()
{
	m_advancedUi->setupUi(m_advancedWidget);
	addPage(tr("Advanced"), icons()->icon("preferences-other"), m_advancedWidget);

	m_advancedUi->attachmentsWidget->setReadOnly(false);
	m_advancedUi->attachmentsWidget->setButtonsVisible(true);

	connect(m_advancedUi->attachmentsWidget,
	        &EntryAttachmentsWidget::errorOccurred,
	        this,
	        [this](const QString &error) { showMessage(error, MessageWidget::Error); });

	m_attributesModel->setEntryAttributes(m_entryAttributes);
	m_advancedUi->attributesView->setModel(m_attributesModel);

	// clang-format off
    connect(m_advancedUi->addAttributeButton, SIGNAL(clicked()), SLOT(insertAttribute()));
    connect(m_advancedUi->editAttributeButton, SIGNAL(clicked()), SLOT(editCurrentAttribute()));
    connect(m_advancedUi->removeAttributeButton, SIGNAL(clicked()), SLOT(removeCurrentAttribute()));
    connect(m_advancedUi->protectAttributeButton, SIGNAL(toggled(bool)), SLOT(protectCurrentAttribute(bool)));
    connect(m_advancedUi->revealAttributeButton, SIGNAL(clicked(bool)), SLOT(toggleCurrentAttributeVisibility()));
    connect(m_advancedUi->attributesView->selectionModel(),
            SIGNAL(currentChanged(QModelIndex,QModelIndex)),
            SLOT(updateCurrentAttribute()));
    connect(m_advancedUi->fgColorButton, SIGNAL(clicked()), SLOT(pickColor()));
    connect(m_advancedUi->bgColorButton, SIGNAL(clicked()), SLOT(pickColor()));
	// clang-format on
}

void EditEntryWidget::setupIcon()
{
	m_iconsWidget->setShowApplyIconToButton(false);
	addPage(tr("Icon"), icons()->icon("preferences-desktop-icons"), m_iconsWidget);
	connect(this, SIGNAL(accepted()), m_iconsWidget, SLOT(abortRequests()));
	connect(this, SIGNAL(rejected()), m_iconsWidget, SLOT(abortRequests()));
}

void EditEntryWidget::setupProperties()
{
	addPage(tr("Properties"), icons()->icon("document-properties"), m_editWidgetProperties);
}

void EditEntryWidget::setupHistory()
{
	m_historyUi->setupUi(m_historyWidget);
	addPage(tr("History"), icons()->icon("view-history"), m_historyWidget);

	m_sortModel->setSourceModel(m_historyModel);
	m_sortModel->setDynamicSortFilter(true);
	m_sortModel->setSortLocaleAware(true);
	m_sortModel->setSortCaseSensitivity(Qt::CaseInsensitive);
	m_sortModel->setSortRole(Qt::UserRole);

	m_historyUi->historyView->setModel(m_sortModel);
	m_historyUi->historyView->setRootIsDecorated(false);

	// clang-format off
    connect(m_historyUi->historyView, SIGNAL(activated(QModelIndex)), SLOT(histEntryActivated(QModelIndex)));
    connect(m_historyUi->historyView->selectionModel(),
            SIGNAL(currentChanged(QModelIndex,QModelIndex)),
            SLOT(updateHistoryButtons(QModelIndex,QModelIndex)));

    connect(m_historyUi->showButton, SIGNAL(clicked()), SLOT(showHistoryEntry()));
    connect(m_historyUi->restoreButton, SIGNAL(clicked()), SLOT(restoreHistoryEntry()));
    connect(m_historyUi->deleteButton, SIGNAL(clicked()), SLOT(deleteHistoryEntry()));
    connect(m_historyUi->deleteAllButton, SIGNAL(clicked()), SLOT(deleteAllHistoryEntries()));
	// clang-format on
}

void EditEntryWidget::setupEntryUpdate()
{
	// Entry tab
	connect(m_mainUi->titleEdit, SIGNAL(textChanged(QString)), this, SLOT(setModified()));
	connect(m_mainUi->usernameComboBox->lineEdit(), SIGNAL(textChanged(QString)), this, SLOT(setModified()));
	connect(m_mainUi->passwordEdit, SIGNAL(textChanged(QString)), this, SLOT(setModified()));
	connect(m_mainUi->urlEdit, SIGNAL(textChanged(QString)), this, SLOT(setModified()));
	connect(m_mainUi->tagsList, SIGNAL(tagsEdited()), this, SLOT(setModified()));
	connect(m_mainUi->expireCheck, SIGNAL(stateChanged(int)), this, SLOT(setModified()));
	connect(m_mainUi->expireDatePicker, SIGNAL(dateTimeChanged(QDateTime)), this, SLOT(setModified()));
	connect(m_mainUi->notesEdit, SIGNAL(textChanged()), this, SLOT(setModified()));

	// Advanced tab
	connect(m_advancedUi->attributesEdit, SIGNAL(textChanged()), this, SLOT(setModified()));
	connect(m_advancedUi->protectAttributeButton, SIGNAL(stateChanged(int)), this, SLOT(setModified()));
	connect(m_advancedUi->excludeReportsCheckBox, SIGNAL(stateChanged(int)), this, SLOT(setModified()));
	connect(m_advancedUi->fgColorCheckBox, SIGNAL(stateChanged(int)), this, SLOT(setModified()));
	connect(m_advancedUi->bgColorCheckBox, SIGNAL(stateChanged(int)), this, SLOT(setModified()));
	connect(m_advancedUi->attachmentsWidget, SIGNAL(widgetUpdated()), this, SLOT(setModified()));

	// Icon tab
	connect(m_iconsWidget, SIGNAL(widgetUpdated()), this, SLOT(setModified()));

	// Properties and History tabs don't need extra connections
}

void EditEntryWidget::emitHistoryEntryActivated(const QModelIndex &index)
{
	Q_ASSERT(!m_history);

	Entry *entry = m_historyModel->entryFromIndex(index);
	if (entry)
	{
		emit historyEntryActivated(entry);
	}
}

void EditEntryWidget::histEntryActivated(const QModelIndex &index)
{
	Q_ASSERT(!m_history);

	QModelIndex indexMapped = m_sortModel->mapToSource(index);
	if (indexMapped.isValid())
	{
		emitHistoryEntryActivated(indexMapped);
	}
}

void EditEntryWidget::updateHistoryButtons(const QModelIndex &current, const QModelIndex &previous)
{
	Q_UNUSED(previous);

	if (m_historyModel->entryFromIndex(current))
	{
		m_historyUi->showButton->setEnabled(true);
		m_historyUi->restoreButton->setEnabled(true);
		m_historyUi->deleteButton->setEnabled(true);
	}
	else
	{
		m_historyUi->showButton->setEnabled(false);
		m_historyUi->restoreButton->setEnabled(false);
		m_historyUi->deleteButton->setEnabled(false);
	}
}

void EditEntryWidget::useExpiryPreset(QAction *action)
{
	m_mainUi->expireCheck->setChecked(true);
	TimeDelta delta = action->data().value<TimeDelta>();
	QDateTime now = Clock::currentDateTime();
	QDateTime expiryDateTime = now + delta;
	m_mainUi->expireDatePicker->setDateTime(expiryDateTime);
}

void EditEntryWidget::toggleHideNotes(bool visible)
{
	m_mainUi->notesEdit->setVisible(visible);
	m_mainUi->revealNotesButton->setIcon(icons()->onOffIcon("password-show", visible));
}

Entry *EditEntryWidget::currentEntry() const
{
	return m_entry;
}

void EditEntryWidget::loadEntry(Entry *entry,
                                bool create,
                                bool history,
                                const QString &parentName,
                                QSharedPointer<Database> database)
{
	m_entry = entry;
	m_db = std::move(database);
	m_create = create;
	m_history = history;

	if (history)
	{
		setHeadline(QString("%1 \u2022 %2").arg(parentName, tr("Entry history")));
	}
	else
	{
		if (create)
		{
			setHeadline(QString("%1 \u2022 %2").arg(parentName, tr("Add entry")));
		}
		else
		{
			setHeadline(QString("%1 \u2022 %2 \u2022 %3").arg(parentName, entry->title(), tr("Edit entry")));
			// Reload entry details if changed outside of the edit dialog
			connect(m_entry, &Entry::modified, this, [this] { m_entryModifiedTimer.start(); });
		}
	}

	setForms(entry);
	setReadOnly(m_history);

	switchToPage(Page::Main);
	setPageHidden(m_historyWidget, m_history || m_entry->historyItems().count() < 1);

	// Force the user to Save/Discard new entries
	showApplyButton(!m_create);

	// Set an initial password for new entries if the option is enabled
	if (create && config()->get(Config::AutoGeneratePasswordForNewEntries).toBool())
	{
		PasswordGenerator generator;
		m_mainUi->passwordEdit->setText(generator.generatePassword());
	}

	setModified(false);
}

void EditEntryWidget::setForms(Entry *entry, bool restore)
{
	m_attachments->copyDataFrom(entry->attachments());
	m_customData->copyDataFrom(entry->customData());

	m_mainUi->titleEdit->setReadOnly(m_history);
	m_mainUi->usernameComboBox->lineEdit()->setReadOnly(m_history);
	m_mainUi->urlEdit->setReadOnly(m_history);
	m_mainUi->passwordEdit->setReadOnly(m_history);
	m_mainUi->tagsList->tags(entry->tagList());
	m_mainUi->tagsList->completion(m_db->tagList());
	m_mainUi->expireCheck->setEnabled(!m_history);
	m_mainUi->expireDatePicker->setReadOnly(m_history);
	m_mainUi->revealNotesButton->setIcon(icons()->onOffIcon("password-show", false));
	m_mainUi->revealNotesButton->setVisible(config()->get(Config::Security_HideNotes).toBool());
	m_mainUi->revealNotesButton->setChecked(false);
	m_mainUi->notesEdit->setReadOnly(m_history);
	m_mainUi->notesEdit->setVisible(!config()->get(Config::Security_HideNotes).toBool());
	if (config()->get(Config::GUI_MonospaceNotes).toBool())
	{
		m_mainUi->notesEdit->setFont(Font::fixedFont());
	}
	else
	{
		m_mainUi->notesEdit->setFont(Font::defaultFont());
	}

	m_advancedUi->attachmentsWidget->setReadOnly(m_history);
	m_advancedUi->addAttributeButton->setEnabled(!m_history);
	m_advancedUi->editAttributeButton->setEnabled(false);
	m_advancedUi->removeAttributeButton->setEnabled(false);
	m_advancedUi->attributesEdit->setReadOnly(m_history);
	QAbstractItemView::EditTriggers editTriggers;
	if (m_history)
	{
		editTriggers = QAbstractItemView::NoEditTriggers;
	}
	else
	{
		editTriggers = QAbstractItemView::DoubleClicked;
	}
	m_advancedUi->attributesView->setEditTriggers(editTriggers);
	m_advancedUi->excludeReportsCheckBox->setChecked(entry->excludeFromReports());
	setupColorButton(true, entry->foregroundColor());
	setupColorButton(false, entry->backgroundColor());
	m_iconsWidget->setEnabled(!m_history);
	m_historyWidget->setEnabled(!m_history);

	m_mainUi->titleEdit->setText(entry->title());
	m_mainUi->usernameComboBox->lineEdit()->setText(entry->username());
	m_mainUi->urlEdit->setText(entry->url());
	m_mainUi->passwordEdit->setText(entry->password());
	m_mainUi->passwordEdit->setShowPassword(!config()->get(Config::Security_PasswordsHidden).toBool());
	if (!m_history)
	{
		m_mainUi->passwordEdit->enablePasswordGenerator();
	}
	m_mainUi->expireCheck->setChecked(entry->timeInfo().expires());
	m_mainUi->expireDatePicker->setDateTime(entry->timeInfo().expiryTime().toLocalTime());
	m_mainUi->expirePresets->setEnabled(!m_history);

	QList<QString> commonUsernames = m_db->commonUsernames();
	m_usernameCompleterModel->setStringList(commonUsernames);
	QString usernameToRestore = m_mainUi->usernameComboBox->lineEdit()->text();
	m_mainUi->usernameComboBox->clear();
	m_mainUi->usernameComboBox->addItems(commonUsernames);
	m_mainUi->usernameComboBox->lineEdit()->setText(usernameToRestore);

	m_mainUi->notesEdit->setPlainText(entry->notes());

	m_advancedUi->attachmentsWidget->linkAttachments(m_attachments.data());
	m_entryAttributes->copyCustomKeysFrom(entry->attributes());

	if (m_attributesModel->rowCount() != 0)
	{
		m_advancedUi->attributesView->setCurrentIndex(m_attributesModel->index(0, 0));
	}
	else
	{
		m_advancedUi->attributesEdit->setPlainText("");
		m_advancedUi->attributesEdit->setEnabled(false);
	}

	QList<int> sizes = m_advancedUi->attributesSplitter->sizes();
	sizes.replace(0, m_advancedUi->attributesSplitter->width() * 0.3);
	sizes.replace(1, m_advancedUi->attributesSplitter->width() * 0.7);
	m_advancedUi->attributesSplitter->setSizes(sizes);

	IconStruct iconStruct;
	iconStruct.uuid = entry->iconUuid();
	iconStruct.number = entry->iconNumber();
	m_iconsWidget->load(entry->uuid(), m_db, iconStruct, entry->webUrl());

	m_editWidgetProperties->setFields(entry->timeInfo(), entry->uuid());

	if (!m_history && !restore)
	{
		m_historyModel->setEntries(entry->historyItems(), entry);
		m_historyUi->historyView->sortByColumn(0, Qt::DescendingOrder);
	}
	if (m_historyModel->rowCount() > 0)
	{
		m_historyUi->deleteAllButton->setEnabled(true);
	}
	else
	{
		m_historyUi->deleteAllButton->setEnabled(false);
	}

	updateHistoryButtons(m_historyUi->historyView->currentIndex(), QModelIndex());

	m_mainUi->titleEdit->setFocus();
}

/**
 * Commit the form values to in-memory database representation
 *
 * @return true is commit successful, otherwise false
 */
bool EditEntryWidget::commitEntry()
{
	if (m_history)
	{
		clear();
		hideMessage();
		emit editFinished(false);
		return true;
	}

	// HACK: Check that entry pointer is still valid, see https://github.com/keepassxreboot/keepassxc/issues/5722
	if (!m_entry)
	{
		QMessageBox::information(this,
		                         tr("Invalid Entry"),
		                         tr("An external merge operation has invalidated this entry.\n"
		                            "Unfortunately, any changes made have been lost."));
		return true;
	}

	if (m_advancedUi->attributesView->currentIndex().isValid() && m_advancedUi->attributesEdit->isEnabled())
	{
		QString key = m_attributesModel->keyByIndex(m_advancedUi->attributesView->currentIndex());
		m_entryAttributes->set(key, m_advancedUi->attributesEdit->toPlainText(), m_entryAttributes->isProtected(key));
	}

	m_currentAttribute = QPersistentModelIndex();

	// must stand before beginUpdate()
	// we don't want to create a new history item, if only the history has changed
	m_entry->removeHistoryItems(m_historyModel->deletedEntries());
	m_historyModel->clearDeletedEntries();

	// Begin entry update
	if (!m_create)
	{
		m_entry->beginUpdate();
	}

	updateEntryData(m_entry);

	if (!m_create)
	{
		m_entry->endUpdate();
	}
	// End entry update

	m_historyModel->setEntries(m_entry->historyItems(), m_entry);
	setPageHidden(m_historyWidget, m_history || m_entry->historyItems().count() < 1);
	m_advancedUi->attachmentsWidget->linkAttachments(m_entry->attachments());

	showMessage(tr("Entry updated successfully."), MessageWidget::Positive);
	setModified(false);
	// Prevent a reload due to entry modified signals
	m_entryModifiedTimer.stop();

	return true;
}

void EditEntryWidget::acceptEntry()
{
	if (commitEntry())
	{
		clear();
		emit editFinished(true);
	}
}

void EditEntryWidget::updateEntryData(Entry *entry) const
{
	QRegularExpression newLineRegex("(?:\r?\n|\r)");

	entry->attributes()->copyCustomKeysFrom(m_entryAttributes);
	entry->attachments()->copyDataFrom(m_attachments.data());
	entry->customData()->copyDataFrom(m_customData.data());
	entry->setTitle(m_mainUi->titleEdit->text().replace(newLineRegex, " "));
	entry->setUsername(m_mainUi->usernameComboBox->lineEdit()->text().replace(newLineRegex, " "));
	entry->setUrl(m_mainUi->urlEdit->text().replace(newLineRegex, " "));
	entry->setPassword(m_mainUi->passwordEdit->text());
	entry->setExpires(m_mainUi->expireCheck->isChecked());
	entry->setExpiryTime(m_mainUi->expireDatePicker->dateTime().toUTC());

	QStringList uniqueTags(m_mainUi->tagsList->tags());
	uniqueTags.removeDuplicates();
	entry->setTags(uniqueTags.join(";"));

	entry->setNotes(m_mainUi->notesEdit->toPlainText());

	if (entry->excludeFromReports() != m_advancedUi->excludeReportsCheckBox->isChecked())
	{
		entry->setExcludeFromReports(m_advancedUi->excludeReportsCheckBox->isChecked());
	}

	if (m_advancedUi->fgColorCheckBox->isChecked() && m_advancedUi->fgColorButton->property("color").isValid())
	{
		entry->setForegroundColor(m_advancedUi->fgColorButton->property("color").toString());
	}
	else
	{
		entry->setForegroundColor(QString());
	}

	if (m_advancedUi->bgColorCheckBox->isChecked() && m_advancedUi->bgColorButton->property("color").isValid())
	{
		entry->setBackgroundColor(m_advancedUi->bgColorButton->property("color").toString());
	}
	else
	{
		entry->setBackgroundColor(QString());
	}

	IconStruct iconStruct = m_iconsWidget->state();

	if (iconStruct.number < 0)
	{
		entry->setIcon(Entry::DefaultIconNumber);
	}
	else if (iconStruct.uuid.isNull())
	{
		entry->setIcon(iconStruct.number);
	}
	else
	{
		entry->setIcon(iconStruct.uuid);
	}
}

void EditEntryWidget::cancel()
{
	if (m_history)
	{
		clear();
		hideMessage();
		emit editFinished(false);
		return;
	}

	if (!m_entry->iconUuid().isNull() && !m_db->metadata()->hasCustomIcon(m_entry->iconUuid()))
	{
		m_entry->setIcon(Entry::DefaultIconNumber);
	}

	bool accepted = false;
	if (isModified())
	{
		auto result = MessageBox::question(this,
		                                   tr("Unsaved Changes"),
		                                   tr("Would you like to save changes to this entry?"),
		                                   MessageBox::Cancel | MessageBox::Save | MessageBox::Discard,
		                                   MessageBox::Cancel);
		if (result == MessageBox::Cancel)
		{
			return;
		}
		else if (result == MessageBox::Save)
		{
			accepted = true;
			if (!commitEntry())
			{
				return;
			}
		}
	}

	clear();
	emit editFinished(accepted);
}

void EditEntryWidget::clear()
{
	if (m_entry)
	{
		m_entry->disconnect(this);
	}

	m_entry = nullptr;
	m_db.reset();

	m_mainUi->titleEdit->setText("");
	m_mainUi->passwordEdit->setText("");
	m_mainUi->urlEdit->setText("");
	m_mainUi->notesEdit->clear();

	m_entryAttributes->clear();
	m_attachments->clear();
	m_customData->clear();
	m_historyModel->clear();
	m_iconsWidget->reset();
	hideMessage();
}

void EditEntryWidget::insertAttribute()
{
	Q_ASSERT(!m_history);

	QString name = tr("New attribute");
	int i = 1;

	while (m_entryAttributes->keys().contains(name))
	{
		name = tr("New attribute %1").arg(i);
		i++;
	}

	m_entryAttributes->set(name, "");
	QModelIndex index = m_attributesModel->indexByKey(name);

	m_advancedUi->attributesView->setCurrentIndex(index);
	m_advancedUi->attributesView->edit(index);

	setModified(true);
}

void EditEntryWidget::editCurrentAttribute()
{
	Q_ASSERT(!m_history);

	QModelIndex index = m_advancedUi->attributesView->currentIndex();

	if (index.isValid())
	{
		m_advancedUi->attributesView->edit(index);
		setModified(true);
	}
}

void EditEntryWidget::removeCurrentAttribute()
{
	Q_ASSERT(!m_history);

	QModelIndex index = m_advancedUi->attributesView->currentIndex();

	if (index.isValid())
	{

		auto result = MessageBox::question(this,
		                                   tr("Confirm Removal"),
		                                   tr("Are you sure you want to remove this attribute?"),
		                                   MessageBox::Remove | MessageBox::Cancel,
		                                   MessageBox::Cancel);

		if (result == MessageBox::Remove)
		{
			m_entryAttributes->remove(m_attributesModel->keyByIndex(index));
			setModified(true);
		}
	}
}

void EditEntryWidget::updateCurrentAttribute()
{
	QModelIndex newIndex = m_advancedUi->attributesView->currentIndex();
	QString newKey = m_attributesModel->keyByIndex(newIndex);

	if (!m_history && m_currentAttribute != newIndex)
	{
		// Save changes to the currently selected attribute if editing is enabled
		if (m_currentAttribute.isValid() && m_advancedUi->attributesEdit->isEnabled())
		{
			QString currKey = m_attributesModel->keyByIndex(m_currentAttribute);
			m_entryAttributes->set(
				currKey, m_advancedUi->attributesEdit->toPlainText(), m_entryAttributes->isProtected(currKey));
		}
	}

	displayAttribute(newIndex, m_entryAttributes->isProtected(newKey));

	m_currentAttribute = newIndex;
}

void EditEntryWidget::displayAttribute(QModelIndex index, bool showProtected)
{
	// Block signals to prevent modified being set
	m_advancedUi->protectAttributeButton->blockSignals(true);
	m_advancedUi->attributesEdit->blockSignals(true);
	m_advancedUi->revealAttributeButton->setText(tr("Reveal"));

	if (index.isValid())
	{
		QString key = m_attributesModel->keyByIndex(index);
		if (showProtected)
		{
			m_advancedUi->attributesEdit->setPlainText(tr("[PROTECTED] Press Reveal to view or edit"));
			m_advancedUi->attributesEdit->setEnabled(false);
			m_advancedUi->revealAttributeButton->setEnabled(true);
			m_advancedUi->protectAttributeButton->setChecked(true);
		}
		else
		{
			m_advancedUi->attributesEdit->setPlainText(m_entryAttributes->value(key));
			m_advancedUi->attributesEdit->setEnabled(true);
			m_advancedUi->revealAttributeButton->setEnabled(false);
			m_advancedUi->protectAttributeButton->setChecked(false);
		}

		// Don't allow editing in history view
		m_advancedUi->protectAttributeButton->setEnabled(!m_history);
		m_advancedUi->editAttributeButton->setEnabled(!m_history);
		m_advancedUi->removeAttributeButton->setEnabled(!m_history);
	}
	else
	{
		m_advancedUi->attributesEdit->setPlainText("");
		m_advancedUi->attributesEdit->setEnabled(false);
		m_advancedUi->revealAttributeButton->setEnabled(false);
		m_advancedUi->protectAttributeButton->setChecked(false);
		m_advancedUi->protectAttributeButton->setEnabled(false);
		m_advancedUi->editAttributeButton->setEnabled(false);
		m_advancedUi->removeAttributeButton->setEnabled(false);
	}

	m_advancedUi->protectAttributeButton->blockSignals(false);
	m_advancedUi->attributesEdit->blockSignals(false);
}

void EditEntryWidget::protectCurrentAttribute(bool state)
{
	QModelIndex index = m_advancedUi->attributesView->currentIndex();
	if (!m_history && index.isValid())
	{
		QString key = m_attributesModel->keyByIndex(index);
		if (state)
		{
			// Save the current text and protect the attribute
			m_entryAttributes->set(key, m_advancedUi->attributesEdit->toPlainText(), true);
		}
		else
		{
			// Unprotect the current attribute value (don't save text as it is obscured)
			m_entryAttributes->set(key, m_entryAttributes->value(key), false);
		}

		// Display the attribute
		displayAttribute(index, state);
	}
}

void EditEntryWidget::toggleCurrentAttributeVisibility()
{
	if (!m_advancedUi->attributesEdit->isEnabled())
	{
		QModelIndex index = m_advancedUi->attributesView->currentIndex();
		if (index.isValid())
		{
			bool oldBlockSignals = m_advancedUi->attributesEdit->blockSignals(true);
			QString key = m_attributesModel->keyByIndex(index);
			m_advancedUi->attributesEdit->setPlainText(m_entryAttributes->value(key));
			m_advancedUi->attributesEdit->setEnabled(true);
			m_advancedUi->attributesEdit->blockSignals(oldBlockSignals);
		}
		m_advancedUi->revealAttributeButton->setText(tr("Hide"));
	}
	else
	{
		protectCurrentAttribute(true);
		m_advancedUi->revealAttributeButton->setText(tr("Reveal"));
	}
}

void EditEntryWidget::showHistoryEntry()
{
	QModelIndex index = m_sortModel->mapToSource(m_historyUi->historyView->currentIndex());
	if (index.isValid())
	{
		emitHistoryEntryActivated(index);
	}
}

void EditEntryWidget::restoreHistoryEntry()
{
	QModelIndex index = m_sortModel->mapToSource(m_historyUi->historyView->currentIndex());
	auto entry = m_historyModel->entryFromIndex(index);
	if (entry)
	{
		setForms(entry, true);
		setModified(true);
	}
}

void EditEntryWidget::deleteHistoryEntry()
{
	QModelIndex index = m_sortModel->mapToSource(m_historyUi->historyView->currentIndex());
	if (m_historyModel->entryFromIndex(index))
	{
		m_historyModel->deleteIndex(index);
		if (m_historyModel->rowCount() > 0)
		{
			m_historyUi->deleteAllButton->setEnabled(true);
		}
		else
		{
			m_historyUi->deleteAllButton->setEnabled(false);
		}
		setModified(true);
	}
}

void EditEntryWidget::deleteAllHistoryEntries()
{
	m_historyModel->deleteAll();
	m_historyUi->deleteAllButton->setEnabled(m_historyModel->rowCount() > 0);
	setModified(true);
}

QMenu *EditEntryWidget::createPresetsMenu()
{
	auto *expirePresetsMenu = new QMenu(this);
	expirePresetsMenu->addAction(tr("%n hour(s)", "", 12))->setData(QVariant::fromValue(TimeDelta::fromHours(12)));
	expirePresetsMenu->addAction(tr("%n hour(s)", "", 24))->setData(QVariant::fromValue(TimeDelta::fromHours(24)));
	expirePresetsMenu->addSeparator();
	expirePresetsMenu->addAction(tr("%n week(s)", "", 1))->setData(QVariant::fromValue(TimeDelta::fromDays(7)));
	expirePresetsMenu->addAction(tr("%n week(s)", "", 2))->setData(QVariant::fromValue(TimeDelta::fromDays(14)));
	expirePresetsMenu->addAction(tr("%n week(s)", "", 3))->setData(QVariant::fromValue(TimeDelta::fromDays(21)));
	expirePresetsMenu->addSeparator();
	expirePresetsMenu->addAction(tr("%n month(s)", "", 1))->setData(QVariant::fromValue(TimeDelta::fromMonths(1)));
	expirePresetsMenu->addAction(tr("%n month(s)", "", 2))->setData(QVariant::fromValue(TimeDelta::fromMonths(2)));
	expirePresetsMenu->addAction(tr("%n month(s)", "", 3))->setData(QVariant::fromValue(TimeDelta::fromMonths(3)));
	expirePresetsMenu->addAction(tr("%n month(s)", "", 6))->setData(QVariant::fromValue(TimeDelta::fromMonths(6)));
	expirePresetsMenu->addSeparator();
	expirePresetsMenu->addAction(tr("%n year(s)", "", 1))->setData(QVariant::fromValue(TimeDelta::fromYears(1)));
	expirePresetsMenu->addAction(tr("%n year(s)", "", 2))->setData(QVariant::fromValue(TimeDelta::fromYears(2)));
	expirePresetsMenu->addAction(tr("%n year(s)", "", 3))->setData(QVariant::fromValue(TimeDelta::fromYears(3)));
	return expirePresetsMenu;
}

void EditEntryWidget::setupColorButton(bool foreground, const QColor &color)
{
	QWidget *button = m_advancedUi->fgColorButton;
	QCheckBox *checkBox = m_advancedUi->fgColorCheckBox;
	if (!foreground)
	{
		button = m_advancedUi->bgColorButton;
		checkBox = m_advancedUi->bgColorCheckBox;
	}

	if (color.isValid())
	{
		button->setStyleSheet(QString("background-color:%1").arg(color.name()));
		button->setProperty("color", color.name());
		checkBox->setChecked(true);
	}
	else
	{
		button->setStyleSheet("");
		button->setProperty("color", QVariant());
		checkBox->setChecked(false);
	}
}

void EditEntryWidget::pickColor()
{
	bool isForeground = (sender() == m_advancedUi->fgColorButton);
	QColor oldColor = QColor(m_advancedUi->fgColorButton->property("color").toString());
	if (!isForeground)
	{
		oldColor = QColor(m_advancedUi->bgColorButton->property("color").toString());
	}

	QColor newColor = QColorDialog::getColor(oldColor);
	if (newColor.isValid())
	{
		setupColorButton(isForeground, newColor);
		setModified(true);
	}
}
