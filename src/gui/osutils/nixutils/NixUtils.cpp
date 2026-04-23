/*
 * Copyright (C) 2020 KeePassXC Team <team@keepassxc.org>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, version 3.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "NixUtils.h"

#include "config-keepassx.h"
#include "core/Config.h"
#include "core/Global.h"

#include <QApplication>
#include <QDBusInterface>
#include <QDebug>
#include <QDir>
#include <QPointer>
#include <QRandomGenerator>
#include <QStandardPaths>
#include <QStyle>
#include <QTextStream>

#include <memory>

std::unique_ptr<NixUtils> NixUtils::m_instance;

NixUtils* NixUtils::instance()
{
	if (!m_instance)
	{
		m_instance.reset(new NixUtils(qApp));
	}

	return m_instance.get();
}

NixUtils::NixUtils(QObject *parent)
	: OSUtilsBase(parent)
{
	// notify about system color scheme changes
	QDBusConnection sessionBus = QDBusConnection::sessionBus();
	sessionBus.connect(
		"org.freedesktop.portal.Desktop",
		"/org/freedesktop/portal/desktop",
		"org.freedesktop.portal.Settings",
		"SettingChanged",
		this,
		SLOT(handleColorSchemeChanged(QString, QString, QDBusVariant)));

	QDBusMessage msg = QDBusMessage::createMethodCall("org.freedesktop.portal.Desktop", "/org/freedesktop/portal/desktop", "org.freedesktop.portal.Settings", "Read");
	msg << QVariant("org.freedesktop.appearance") << QVariant("color-scheme");
	sessionBus.callWithCallback(msg, this, SLOT(handleColorSchemeRead(QDBusVariant)));
}

bool NixUtils::isDarkMode() const
{
	// prefer freedesktop "org.freedesktop.appearance color-scheme" setting
	if (m_systemColorschemePrefExists)
	{
		return m_systemColorschemePref == ColorschemePref::PreferDark;
	}

	if (!qApp || !qApp->style())
	{
		return false;
	}

	return qApp->style()->standardPalette().color(QPalette::Window).toHsl().lightness() < 110;
}

void NixUtils::handleColorSchemeRead(QDBusVariant value)
{
	value = qvariant_cast<QDBusVariant>(value.variant());
	setColorScheme(value);
}

void NixUtils::handleColorSchemeChanged(QString ns, QString key, QDBusVariant value)
{
	if (ns == "org.freedesktop.appearance" && key == "color-scheme")
	{
		setColorScheme(value);
	}
}

void NixUtils::setColorScheme(QDBusVariant value)
{
	m_systemColorschemePref = static_cast<ColorschemePref>(value.variant().toInt());
	m_systemColorschemePrefExists = true;
	emit interfaceThemeChanged();
}
