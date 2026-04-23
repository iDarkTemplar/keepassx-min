/*
 *  Copyright (C) 2026 i.Dark_Templar <darktemplar@dark-templar-archives.net>
 *  Copyright (C) 2019 Aetf <aetf@unlimitedcodeworks.xyz>
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

#ifndef KEEPASSXM_DATABASESETTINGSWIDGETFDOSECRETS_PRIVATE_H
#define KEEPASSXM_DATABASESETTINGSWIDGETFDOSECRETS_PRIVATE_H

#include "DatabaseSettingsWidgetFdoSecrets.h"

#include "core/Database.h"

#include <QSortFilterProxyModel>

class DatabaseSettingsWidgetFdoSecrets::GroupModelNoRecycle: public QSortFilterProxyModel
{
	Q_OBJECT

public:
	explicit GroupModelNoRecycle(Database *db);

	Group* groupFromIndex(const QModelIndex &index) const;
	Group* groupFromSourceIndex(const QModelIndex &index) const;
	QModelIndex indexFromGroup(Group *group) const;

protected:
	bool filterAcceptsRow(int source_row, const QModelIndex &source_parent) const override;

private:
	Database *m_db;
};

#endif // KEEPASSXM_DATABASESETTINGSWIDGETFDOSECRETS_PRIVATE_H
