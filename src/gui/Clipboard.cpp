/*
 *  Copyright (C) 2026 i.Dark_Templar <darktemplar@dark-templar-archives.net>
 *  Copyright (C) 2017 KeePassXC Team <team@keepassxc.org>
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

#include "Clipboard.h"

#include <QApplication>
#include <QClipboard>
#include <QMimeData>
#include <QProcess>
#include <QTimer>

#include "core/Config.h"

std::unique_ptr<Clipboard> Clipboard::m_instance;

Clipboard::Clipboard(QObject *parent)
	: QObject(parent)
	, m_timer(new QTimer(this))
{
	connect(m_timer, &QTimer::timeout, this, &Clipboard::countdownTick);
	connect(qApp, &QApplication::aboutToQuit, this, &Clipboard::clearCopiedText);
}

void Clipboard::setText(const QString &text, bool clear)
{
	auto *clipboard = QApplication::clipboard();
	if (!clipboard)
	{
		qWarning("Unable to access the clipboard.");
		return;
	}

	auto *mime = new QMimeData;
	mime->setText(text);
	mime->setData("x-kde-passwordManagerHint", QByteArrayLiteral("secret"));

	if (clipboard->supportsSelection())
	{
		clipboard->setMimeData(mime, QClipboard::Selection);
	}

	clipboard->setMimeData(mime, QClipboard::Clipboard);

	if (clear)
	{
		m_lastCopied = text;
		if (config()->get(Config::Security_ClearClipboard).toBool())
		{
			int timeout = config()->get(Config::Security_ClearClipboardTimeout).toInt();
			if (timeout > 0)
			{
				m_secondsToClear = timeout;
				sendCountdownStatus();
				m_timer->start(1000);
			}
			else
			{
				clearCopiedText();
			}
		}
	}
}

int Clipboard::secondsToClear()
{
	return m_secondsToClear;
}

void Clipboard::clearCopiedText()
{
	m_timer->stop();
	emit updateCountdown(-1, "");

	auto *clipboard = QApplication::clipboard();
	if (!clipboard)
	{
		qWarning("Unable to access the clipboard.");
		return;
	}

	if (!m_lastCopied.isEmpty()
		&& (m_lastCopied == clipboard->text(QClipboard::Clipboard) || m_lastCopied == clipboard->text(QClipboard::Selection)))
	{
		clipboard->clear(QClipboard::Clipboard);
		clipboard->clear(QClipboard::Selection);

		// Gnome Wayland doesn't let apps modify the clipboard when not in focus, so force clear
		if (QProcessEnvironment::systemEnvironment().contains("WAYLAND_DISPLAY"))
		{
			QProcess::startDetached("wl-copy", {"-c"});
		}
	}

	m_lastCopied.clear();
}

void Clipboard::countdownTick()
{
	if (--m_secondsToClear <= 0)
	{
		clearCopiedText();
	}
	else
	{
		sendCountdownStatus();
	}
}

void Clipboard::sendCountdownStatus()
{
	emit updateCountdown(
		100 * m_secondsToClear / config()->get(Config::Security_ClearClipboardTimeout).toInt(),
		QObject::tr("Clearing the clipboard in %1 second(s)…", "", m_secondsToClear).arg(m_secondsToClear));
}

Clipboard* Clipboard::instance()
{
	if (!m_instance)
	{
		m_instance.reset(new Clipboard);
	}

	return m_instance.get();
}
