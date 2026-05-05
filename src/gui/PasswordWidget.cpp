/*
 *  Copyright (C) 2026 i.Dark_Templar <darktemplar@dark-templar-archives.net>
 *  Copyright (C) 2014 Felix Geyer <debfx@fobos.de>
 *  Copyright (C) 2020 KeePassXC Team <team@keepassxc.org>
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

#include "PasswordWidget.h"
#include "ui_PasswordWidget.h"

#include "core/Config.h"
#include "core/PasswordHealth.h"
#include "gui/ColorPalette.h"
#include "gui/Font.h"
#include "gui/Icons.h"
#include "gui/PasswordGeneratorWidget.h"

#include <QEvent>
#include <QLineEdit>
#include <QTimer>
#include <QToolTip>

PasswordWidget::PasswordWidget(QWidget *parent)
	: QWidget(parent)
	, m_ui(new Ui::PasswordWidget())
{
	m_ui->setupUi(this);
	setFocusProxy(m_ui->passwordEdit);
	m_ui->passwordEdit->installEventFilter(this);

	const QIcon errorIcon = icons()->icon(QStringLiteral("dialog-error"));
	m_errorAction = m_ui->passwordEdit->addAction(errorIcon, QLineEdit::TrailingPosition);
	m_errorAction->setVisible(false);
	m_errorAction->setToolTip(tr("Passwords do not match"));

	const QIcon correctIcon = icons()->icon(QStringLiteral("dialog-ok"));
	m_correctAction = m_ui->passwordEdit->addAction(correctIcon, QLineEdit::TrailingPosition);
	m_correctAction->setVisible(false);
	m_correctAction->setToolTip(tr("Passwords match so far"));

	setEchoMode(QLineEdit::Password);

	// use a monospace font for the password field
	QFont passwordFont = Font::fixedFont();
	passwordFont.setLetterSpacing(QFont::PercentageSpacing, 110);
	m_ui->passwordEdit->setFont(passwordFont);

	m_toggleVisibleAction = new QAction(
		icons()->onOffIcon(QStringLiteral("password-show"), false),
		tr("Toggle Password (%1)").arg(QKeySequence(Qt::ControlModifier | Qt::Key_H).toString(QKeySequence::NativeText)),
		this);
	m_toggleVisibleAction->setCheckable(true);
	m_toggleVisibleAction->setShortcut(Qt::ControlModifier | Qt::Key_H);
	m_toggleVisibleAction->setShortcutContext(Qt::WidgetShortcut);
	m_ui->passwordEdit->addAction(m_toggleVisibleAction, QLineEdit::TrailingPosition);
	connect(m_toggleVisibleAction, &QAction::triggered, this, &PasswordWidget::setShowPassword);

	m_passwordGeneratorAction = new QAction(
		icons()->icon(QStringLiteral("password-generator")),
		tr("Generate Password (%1)").arg(QKeySequence(Qt::ControlModifier | Qt::Key_G).toString(QKeySequence::NativeText)),
		this);

	m_passwordGeneratorAction->setShortcut(Qt::ControlModifier | Qt::Key_G);
	m_passwordGeneratorAction->setShortcutContext(Qt::WidgetShortcut);
	m_ui->passwordEdit->addAction(m_passwordGeneratorAction, QLineEdit::TrailingPosition);
	m_passwordGeneratorAction->setVisible(false);

	// Reset the password strength bar, hidden by default
	updatePasswordStrength(QString());
	m_ui->qualityProgressBar->setVisible(false);

	connect(m_ui->passwordEdit, &QLineEdit::textChanged, this, [this](const QString &pwd) {
		updatePasswordStrength(pwd);
		Q_EMIT textChanged(pwd);
	});
}

PasswordWidget::~PasswordWidget()
{
}

void PasswordWidget::setQualityVisible(bool state)
{
	m_ui->qualityProgressBar->setVisible(state);
}

QString PasswordWidget::text()
{
	return m_ui->passwordEdit->text();
}

void PasswordWidget::setText(const QString &text)
{
	m_ui->passwordEdit->setText(text);
}

void PasswordWidget::setEchoMode(QLineEdit::EchoMode mode)
{
	m_ui->passwordEdit->setEchoMode(mode);
}

void PasswordWidget::clear()
{
	m_ui->passwordEdit->clear();
}

void PasswordWidget::setClearButtonEnabled(bool enabled)
{
	m_ui->passwordEdit->setClearButtonEnabled(enabled);
}

void PasswordWidget::selectAll()
{
	m_ui->passwordEdit->selectAll();
}

void PasswordWidget::setReadOnly(bool state)
{
	m_ui->passwordEdit->setReadOnly(state);
}

void PasswordWidget::setRepeatPartner(PasswordWidget *repeatPartner)
{
	m_repeatPasswordWidget = repeatPartner;
	m_repeatPasswordWidget->setParentPasswordEdit(this);

	connect(m_ui->passwordEdit, &QLineEdit::textChanged, m_repeatPasswordWidget, &PasswordWidget::updateRepeatStatus);
}

void PasswordWidget::setParentPasswordEdit(PasswordWidget *parent)
{
	m_parentPasswordWidget = parent;
	// Hide actions
	m_toggleVisibleAction->setVisible(false);
	m_passwordGeneratorAction->setVisible(false);
	connect(m_ui->passwordEdit, &QLineEdit::textChanged, this, &PasswordWidget::updateRepeatStatus);
}

void PasswordWidget::enablePasswordGenerator()
{
	if (!m_passwordGeneratorAction->isVisible())
	{
		m_passwordGeneratorAction->setVisible(true);
		connect(m_passwordGeneratorAction, &QAction::triggered, this, &PasswordWidget::popupPasswordGenerator);
	}
}

void PasswordWidget::setShowPassword(bool show)
{
	setEchoMode(show ? QLineEdit::Normal : QLineEdit::Password);
	m_toggleVisibleAction->setIcon(icons()->onOffIcon(QStringLiteral("password-show"), show));
	m_toggleVisibleAction->setChecked(show);

	if (m_repeatPasswordWidget)
	{
		m_repeatPasswordWidget->setEchoMode(show ? QLineEdit::Normal : QLineEdit::Password);
	}
}

bool PasswordWidget::isPasswordVisible() const
{
	return m_ui->passwordEdit->echoMode() == QLineEdit::Normal;
}

void PasswordWidget::popupPasswordGenerator()
{
	auto generator = PasswordGeneratorWidget::popupGenerator(this);
	generator->setPasswordVisible(isPasswordVisible());
	generator->setPasswordLength(text().length());

	connect(generator, &PasswordGeneratorWidget::appliedPassword, this, &PasswordWidget::setText);
	if (m_repeatPasswordWidget)
	{
		connect(generator, &PasswordGeneratorWidget::appliedPassword, m_repeatPasswordWidget, &PasswordWidget::setText);
	}
}

void PasswordWidget::updateRepeatStatus()
{
	static const auto stylesheetTemplate = QStringLiteral("QLineEdit { background: %1; }");
	if (!m_parentPasswordWidget)
	{
		return;
	}

	const auto otherPassword = m_parentPasswordWidget->text();
	const auto password = text();
	if (otherPassword != password)
	{
		bool isCorrect = false;
		QColor color = QColor::fromString(ColorRole::Error);
		if (!password.isEmpty() && otherPassword.startsWith(password))
		{
			color = QColor::fromString(ColorRole::Incomplete);
			isCorrect = true;
		}

		setStyleSheet(stylesheetTemplate.arg(color.name()));
		m_correctAction->setVisible(isCorrect);
		m_errorAction->setVisible(!isCorrect);
	}
	else
	{
		m_correctAction->setVisible(false);
		m_errorAction->setVisible(false);
		setStyleSheet(QString());
	}
}

void PasswordWidget::updatePasswordStrength(const QString &password)
{
	if (password.isEmpty())
	{
		m_ui->qualityProgressBar->setValue(0);
		m_ui->qualityProgressBar->setToolTip((tr("")));
		return;
	}

	PasswordHealth health(password);

	m_ui->qualityProgressBar->setValue(std::min(int(health.entropy()), m_ui->qualityProgressBar->maximum()));

	QString style = m_ui->qualityProgressBar->styleSheet();
	QRegularExpression re(QStringLiteral("(QProgressBar::chunk\\s*\\{.*?background-color:)[^;]+;"),
		QRegularExpression::CaseInsensitiveOption | QRegularExpression::DotMatchesEverythingOption);

	style.replace(re, QStringLiteral("\\1 %1;"));

	switch (health.quality())
	{
	case PasswordHealth::Quality::Bad:
	case PasswordHealth::Quality::Poor:
		m_ui->qualityProgressBar->setStyleSheet(style.arg(ColorRole::HealthCritical));
		m_ui->qualityProgressBar->setToolTip(tr("Quality: %1").arg(tr("Poor", "Password quality")));
		break;

	case PasswordHealth::Quality::Weak:
		m_ui->qualityProgressBar->setStyleSheet(style.arg(ColorRole::HealthBad));
		m_ui->qualityProgressBar->setToolTip(tr("Quality: %1").arg(tr("Weak", "Password quality")));
		break;

	case PasswordHealth::Quality::Good:
		m_ui->qualityProgressBar->setStyleSheet(style.arg(ColorRole::HealthOk));
		m_ui->qualityProgressBar->setToolTip(tr("Quality: %1").arg(tr("Good", "Password quality")));
		break;

	case PasswordHealth::Quality::Excellent:
		m_ui->qualityProgressBar->setStyleSheet(style.arg(ColorRole::HealthExcellent));
		m_ui->qualityProgressBar->setToolTip(tr("Quality: %1").arg(tr("Excellent", "Password quality")));
		break;
	}
}
