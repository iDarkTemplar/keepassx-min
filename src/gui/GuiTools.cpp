/*
 *  Copyright (C) 2024 KeePassXC Team <team@keepassxc.org>
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

#include "GuiTools.h"

#include "core/Config.h"
#include "core/Group.h"
#include "gui/MessageBox.h"

namespace GuiTools {

bool confirmDeleteEntries(QWidget *parent, const QList<Entry*> &entries, bool permanent)
{
	if (!parent || entries.isEmpty())
	{
		return false;
	}

	if (permanent)
	{
		QString prompt;
		if (entries.size() == 1)
		{
			auto entry = entries.first();
			prompt = QObject::tr("Do you really want to permanently delete the entry \"%1\"?").arg(entry->title().toHtmlEscaped());
		}
		else
		{
			prompt = QObject::tr("Do you really want to permanently delete %n entry(s)?", "", entries.size());
		}

		auto answer = MessageBox::question(parent,
			QObject::tr("Confirm Delete Entry(s)", "", entries.size()),
			prompt,
			MessageBox::Delete | MessageBox::Cancel,
			MessageBox::Cancel);

		return answer == MessageBox::Delete;
	}
	else if (config()->get(Config::Security_NoConfirmMoveEntryToRecycleBin).toBool())
	{
		return true;
	}
	else
	{
		QString prompt;
		if (entries.size() == 1)
		{
			auto entry = entries.first();
			prompt = QObject::tr("Do you really want to move entry \"%1\" to the recycle bin?").arg(entry->title().toHtmlEscaped());
		}
		else
		{
			prompt = QObject::tr("Do you really want to move %n entry(s) to the recycle bin?", "", entries.size());
		}

		auto answer = MessageBox::question(parent,
			QObject::tr("Confirm Recycle Entry(s)", "", entries.size()),
			prompt,
			MessageBox::Move | MessageBox::Cancel,
			MessageBox::Cancel);

		return answer == MessageBox::Move;
	}
}

size_t deleteEntries(const QList<Entry*> &entries, bool permanent)
{
	for (auto entry: entries)
	{
		if (permanent)
		{
			delete entry;
		}
		else
		{
			entry->database()->recycleEntry(entry);
		}
	}

	return entries.size();
}

} // namespace GuiTools
