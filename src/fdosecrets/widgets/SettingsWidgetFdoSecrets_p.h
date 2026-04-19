/*
 *  Copyright (C) 2026 i.Dark_Templar <darktemplar@dark-templar-archives.net>
 *  Copyright (C) 2018 Aetf <aetf@unlimitedcodeworks.xyz>
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

#ifndef KEEPASSXM_SETTINGSWIDGETFDOSECRETS_PRIVATE_H
#define KEEPASSXM_SETTINGSWIDGETFDOSECRETS_PRIVATE_H

#include "fdosecrets/FdoSecretsPlugin.h"
#include "fdosecrets/dbus/DBusClient.h"
#include "gui/DatabaseWidget.h"

#include <QWidget>
#include <QPointer>

using FdoSecrets::DBusClientPtr;

class FdoSecretsManageDatabase: public QWidget
{
	Q_OBJECT

	Q_PROPERTY(DatabaseWidget *dbWidget READ dbWidget WRITE setDbWidget USER true)

public:
	explicit FdoSecretsManageDatabase(FdoSecretsPlugin *plugin, QWidget *parent = nullptr);

	DatabaseWidget* dbWidget() const;
	void setDbWidget(DatabaseWidget *dbWidget);

private:
	void disconnect();
	void reconnect();

private:
	FdoSecretsPlugin *m_plugin = nullptr;
	QPointer<DatabaseWidget> m_dbWidget = nullptr;
	QAction *m_dbSettingsAct = nullptr;
	QAction *m_lockAct = nullptr;
};

class FdoSecretsManageSession: public QWidget
{
	Q_OBJECT

	Q_PROPERTY(const DBusClientPtr &client READ client WRITE setClient USER true)

public:
	explicit FdoSecretsManageSession(QWidget *parent = nullptr);

	const DBusClientPtr &client() const;
	void setClient(DBusClientPtr client);

private:
	DBusClientPtr m_client{};
};

#endif // KEEPASSXM_SETTINGSWIDGETFDOSECRETS_PRIVATE_H
