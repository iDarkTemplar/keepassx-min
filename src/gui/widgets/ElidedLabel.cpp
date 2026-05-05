/*
 *  Copyright (C) 2026 i.Dark_Templar <darktemplar@dark-templar-archives.net>
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

#include "ElidedLabel.h"

namespace {

const QString htmlLinkTemplate = QStringLiteral("<a href=\"%1\">%2</a>");

} // namespace

ElidedLabel::ElidedLabel(QWidget *parent, Qt::WindowFlags f)
	: QLabel(parent, f)
	, m_elideMode(Qt::ElideMiddle)
{
	connect(this, &ElidedLabel::elideModeChanged, this, &ElidedLabel::updateElidedText);
	connect(this, &ElidedLabel::rawTextChanged, this, &ElidedLabel::updateElidedText);
	connect(this, &ElidedLabel::urlChanged, this, &ElidedLabel::updateElidedText);
}

ElidedLabel::ElidedLabel(const QString &text, QWidget *parent, Qt::WindowFlags f)
	: ElidedLabel(parent, f)
{
	setText(text);
}

Qt::TextElideMode ElidedLabel::elideMode() const
{
	return m_elideMode;
}

QString ElidedLabel::rawText() const
{
	return m_rawText;
}

QString ElidedLabel::url() const
{
	return m_url;
}

void ElidedLabel::setElideMode(Qt::TextElideMode elideMode)
{
	if (m_elideMode == elideMode)
	{
		return;
	}

	if (m_elideMode != Qt::ElideNone)
	{
		setWordWrap(false);
	}

	m_elideMode = elideMode;
	Q_EMIT elideModeChanged(m_elideMode);
}

void ElidedLabel::setRawText(const QString &elidedText)
{
	if (m_rawText == elidedText)
	{
		return;
	}

	m_rawText = elidedText;
	Q_EMIT rawTextChanged(m_rawText);
}

void ElidedLabel::setUrl(const QString &url)
{
	if (m_url == url)
	{
		return;
	}

	m_url = url;
	Q_EMIT urlChanged(m_url);
}

void ElidedLabel::clear()
{
	setRawText(QString());
	setElideMode(Qt::ElideMiddle);
	setUrl(QString());
	QLabel::clear();
}

void ElidedLabel::updateElidedText()
{
	if (m_rawText.isEmpty())
	{
		QLabel::clear();
		return;
	}

	QString displayText = m_rawText;
	if (m_elideMode != Qt::ElideNone)
	{
		const QFontMetrics metrix(font());
		displayText = metrix.elidedText(m_rawText, m_elideMode, width() - 2);
	}

	bool hasUrl = !m_url.isEmpty();
	setText(hasUrl ? htmlLinkTemplate.arg(m_url.toHtmlEscaped(), displayText) : displayText);
	setOpenExternalLinks(false);
}

void ElidedLabel::resizeEvent(QResizeEvent *event)
{
	updateElidedText();
	QLabel::resizeEvent(event);
}
