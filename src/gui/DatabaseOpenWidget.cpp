/*
 *  Copyright (C) 2026 i.Dark_Templar <darktemplar@dark-templar-archives.net>
 *  Copyright (C) 2011 Felix Geyer <debfx@fobos.de>
 *  Copyright (C) 2017 KeePassXC Team <team@keepassxc.org>
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

#include "DatabaseOpenWidget.h"
#include "ui_DatabaseOpenWidget.h"

#include "gui/FileDialog.h"
#include "gui/Icons.h"
#include "gui/MainWindow.h"
#include "gui/MessageBox.h"
#include "keys/FileKey.h"

#include <QCheckBox>
#include <QCloseEvent>
#include <QDesktopServices>
#include <QFont>

namespace {

constexpr int clearFormsDelay = 30000;

} // namespace

DatabaseOpenWidget::DatabaseOpenWidget(QWidget *parent)
	: DialogyWidget(parent)
	, m_ui(new Ui::DatabaseOpenWidget())
	, m_db(nullptr)
{
	m_ui->setupUi(this);

	m_ui->messageWidget->setHidden(true);

	m_hideTimer.setInterval(clearFormsDelay);
	m_hideTimer.setSingleShot(true);
	connect(&m_hideTimer, &QTimer::timeout, this, [this] {
		// Reset the password field after being hidden for a set time
		m_ui->editPassword->setText(QString());
		m_ui->editPassword->setShowPassword(false);
	});

	QFont font;
	font.setPointSize(font.pointSize() + 4);
	font.setBold(true);
	m_ui->labelHeadline->setFont(font);

	connect(m_ui->buttonBrowseFile, &QPushButton::clicked, this, &DatabaseOpenWidget::browseKeyFile);

	auto okBtn = m_ui->buttonBox->button(QDialogButtonBox::Ok);
	okBtn->setText(tr("Unlock"));
	okBtn->setDefault(true);
	connect(m_ui->buttonBox, &QDialogButtonBox::accepted, this, &DatabaseOpenWidget::openDatabase);
	connect(m_ui->buttonBox, &QDialogButtonBox::rejected, this, &DatabaseOpenWidget::reject);

	connect(m_ui->addKeyFileLinkLabel, &QLabel::linkActivated, this, &DatabaseOpenWidget::browseKeyFile);
	connect(m_ui->keyFileLineEdit, &PasswordWidget::textChanged, this, [&](const QString &text) {
		bool state = !text.isEmpty();
		m_ui->addKeyFileLinkLabel->setVisible(!state);
		m_ui->selectKeyFileComponent->setVisible(state);
	});

	m_ui->selectKeyFileComponent->setVisible(false);
}

DatabaseOpenWidget::~DatabaseOpenWidget()
{
}

void DatabaseOpenWidget::closeDatabase()
{
	int closeWarningInterval = 3000;

	if (!m_triedToQuit && window() == getMainWindow())
	{
		m_triedToQuit = true;
		m_ui->messageWidget->showMessage(
			tr("Press ESC again to close this database"), MessageWidget::Warning, closeWarningInterval);

		QTimer::singleShot(closeWarningInterval, this, [this]() { m_triedToQuit = false; });
		return;
	}

	reject();
}

void DatabaseOpenWidget::keyPressEvent(QKeyEvent *event)
{
	if (event->key() == Qt::Key_Escape)
	{
		closeDatabase();
	}
	else
	{
		DialogyWidget::keyPressEvent(event);
	}
}

bool DatabaseOpenWidget::event(QEvent *event)
{
	bool ret = DialogyWidget::event(event);
	auto type = event->type();

	if (type == QEvent::Show || type == QEvent::WindowActivate)
	{
		if (isVisible())
		{
			m_hideTimer.stop();
		}

		ret = true;
	}
	else if (type == QEvent::Hide || type == QEvent::WindowDeactivate)
	{
		// Schedule form clearing if we are hidden
		if (!m_hideTimer.isActive())
		{
			m_hideTimer.start();
		}

		ret = true;
	}

	return ret;
}

bool DatabaseOpenWidget::unlockingDatabase()
{
	return m_unlockingDatabase;
}

void DatabaseOpenWidget::showMessage(const QString &text, MessageWidget::MessageType type, int autoHideTimeout)
{
	m_ui->messageWidget->showMessage(text, type, autoHideTimeout);
}

void DatabaseOpenWidget::load(const QString &filename)
{
	clearForms();

	m_filename = filename;

	// Read public headers
	QString error;
	m_db.reset(new Database());
	m_db->open(m_filename, nullptr, &error);

	m_ui->fileNameLabel->setRawText(m_filename);

	// Set the public name if defined
	auto label = tr("Unlock KeePassX-min Database");
	if (!m_db->publicName().isEmpty())
	{
		label.append(QStringLiteral(": %1").arg(m_db->publicName()));
	}

	m_ui->labelHeadline->setText(label);

	// Apply the public color to the central unlock stack if defined
	auto color = m_db->publicColor();
	if (!color.isEmpty())
	{
		m_ui->centralStack->setStyleSheet(QStringLiteral("QStackedWidget {border: 4px solid %1}").arg(color));
	}
	else
	{
		m_ui->centralStack->setStyleSheet(QString());
	}

	// Show the database icon if defined
	auto iconIndex = m_db->publicIcon();
	if (iconIndex >= 0 && iconIndex < databaseIcons()->count())
	{
		m_ui->dbIconLabel->setPixmap(databaseIcons()->icon(iconIndex, IconSize::Large));
		m_ui->dbIconLabel->setVisible(true);
	}
	else
	{
		m_ui->dbIconLabel->setPixmap({});
		m_ui->dbIconLabel->setVisible(false);
	}

	if (config()->get(Config::RememberLastKeyFiles).toBool())
	{
		auto lastKeyFiles = config()->get(Config::LastKeyFiles).toHash();
		if (lastKeyFiles.contains(m_filename))
		{
			m_ui->keyFileLineEdit->setText(lastKeyFiles[m_filename].toString());
		}
	}
}

void DatabaseOpenWidget::clearForms()
{
	setUserInteractionLock(false);
	m_ui->editPassword->setText(QString());
	m_ui->editPassword->setShowPassword(false);
	m_ui->keyFileLineEdit->clear();
	m_ui->keyFileLineEdit->setShowPassword(false);
	m_ui->keyFileLineEdit->setClearButtonEnabled(true);

	QString error;
	m_db.reset(new Database());
	m_db->open(m_filename, nullptr, &error);
}

QSharedPointer<Database> DatabaseOpenWidget::database()
{
	return m_db;
}

QString DatabaseOpenWidget::filename()
{
	return m_filename;
}

void DatabaseOpenWidget::enterKey(const QString &pw, const QString &keyFile)
{
	if (unlockingDatabase())
	{
		qWarning() << tr("Ignoring unlock request for %1 because of running unlock action.").arg(m_filename);
		return;
	}

	m_ui->editPassword->setText(pw);
	m_ui->keyFileLineEdit->setText(keyFile);

	openDatabase();
}

void DatabaseOpenWidget::openDatabase()
{
	setUserInteractionLock(true);
	m_ui->editPassword->setShowPassword(false);
	m_ui->messageWidget->hide();
	QCoreApplication::processEvents();

	const auto databaseKey = buildDatabaseKey();
	if (!databaseKey)
	{
		setUserInteractionLock(false);
		return;
	}

	QString error;
	m_db.reset(new Database());
	bool ok = m_db->open(m_filename, databaseKey, &error);

	if (ok)
	{
		// Warn user about minor version mismatch to halt loading if necessary
		if (m_db->hasMinorVersionMismatch())
		{
			QScopedPointer<QMessageBox> msgBox(new QMessageBox(this));
			msgBox->setIcon(QMessageBox::Warning);
			msgBox->setWindowTitle(tr("Database Version Mismatch"));
			msgBox->setText(tr("The database you are trying to open was most likely\n"
				"created by a newer version of KeePassX-min.\n\n"
				"You can try to open it anyway, but it may be incomplete\n"
				"and saving any changes may incur data loss.\n\n"
				"We recommend you update your KeePassX-min installation."));

			auto btn = msgBox->addButton(tr("Open database anyway"), QMessageBox::ButtonRole::AcceptRole);
			msgBox->setDefaultButton(btn);
			msgBox->addButton(QMessageBox::Cancel);
			msgBox->layout()->setSizeConstraint(QLayout::SetMinimumSize);
			msgBox->exec();
			if (msgBox->clickedButton() != btn)
			{
				m_db.reset(new Database());
				m_db->open(m_filename, nullptr, &error);

				m_ui->messageWidget->showMessage(tr("Database unlock canceled."), MessageWidget::MessageType::Error);
				setUserInteractionLock(false);
				return;
			}
		}

		Q_EMIT dialogFinished(true);
		clearForms();
	}
	else
	{
		if (m_ui->editPassword->text().isEmpty() && !m_retryUnlockWithEmptyPassword)
		{
			QScopedPointer<QMessageBox> msgBox(new QMessageBox(this));
			msgBox->setIcon(QMessageBox::Critical);
			msgBox->setWindowTitle(tr("Unlock failed and no password given"));
			msgBox->setText(tr("Unlocking the database failed and you did not enter a password.\n"
				"Do you want to retry with an \"empty\" password instead?\n\n"
				"To prevent this error from appearing, you must go to "
				"\"Database Settings / Security\" and reset your password."));

			auto btn = msgBox->addButton(tr("Retry with empty password"), QMessageBox::ButtonRole::AcceptRole);
			msgBox->setDefaultButton(btn);
			msgBox->addButton(QMessageBox::Cancel);
			msgBox->exec();

			if (msgBox->clickedButton() == btn)
			{
				m_retryUnlockWithEmptyPassword = true;
				setUserInteractionLock(false);
				openDatabase();
				return;
			}
		}

		setUserInteractionLock(false);

		m_retryUnlockWithEmptyPassword = false;
		m_ui->messageWidget->showMessage(error, MessageWidget::MessageType::Error);

		// Focus on the password field and select the input for easy retry
		m_ui->editPassword->selectAll();
		m_ui->editPassword->setFocus();
	}
}

QSharedPointer<CompositeKey> DatabaseOpenWidget::buildDatabaseKey()
{
	auto databaseKey = QSharedPointer<CompositeKey>::create();

	if (!m_ui->editPassword->text().isEmpty() || m_retryUnlockWithEmptyPassword)
	{
		databaseKey->addKey(QSharedPointer<PasswordKey>::create(m_ui->editPassword->text()));
	}

	auto lastKeyFiles = config()->get(Config::LastKeyFiles).toHash();
	lastKeyFiles.remove(m_filename);

	auto key = QSharedPointer<FileKey>::create();
	QString keyFilename = m_ui->keyFileLineEdit->text();
	if (!keyFilename.isEmpty())
	{
		QString errorMsg;
		if (!key->load(keyFilename, &errorMsg))
		{
			m_ui->messageWidget->showMessage(tr("Failed to open key file: %1").arg(errorMsg), MessageWidget::Error);
			return {};
		}

		if (key->type() != FileKey::KeePass2XMLv2
			&& key->type() != FileKey::Hashed
			&& !config()->get(Config::Messages_NoLegacyKeyFileWarning).toBool())
		{
			QMessageBox legacyWarning;
			legacyWarning.setWindowTitle(tr("Old key file format"));
			legacyWarning.setText(tr("You are using an old key file format which KeePassX-min may<br>"
				"stop supporting in the future.<br><br>"
				"Please consider generating a new key file by going to:<br>"
				"<strong>Database &gt; Database Security &gt; Change Key File.</strong><br>"));

			legacyWarning.setIcon(QMessageBox::Icon::Warning);
			legacyWarning.addButton(QMessageBox::Ok);
			legacyWarning.setDefaultButton(QMessageBox::Ok);
			legacyWarning.setCheckBox(new QCheckBox(tr("Don't show this warning again")));

			connect(legacyWarning.checkBox(), &QCheckBox::stateChanged, this, [](int state) {
				config()->set(Config::Messages_NoLegacyKeyFileWarning, state == Qt::CheckState::Checked);
			});

			legacyWarning.exec();
		}
		databaseKey->addKey(key);
		lastKeyFiles.insert(m_filename, keyFilename);
	}

	if (config()->get(Config::RememberLastKeyFiles).toBool())
	{
		config()->set(Config::LastKeyFiles, lastKeyFiles);
	}

	return databaseKey;
}

void DatabaseOpenWidget::reject()
{
	Q_EMIT dialogFinished(false);
}

bool DatabaseOpenWidget::browseKeyFile()
{
	QString filters = QStringLiteral("%1 (*);;%2 (*.keyx; *.key)").arg(tr("All files"), tr("Key files"));
	QString filename = fileDialog()->getOpenFileName(this, tr("Select key file"), FileDialog::getLastDir(QStringLiteral("keyfile")), filters);
	if (filename.isEmpty())
	{
		return false;
	}

	if (config()->get(Config::RememberLastKeyFiles).toBool())
	{
		FileDialog::saveLastDir(QStringLiteral("keyfile"), filename, true);
	}
	else
	{
		FileDialog::saveLastDir(QStringLiteral("keyfile"), {});
	}

	if (QFileInfo(filename).canonicalFilePath() == QFileInfo(m_filename).canonicalFilePath())
	{
		MessageBox::warning(this,
			tr("Cannot use database file as key file"),
			tr("Your database file is NOT a key file!\nIf you don't have a key file or don't know what "
				"that is, you don't have to select one."),
			MessageBox::Button::Ok);

		return false;
	}

	if (filename.endsWith(QStringLiteral(".kdbxm")) && MessageBox::warning(this,
		tr("KeePassX-min database file selected"),
		tr("The file you selected looks like a database file.\nA database file is NOT a key "
			"file!\n\nAre you sure you want to continue with this file?."),
		MessageBox::Button::Yes | MessageBox::Button::Cancel,
		MessageBox::Button::Cancel)
		!= MessageBox::Yes)
	{
		return false;
	}

	m_ui->keyFileLineEdit->setText(filename);
	return true;
}

void DatabaseOpenWidget::setUserInteractionLock(bool state)
{
	if (state)
	{
		QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
		m_ui->centralStack->setEnabled(false);
	}
	else
	{
		// Ensure no override cursors remain
		while (QApplication::overrideCursor())
		{
			QApplication::restoreOverrideCursor();
		}
		m_ui->centralStack->setEnabled(true);
	}

	m_unlockingDatabase = state;
}
