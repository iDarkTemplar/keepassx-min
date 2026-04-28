/*
 *  Copyright (C) 2026 i.Dark_Templar <darktemplar@dark-templar-archives.net>
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

#include "core/Config.h"
#include "core/Global.h"

std::unique_ptr<Resources> Resources::m_instance;

QString Resources::dataPath(const QString &name) const
{
	if (name.isEmpty() || name.startsWith('/'))
	{
		return m_dataPath + name;
	}

	return m_dataPath + QStringLiteral("/") + name;
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
		trySetResourceDir(appDirPath + QStringLiteral("/../share/keepassxmin"));
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

Resources* Resources::instance()
{
	if (!m_instance)
	{
		m_instance.reset(new Resources());
	}

	return m_instance.get();
}
