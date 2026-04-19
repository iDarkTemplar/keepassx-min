/*
 * Copyright (C) 2021 KeePassXC Team <team@keepassxc.org>
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

#ifndef KEEPASSXC_OSUTILSBASE_H
#define KEEPASSXC_OSUTILSBASE_H

#include <QObject>

class QWindow;

/**
 * Abstract base class for generic OS-specific functionality
 * which can be reasonably expected to be available on all platforms.
 */
class OSUtilsBase: public QObject
{
	Q_OBJECT

public:
	/**
	 * @return OS dark mode enabled.
	 */
	virtual bool isDarkMode() const = 0;

	/**
	 * @return OS caps lock enabled.
	 */
	virtual bool isCapslockEnabled() = 0;

	/**
	 * @param enable Toggle protection on user input (if available).
	 */
	virtual void setUserInputProtection(bool enable) = 0;

	virtual void registerNativeEventFilter() = 0;

	virtual bool registerGlobalShortcut(const QString &name,
	                                    Qt::Key key,
	                                    Qt::KeyboardModifiers modifiers,
	                                    QString *error = nullptr) = 0;
	virtual bool unregisterGlobalShortcut(const QString &name) = 0;

signals:
	void globalShortcutTriggered(const QString &name, const QString &search = {});

	/**
	 * Indicates platform UI theme change (light mode to dark mode).
	 */
	void interfaceThemeChanged();

	/*
	 * Indicates a change in the tray / statusbar theme.
	 */
	void statusbarThemeChanged();

protected:
	explicit OSUtilsBase(QObject *parent = nullptr);
	virtual ~OSUtilsBase();
};

#endif // KEEPASSXC_OSUTILSBASE_H
