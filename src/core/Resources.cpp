/*
 *  Copyright (C) 2020 KeePassXC Team <team@keepassxc.org>
 *  Copyright (C) 2011 Felix Geyer <debfx@fobos.de>
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

#include "Resources.h"

#include <QCoreApplication>
#include <QDir>
#include <QLibrary>

#include "config-keepassx.h"
#include "core/Config.h"
#include "core/Global.h"

Resources *Resources::m_instance(nullptr);

QString Resources::dataPath(const QString &name) const
{
	if (name.isEmpty() || name.startsWith('/'))
	{
		return m_dataPath + name;
	}
	return m_dataPath + "/" + name;
}

QString Resources::wordlistPath(const QString &name) const
{
	return dataPath(QStringLiteral("wordlists/%1").arg(name));
}

QString Resources::userWordlistPath(const QString &name) const
{
	QString configPath = QFileInfo(config()->getFileName()).absolutePath();
	return configPath + QStringLiteral("/wordlists/%1").arg(name);
}

Resources::Resources()
{
	const QString appDirPath = QCoreApplication::applicationDirPath();

	if (m_dataPath.isEmpty())
	{
		// Last ditch check if we are running from inside the src or test build directory
		trySetResourceDir(appDirPath + QStringLiteral("/../share"))
			|| trySetResourceDir(appDirPath + QStringLiteral("/../../share"));
	}

	if (m_dataPath.isEmpty())
	{
		qWarning("Resources::DataPath: can't find data dir");
	}
}

bool Resources::trySetResourceDir(const QString &path)
{
	QDir dir(path);
	if (dir.exists())
	{
		m_dataPath = dir.canonicalPath();
		return true;
	}
	return false;
}

Resources *Resources::instance()
{
	if (!m_instance)
	{
		m_instance = new Resources();
	}

	return m_instance;
}
