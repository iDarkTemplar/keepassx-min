/*
 *  Copyright (C) 2026 i.Dark_Templar <darktemplar@dark-templar-archives.net>
 *  Copyright (C) 2018 KeePassXC Team <team@keepassxc.org>
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

#include "TagModel.h"

#include "core/Database.h"
#include "core/Metadata.h"
#include "gui/Icons.h"
#include "gui/MessageBox.h"

#include <QApplication>
#include <QMenu>

TagModel::TagModel(QObject *parent)
	: QAbstractListModel(parent)
{
	m_defaultSearches << qMakePair(tr("Clear Search"), QString())
		<< qMakePair(tr("All Entries"), QStringLiteral("*"))
		<< qMakePair(tr("Expired"), QStringLiteral("is:expired"))
		<< qMakePair(tr("Weak Passwords"), QStringLiteral("is:weak"))
		<< qMakePair(tr("TOTP Entries"), QStringLiteral("has:totp"));
}

void TagModel::setDatabase(QSharedPointer<Database> db)
{
	if (m_db)
	{
		disconnect(m_db.data());
	}

	m_db = db;
	if (!m_db)
	{
		m_tagList.clear();
		return;
	}

	connect(m_db.data(), &Database::tagListUpdated, this, &TagModel::updateTagList);
	connect(m_db->metadata()->customData(), &CustomData::modified, this, &TagModel::updateTagList);

	updateTagList();
}

void TagModel::updateTagList()
{
	beginResetModel();
	m_tagList.clear();

	m_tagList << m_defaultSearches;

	auto savedSearches = m_db->metadata()->savedSearches();
	for (auto search: savedSearches.keys())
	{
		m_tagList << qMakePair(search, savedSearches[search].toString());
	}

	m_tagListStart = m_tagList.size();
	for (auto tag: m_db->tagList())
	{
		auto escapedTag = tag;
		escapedTag.replace(QStringLiteral("\""), QStringLiteral("\\\""));
		m_tagList << qMakePair(tag, QStringLiteral("tag:\"%1\"").arg(escapedTag));
	}

	endResetModel();
}

TagModel::TagType TagModel::itemType(const QModelIndex &index)
{
	int row = index.row();
	if (row < m_defaultSearches.size())
	{
		return TagType::DEFAULT_SEARCH;
	}
	else if (row < m_tagListStart)
	{
		return TagType::SAVED_SEARCH;
	}

	return TagType::TAG;
}

int TagModel::rowCount(const QModelIndex &parent) const
{
	Q_UNUSED(parent);
	return m_tagList.size();
}

QVariant TagModel::data(const QModelIndex &index, int role) const
{
	if (!index.isValid() || index.row() >= m_tagList.size())
	{
		return {};
	}

	const auto row = index.row();
	switch (role)
	{
	case Qt::DecorationRole:
		if (row < m_tagListStart)
		{
			return icons()->icon(QStringLiteral("database-search"));
		}

		return icons()->icon(QStringLiteral("tag"));
	case Qt::DisplayRole:
		return m_tagList.at(row).first;
	case Qt::UserRole:
		return m_tagList.at(row).second;
	case Qt::UserRole + 1:
		if (row == (m_defaultSearches.size() - 1))
		{
			return true;
		}

		return false;
	}

	return {};
}
