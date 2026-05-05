/*
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

#include "ScreenLockListenerDBus.h"

#include <QDBusInterface>
#include <QDebug>
#include <QProcessEnvironment>

ScreenLockListenerDBus::ScreenLockListenerDBus(QWidget *parent)
	: ScreenLockListenerPrivate(parent)
{
	QDBusConnection sessionBus = QDBusConnection::sessionBus();
	QDBusConnection systemBus = QDBusConnection::systemBus();

	sessionBus.connect(
		QStringLiteral("org.freedesktop.ScreenSaver"), // service
		QStringLiteral("/org/freedesktop/ScreenSaver"), // path
		QStringLiteral("org.freedesktop.ScreenSaver"), // interface
		QStringLiteral("ActiveChanged"), // signal name
		this, // receiver
		SLOT(freedesktopScreenSaver(bool)));

	sessionBus.connect(
		QStringLiteral("org.gnome.ScreenSaver"), // service
		QStringLiteral("/org/gnome/ScreenSaver"), // path
		QStringLiteral("org.gnome.ScreenSaver"), // interface
		QStringLiteral("ActiveChanged"), // signal name
		this, // receiver
		SLOT(freedesktopScreenSaver(bool)));

	sessionBus.connect(
		QStringLiteral("org.gnome.SessionManager"), // service
		QStringLiteral("/org/gnome/SessionManager/Presence"), // path
		QStringLiteral("org.gnome.SessionManager.Presence"), // interface
		QStringLiteral("StatusChanged"), // signal name
		this, // receiver
		SLOT(gnomeSessionStatusChanged(uint)));

	sessionBus.connect(
		QStringLiteral("org.xfce.ScreenSaver"), // service
		QStringLiteral("/org/xfce/ScreenSaver"), // path
		QStringLiteral("org.xfce.ScreenSaver"), // interface
		QStringLiteral("ActiveChanged"), // signal name
		this, // receiver
		SLOT(freedesktopScreenSaver(bool)));

	systemBus.connect(
		QStringLiteral("org.freedesktop.login1"), // service
		QStringLiteral("/org/freedesktop/login1"), // path
		QStringLiteral("org.freedesktop.login1.Manager"), // interface
		QStringLiteral("PrepareForSleep"), // signal name
		this, // receiver
		SLOT(logindPrepareForSleep(bool)));

	QString sessionId = QProcessEnvironment::systemEnvironment().value(QStringLiteral("XDG_SESSION_ID"));
	QDBusInterface loginManager(
		QStringLiteral("org.freedesktop.login1"), // service
		QStringLiteral("/org/freedesktop/login1"), // path
		QStringLiteral("org.freedesktop.login1.Manager"), // interface
		systemBus);

	if (loginManager.isValid())
	{
		QList<QVariant> args = {sessionId};
		loginManager.callWithCallback(QStringLiteral("GetSession"), args, this, SLOT(login1SessionObjectReceived(QDBusMessage)));
	}

	sessionBus.connect(
		QStringLiteral("com.canonical.Unity"), // service
		QStringLiteral("/com/canonical/Unity/Session"), // path
		QStringLiteral("com.canonical.Unity.Session"), // interface
		QStringLiteral("Locked"), // signal name
		this, // receiver
		SLOT(unityLocked()));
}

void ScreenLockListenerDBus::login1SessionObjectReceived(QDBusMessage response)
{
	if (response.arguments().isEmpty())
	{
		qDebug() << tr("org.freedesktop.login1.Manager.GetSession did not return results");
		return;
	}

	QVariant arg0 = response.arguments().at(0);
	if (!arg0.canConvert<QDBusObjectPath>())
	{
		qDebug() << tr("org.freedesktop.login1.Manager.GetSession did not return a QDBusObjectPath");
		return;
	}

	QDBusObjectPath path = arg0.value<QDBusObjectPath>();
	QDBusConnection systemBus = QDBusConnection::systemBus();

	systemBus.connect(
		QString(), // service
		path.path(), // path
		QStringLiteral("org.freedesktop.login1.Session"), // interface
		QStringLiteral("Lock"), // signal name
		this, // receiver
		SLOT(unityLocked()));
}

void ScreenLockListenerDBus::gnomeSessionStatusChanged(uint status)
{
	if (status != 0)
	{
		Q_EMIT screenLocked();
	}
}

void ScreenLockListenerDBus::logindPrepareForSleep(bool beforeSleep)
{
	if (beforeSleep)
	{
		Q_EMIT screenLocked();
	}
}

void ScreenLockListenerDBus::unityLocked()
{
	Q_EMIT screenLocked();
}

void ScreenLockListenerDBus::freedesktopScreenSaver(bool status)
{
	if (status)
	{
		Q_EMIT screenLocked();
	}
}
