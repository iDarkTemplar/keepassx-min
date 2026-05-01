/*
 *  Copyright (C) 2026 i.Dark_Templar <darktemplar@dark-templar-archives.net>
 *  Copyright (C) 2023 KeePassXC Team <team@keepassxc.org>
 *  Copyright (C) 2014 Felix Geyer <debfx@fobos.de>
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

#include "URLEdit.h"

#include "core/Tools.h"
#include "gui/Icons.h"
#include "gui/ColorPalette.h"

#include <QUrl>

URLEdit::URLEdit(QWidget *parent)
	: QLineEdit(parent)
{
	const QIcon errorIcon = icons()->icon("dialog-error");
	m_errorAction = addAction(errorIcon, QLineEdit::TrailingPosition);
	m_errorAction->setVisible(false);
	m_errorAction->setToolTip(tr("Invalid URL"));

	updateStylesheet();
}

void URLEdit::enableVerifyMode()
{
	updateStylesheet();

	connect(this, &URLEdit::textChanged, this, &URLEdit::updateStylesheet);
}

void URLEdit::updateStylesheet()
{
	if (!QUrl(text()).isValid())
	{
		setStyleSheet(QStringLiteral("QLineEdit { background: %1; }").arg(ColorRole::Error));
		m_errorAction->setVisible(true);
	}
	else
	{
		m_errorAction->setVisible(false);
		setStyleSheet(QString());
	}
}
