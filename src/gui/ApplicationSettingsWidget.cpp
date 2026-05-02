/*
 *  Copyright (C) 2026 i.Dark_Templar <darktemplar@dark-templar-archives.net>
 *  Copyright (C) 2023 KeePassXC Team <team@keepassxc.org>
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

#include "ApplicationSettingsWidget.h"
#include "ui_ApplicationSettingsWidgetGeneral.h"
#include "ui_ApplicationSettingsWidgetSecurity.h"
#include <QDesktopServices>
#include <QDir>

#include "gui/Application.h"
#include "gui/GuiTools.h"
#include "gui/Icons.h"
#include "gui/MainWindow.h"

#include "FileDialog.h"
#include "MessageBox.h"

class ApplicationSettingsWidget::ExtraPage
{
public:
	ExtraPage(ISettingsPage *page, QWidget *widget)
		: settingsPage(page)
		, widget(widget)
	{
	}

	void loadSettings() const
	{
		settingsPage->loadSettings(widget);
	}

	void saveSettings() const
	{
		settingsPage->saveSettings(widget);
	}

private:
	QSharedPointer<ISettingsPage> settingsPage;
	QWidget *widget;
};

ApplicationSettingsWidget::ApplicationSettingsWidget(QWidget *parent)
	: EditWidget(parent)
	, m_secWidget(new QWidget())
	, m_generalWidget(new QWidget())
	, m_secUi(new Ui::ApplicationSettingsWidgetSecurity())
	, m_generalUi(new Ui::ApplicationSettingsWidgetGeneral())
{
	setHeadline(tr("Application Settings"));
	showApplyButton(false);

	m_secUi->setupUi(m_secWidget);
	m_generalUi->setupUi(m_generalWidget);
	addPage(tr("General"), icons()->icon("preferences-other"), m_generalWidget);
	addPage(tr("Security"), icons()->icon("security-high"), m_secWidget);

	connect(this, &ApplicationSettingsWidget::accepted, this, &ApplicationSettingsWidget::saveSettings);

	connect(m_generalUi->autoSaveAfterEveryChangeCheckBox, &QCheckBox::toggled, this, &ApplicationSettingsWidget::autoSaveToggled);
	connect(m_generalUi->hideWindowOnCopyCheckBox, &QCheckBox::toggled, this, &ApplicationSettingsWidget::hideWindowOnCopyCheckBoxToggled);
	connect(m_generalUi->systrayShowCheckBox, &QCheckBox::toggled, this, &ApplicationSettingsWidget::systrayToggled);
	connect(m_generalUi->rememberLastDatabasesCheckBox, &QCheckBox::toggled, this, &ApplicationSettingsWidget::rememberDatabasesToggled);
	connect(m_generalUi->resetSettingsButton, &QPushButton::clicked, this, &ApplicationSettingsWidget::resetSettings);
	connect(m_generalUi->importSettingsButton, &QPushButton::clicked, this, &ApplicationSettingsWidget::importSettings);
	connect(m_generalUi->exportSettingsButton, &QPushButton::clicked, this, &ApplicationSettingsWidget::exportSettings);

	connect(m_generalUi->backupBeforeSaveCheckBox, &QCheckBox::toggled, m_generalUi->backupFilePath, &QLineEdit::setEnabled);
	connect(m_generalUi->backupBeforeSaveCheckBox, &QCheckBox::toggled, m_generalUi->backupFilePathPicker, &QPushButton::setEnabled);
	connect(m_generalUi->backupFilePathPicker, &QPushButton::pressed, this, &ApplicationSettingsWidget::selectBackupDirectory);
	connect(m_generalUi->showExpiredEntriesOnDatabaseUnlockCheckBox, &QCheckBox::toggled, this, &ApplicationSettingsWidget::showExpiredEntriesOnDatabaseUnlockToggled);

	connect(m_secUi->clearClipboardCheckBox, &QCheckBox::toggled, m_secUi->clearClipboardSpinBox, &QSpinBox::setEnabled);
	connect(m_secUi->clearSearchCheckBox, &QCheckBox::toggled, m_secUi->clearSearchSpinBox, &QSpinBox::setEnabled);
	connect(m_secUi->lockDatabaseIdleCheckBox, &QCheckBox::toggled, m_secUi->lockDatabaseIdleSpinBox, &QSpinBox::setEnabled);

	connect(m_generalUi->minimizeAfterUnlockCheckBox, &QCheckBox::toggled, this, [this](bool state) {
		if (state)
		{
			m_secUi->lockDatabaseMinimizeCheckBox->setChecked(false);
		}

		m_secUi->lockDatabaseMinimizeCheckBox->setToolTip(state ? tr("This setting cannot be enabled when minimize on unlock is enabled.") : QString());
		m_secUi->lockDatabaseMinimizeCheckBox->setEnabled(!state);
	});

	// Disable mouse wheel grab when scrolling
	// This prevents combo box and spinner values from changing without explicit focus
	auto mouseWheelFilter = new MouseWheelEventFilter(this);
	m_generalUi->toolButtonStyleComboBox->installEventFilter(mouseWheelFilter);
	m_generalUi->fontSizeComboBox->installEventFilter(mouseWheelFilter);
}

ApplicationSettingsWidget::~ApplicationSettingsWidget()
{
}

void ApplicationSettingsWidget::addSettingsPage(ISettingsPage *page)
{
	QWidget *widget = page->createWidget();
	widget->setParent(this);
	m_extraPages.append(ExtraPage(page, widget));
	addPage(page->name(), page->icon(), widget);
}

void ApplicationSettingsWidget::loadSettings()
{
	if (config()->hasAccessError())
	{
		showMessage(tr("Access error for config file %1").arg(config()->getFileName()), MessageWidget::Error);
	}

	m_generalUi->rememberLastDatabasesCheckBox->setChecked(config()->get(Config::RememberLastDatabases).toBool());
	m_generalUi->rememberLastDatabasesSpinbox->setValue(config()->get(Config::NumberOfRememberedLastDatabases).toInt());
	m_generalUi->rememberLastKeyFilesCheckBox->setChecked(config()->get(Config::RememberLastKeyFiles).toBool());
	m_generalUi->openPreviousDatabasesOnStartupCheckBox->setChecked(
		config()->get(Config::OpenPreviousDatabasesOnStartup).toBool());
	m_generalUi->autoSaveAfterEveryChangeCheckBox->setChecked(config()->get(Config::AutoSaveAfterEveryChange).toBool());
	m_generalUi->autoSaveOnExitCheckBox->setChecked(config()->get(Config::AutoSaveOnExit).toBool());
	m_generalUi->autoSaveNonDataChangesCheckBox->setChecked(config()->get(Config::AutoSaveNonDataChanges).toBool());
	m_generalUi->backupBeforeSaveCheckBox->setChecked(config()->get(Config::BackupBeforeSave).toBool());

	m_generalUi->backupFilePath->setText(config()->get(Config::BackupFilePathPattern).toString());

	m_generalUi->autoReloadOnChangeCheckBox->setChecked(config()->get(Config::AutoReloadOnChange).toBool());
	m_generalUi->minimizeAfterUnlockCheckBox->setChecked(config()->get(Config::MinimizeAfterUnlock).toBool());
	m_generalUi->urlDoubleClickComboBox->setCurrentIndex(config()->get(Config::URLDoubleClickAction).toInt());
	m_generalUi->hideWindowOnCopyCheckBox->setChecked(config()->get(Config::HideWindowOnCopy).toBool());
	hideWindowOnCopyCheckBoxToggled(m_generalUi->hideWindowOnCopyCheckBox->isChecked());
	m_generalUi->minimizeOnCopyRadioButton->setChecked(config()->get(Config::MinimizeOnCopy).toBool());
	m_generalUi->dropToBackgroundOnCopyRadioButton->setChecked(config()->get(Config::DropToBackgroundOnCopy).toBool());
	m_generalUi->useGroupIconOnEntryCreationCheckBox->setChecked(
		config()->get(Config::UseGroupIconOnEntryCreation).toBool());
	m_generalUi->ConfirmMoveEntryToRecycleBinCheckBox->setChecked(
		!config()->get(Config::Security_NoConfirmMoveEntryToRecycleBin).toBool());
	m_generalUi->EnableCopyOnDoubleClickCheckBox->setChecked(
		config()->get(Config::Security_EnableCopyOnDoubleClick).toBool());
	m_generalUi->autoGeneratePasswordForNewEntriesCheckBox->setChecked(
		config()->get(Config::AutoGeneratePasswordForNewEntries).toBool());

	m_generalUi->menubarShowCheckBox->setChecked(!config()->get(Config::GUI_HideMenubar).toBool());
	m_generalUi->toolbarShowCheckBox->setChecked(!config()->get(Config::GUI_HideToolbar).toBool());
	m_generalUi->toolbarMovableCheckBox->setChecked(config()->get(Config::GUI_MovableToolbar).toBool());
	m_generalUi->monospaceNotesCheckBox->setChecked(config()->get(Config::GUI_MonospaceNotes).toBool());
	m_generalUi->colorPasswordsCheckBox->setChecked(config()->get(Config::GUI_ColorPasswords).toBool());

	m_generalUi->toolButtonStyleComboBox->clear();
	m_generalUi->toolButtonStyleComboBox->addItem(tr("Icon only"), Qt::ToolButtonIconOnly);
	m_generalUi->toolButtonStyleComboBox->addItem(tr("Text only"), Qt::ToolButtonTextOnly);
	m_generalUi->toolButtonStyleComboBox->addItem(tr("Text beside icon"), Qt::ToolButtonTextBesideIcon);
	m_generalUi->toolButtonStyleComboBox->addItem(tr("Text under icon"), Qt::ToolButtonTextUnderIcon);
	m_generalUi->toolButtonStyleComboBox->addItem(tr("Follow style"), Qt::ToolButtonFollowStyle);
	int toolButtonStyleIndex = m_generalUi->toolButtonStyleComboBox->findData(config()->get(Config::GUI_ToolButtonStyle));

	if (toolButtonStyleIndex >= 0)
	{
		m_generalUi->toolButtonStyleComboBox->setCurrentIndex(toolButtonStyleIndex);
	}

	m_generalUi->fontSizeComboBox->clear();
	m_generalUi->fontSizeComboBox->addItem(tr("Small"), -1);
	m_generalUi->fontSizeComboBox->addItem(tr("Normal"), 0);
	m_generalUi->fontSizeComboBox->addItem(tr("Medium"), 1);
	m_generalUi->fontSizeComboBox->addItem(tr("Large"), 2);

	int fontSizeIndex = m_generalUi->fontSizeComboBox->findData(config()->get(Config::GUI_FontSizeOffset));
	if (fontSizeIndex >= 0)
	{
		m_generalUi->fontSizeComboBox->setCurrentIndex(fontSizeIndex);
	}
	else
	{
		// Custom value entered into config file, add it to the list and select it
		m_generalUi->fontSizeComboBox->addItem(tr("Custom"), config()->get(Config::GUI_FontSizeOffset).toInt());
		m_generalUi->fontSizeComboBox->setCurrentIndex(m_generalUi->fontSizeComboBox->count() - 1);
	}

	m_generalUi->systrayShowCheckBox->setChecked(config()->get(Config::GUI_ShowTrayIcon).toBool());
	systrayToggled(m_generalUi->systrayShowCheckBox->isChecked());
	m_generalUi->systrayMinimizeToTrayCheckBox->setChecked(config()->get(Config::GUI_MinimizeToTray).toBool());
	m_generalUi->minimizeOnCloseCheckBox->setChecked(config()->get(Config::GUI_MinimizeOnClose).toBool());
	m_generalUi->systrayMinimizeOnStartup->setChecked(config()->get(Config::GUI_MinimizeOnStartup).toBool());

	m_generalUi->showExpiredEntriesOnDatabaseUnlockCheckBox->setChecked(
		config()->get(Config::GUI_ShowExpiredEntriesOnDatabaseUnlock).toBool());
	m_generalUi->showExpiredEntriesOnDatabaseUnlockOffsetSpinBox->setValue(
		config()->get(Config::GUI_ShowExpiredEntriesOnDatabaseUnlockOffsetDays).toInt());
	showExpiredEntriesOnDatabaseUnlockToggled(m_generalUi->showExpiredEntriesOnDatabaseUnlockCheckBox->isChecked());

	m_secUi->clearClipboardCheckBox->setChecked(config()->get(Config::Security_ClearClipboard).toBool());
	m_secUi->clearClipboardSpinBox->setValue(config()->get(Config::Security_ClearClipboardTimeout).toInt());

	m_secUi->clearSearchCheckBox->setChecked(config()->get(Config::Security_ClearSearch).toBool());
	m_secUi->clearSearchSpinBox->setValue(config()->get(Config::Security_ClearSearchTimeout).toInt());

	m_secUi->lockDatabaseIdleCheckBox->setChecked(config()->get(Config::Security_LockDatabaseIdle).toBool());
	m_secUi->lockDatabaseIdleSpinBox->setValue(config()->get(Config::Security_LockDatabaseIdleSeconds).toInt());
	m_secUi->lockDatabaseMinimizeCheckBox->setChecked(m_secUi->lockDatabaseMinimizeCheckBox->isEnabled()
		&& config()->get(Config::Security_LockDatabaseMinimize).toBool());
	m_secUi->lockDatabaseOnScreenLockCheckBox->setChecked(
		config()->get(Config::Security_LockDatabaseScreenLock).toBool());

	m_secUi->passwordsHiddenCheckBox->setChecked(config()->get(Config::Security_PasswordsHidden).toBool());
	m_secUi->passwordShowDotsCheckBox->setChecked(config()->get(Config::Security_PasswordEmptyPlaceholder).toBool());
	m_secUi->passwordPreviewCleartextCheckBox->setChecked(
		config()->get(Config::Security_HidePasswordPreviewPanel).toBool());
	m_secUi->hideTotpCheckBox->setChecked(config()->get(Config::Security_HideTotpPreviewPanel).toBool());
	m_secUi->hideNotesCheckBox->setChecked(config()->get(Config::Security_HideNotes).toBool());

	for (const ExtraPage &page: asConst(m_extraPages))
	{
		page.loadSettings();
	}

	setCurrentPage(0);
}

void ApplicationSettingsWidget::saveSettings()
{
	if (config()->hasAccessError())
	{
		showMessage(tr("Access error for config file %1").arg(config()->getFileName()), MessageWidget::Error);
		// We prevent closing the settings page if we could not write to
		// the config file.
		return;
	}

	config()->set(Config::RememberLastDatabases, m_generalUi->rememberLastDatabasesCheckBox->isChecked());
	config()->set(Config::NumberOfRememberedLastDatabases, m_generalUi->rememberLastDatabasesSpinbox->value());
	config()->set(Config::RememberLastKeyFiles, m_generalUi->rememberLastKeyFilesCheckBox->isChecked());
	config()->set(Config::OpenPreviousDatabasesOnStartup, m_generalUi->openPreviousDatabasesOnStartupCheckBox->isChecked());
	config()->set(Config::AutoSaveAfterEveryChange, m_generalUi->autoSaveAfterEveryChangeCheckBox->isChecked());
	config()->set(Config::AutoSaveOnExit, m_generalUi->autoSaveOnExitCheckBox->isChecked());
	config()->set(Config::AutoSaveNonDataChanges, m_generalUi->autoSaveNonDataChangesCheckBox->isChecked());
	config()->set(Config::BackupBeforeSave, m_generalUi->backupBeforeSaveCheckBox->isChecked());

	config()->set(Config::BackupFilePathPattern, m_generalUi->backupFilePath->text());

	config()->set(Config::AutoReloadOnChange, m_generalUi->autoReloadOnChangeCheckBox->isChecked());
	config()->set(Config::MinimizeAfterUnlock, m_generalUi->minimizeAfterUnlockCheckBox->isChecked());
	config()->set(Config::URLDoubleClickAction, m_generalUi->urlDoubleClickComboBox->currentIndex());
	config()->set(Config::HideWindowOnCopy, m_generalUi->hideWindowOnCopyCheckBox->isChecked());
	config()->set(Config::MinimizeOnCopy, m_generalUi->minimizeOnCopyRadioButton->isChecked());
	config()->set(Config::DropToBackgroundOnCopy, m_generalUi->dropToBackgroundOnCopyRadioButton->isChecked());
	config()->set(Config::UseGroupIconOnEntryCreation, m_generalUi->useGroupIconOnEntryCreationCheckBox->isChecked());
	config()->set(Config::Security_NoConfirmMoveEntryToRecycleBin, !m_generalUi->ConfirmMoveEntryToRecycleBinCheckBox->isChecked());
	config()->set(Config::Security_EnableCopyOnDoubleClick, m_generalUi->EnableCopyOnDoubleClickCheckBox->isChecked());
	config()->set(Config::AutoGeneratePasswordForNewEntries, m_generalUi->autoGeneratePasswordForNewEntriesCheckBox->isChecked());

	config()->set(Config::GUI_HideMenubar, !m_generalUi->menubarShowCheckBox->isChecked());
	config()->set(Config::GUI_HideToolbar, !m_generalUi->toolbarShowCheckBox->isChecked());
	config()->set(Config::GUI_MovableToolbar, m_generalUi->toolbarMovableCheckBox->isChecked());
	config()->set(Config::GUI_MonospaceNotes, m_generalUi->monospaceNotesCheckBox->isChecked());
	config()->set(Config::GUI_ColorPasswords, m_generalUi->colorPasswordsCheckBox->isChecked());

	config()->set(Config::GUI_ToolButtonStyle, m_generalUi->toolButtonStyleComboBox->currentData().toString());
	config()->set(Config::GUI_FontSizeOffset, m_generalUi->fontSizeComboBox->currentData().toInt());

	config()->set(Config::GUI_ShowTrayIcon, m_generalUi->systrayShowCheckBox->isChecked());
	config()->set(Config::GUI_MinimizeToTray, m_generalUi->systrayMinimizeToTrayCheckBox->isChecked());
	config()->set(Config::GUI_MinimizeOnClose, m_generalUi->minimizeOnCloseCheckBox->isChecked());
	config()->set(Config::GUI_MinimizeOnStartup, m_generalUi->systrayMinimizeOnStartup->isChecked());

	config()->set(Config::GUI_ShowExpiredEntriesOnDatabaseUnlock,
		m_generalUi->showExpiredEntriesOnDatabaseUnlockCheckBox->isChecked());
	config()->set(Config::GUI_ShowExpiredEntriesOnDatabaseUnlockOffsetDays,
		m_generalUi->showExpiredEntriesOnDatabaseUnlockOffsetSpinBox->value());

	config()->set(Config::Security_ClearClipboard, m_secUi->clearClipboardCheckBox->isChecked());
	config()->set(Config::Security_ClearClipboardTimeout, m_secUi->clearClipboardSpinBox->value());

	config()->set(Config::Security_ClearSearch, m_secUi->clearSearchCheckBox->isChecked());
	config()->set(Config::Security_ClearSearchTimeout, m_secUi->clearSearchSpinBox->value());

	config()->set(Config::Security_LockDatabaseIdle, m_secUi->lockDatabaseIdleCheckBox->isChecked());
	config()->set(Config::Security_LockDatabaseIdleSeconds, m_secUi->lockDatabaseIdleSpinBox->value());
	config()->set(Config::Security_LockDatabaseMinimize, m_secUi->lockDatabaseMinimizeCheckBox->isChecked());
	config()->set(Config::Security_LockDatabaseScreenLock, m_secUi->lockDatabaseOnScreenLockCheckBox->isChecked());

	config()->set(Config::Security_PasswordsHidden, m_secUi->passwordsHiddenCheckBox->isChecked());
	config()->set(Config::Security_PasswordEmptyPlaceholder, m_secUi->passwordShowDotsCheckBox->isChecked());

	config()->set(Config::Security_HidePasswordPreviewPanel, m_secUi->passwordPreviewCleartextCheckBox->isChecked());
	config()->set(Config::Security_HideTotpPreviewPanel, m_secUi->hideTotpCheckBox->isChecked());
	config()->set(Config::Security_HideNotes, m_secUi->hideNotesCheckBox->isChecked());

	// Security: clear storage if related settings are disabled
	if (!config()->get(Config::RememberLastDatabases).toBool())
	{
		config()->remove(Config::LastDir);
		config()->remove(Config::LastDatabases);
		config()->remove(Config::LastActiveDatabase);
	}

	if (!config()->get(Config::RememberLastKeyFiles).toBool())
	{
		config()->remove(Config::LastDir);
		config()->remove(Config::LastKeyFiles);
		config()->remove(Config::LastChallengeResponse);
	}

	for (const ExtraPage &page: asConst(m_extraPages))
	{
		page.saveSettings();
	}
}

void ApplicationSettingsWidget::resetSettings()
{
	// Confirm reset
	auto ans = MessageBox::question(this,
		tr("Confirm Reset"),
		tr("Are you sure you want to reset all settings to default?"),
		MessageBox::Reset | MessageBox::Cancel,
		MessageBox::Cancel);

	if (ans == MessageBox::Cancel)
	{
		return;
	}

	if (config()->hasAccessError())
	{
		showMessage(tr("Access error for config file %1").arg(config()->getFileName()), MessageWidget::Error);
		// We prevent closing the settings page if we could not write to
		// the config file.
		return;
	}

	// Reset general and security settings to default
	config()->resetToDefaults();

	// Clear recently used data
	config()->remove(Config::LastDatabases);
	config()->remove(Config::LastActiveDatabase);
	config()->remove(Config::LastKeyFiles);
	config()->remove(Config::LastDir);

	// Save the Extra Pages (these are NOT reset)
	for (const ExtraPage &page: asConst(m_extraPages))
	{
		page.saveSettings();
	}

	config()->sync();

	// Refresh the settings widget and notify listeners
	loadSettings();
	emit settingsReset();
}

void ApplicationSettingsWidget::importSettings()
{
	auto file = fileDialog()->getOpenFileName(this, tr("Import KeePassX-min Settings"), {}, "*.ini");
	if (file.isEmpty())
	{
		return;
	}

	if (!config()->importSettings(file))
	{
		showMessage(tr("Failed to import settings from %1, not a valid settings file.").arg(file),
			MessageWidget::Error);
		return;
	}

	loadSettings();
	emit settingsReset();
}

void ApplicationSettingsWidget::exportSettings()
{
	auto file = fileDialog()->getSaveFileName(this, tr("Export KeePassX-min Settings"), {}, "*.ini");
	if (file.isEmpty())
	{
		return;
	}

	config()->exportSettings(file);
}

void ApplicationSettingsWidget::autoSaveToggled(bool checked)
{
	// Explicitly enable other auto-save options
	if (checked)
	{
		m_generalUi->autoSaveOnExitCheckBox->setChecked(true);
		m_generalUi->autoSaveNonDataChangesCheckBox->setChecked(true);
	}

	m_generalUi->autoSaveOnExitCheckBox->setEnabled(!checked);
	m_generalUi->autoSaveNonDataChangesCheckBox->setEnabled(!checked);
}

void ApplicationSettingsWidget::hideWindowOnCopyCheckBoxToggled(bool checked)
{
	m_generalUi->minimizeOnCopyRadioButton->setEnabled(checked);
	m_generalUi->dropToBackgroundOnCopyRadioButton->setEnabled(checked);
}

void ApplicationSettingsWidget::systrayToggled(bool checked)
{
	m_generalUi->systrayMinimizeToTrayCheckBox->setEnabled(checked);
}

void ApplicationSettingsWidget::rememberDatabasesToggled(bool checked)
{
	if (!checked)
	{
		m_generalUi->rememberLastKeyFilesCheckBox->setChecked(false);
		m_generalUi->openPreviousDatabasesOnStartupCheckBox->setChecked(false);
	}

	m_generalUi->rememberLastDatabasesSpinbox->setEnabled(checked);
	m_generalUi->rememberLastKeyFilesCheckBox->setEnabled(checked);
	m_generalUi->openPreviousDatabasesOnStartupCheckBox->setEnabled(checked);
}

void ApplicationSettingsWidget::showExpiredEntriesOnDatabaseUnlockToggled(bool checked)
{
	m_generalUi->showExpiredEntriesOnDatabaseUnlockOffsetSpinBox->setEnabled(checked);
}

void ApplicationSettingsWidget::selectBackupDirectory()
{
	auto backupDirectory = FileDialog::instance()->getExistingDirectory(this, tr("Select backup storage directory"), QDir::homePath());
	if (!backupDirectory.isEmpty())
	{
		m_generalUi->backupFilePath->setText(QDir(backupDirectory).filePath(config()->getDefault(Config::BackupFilePathPattern).toString()));
	}
}
