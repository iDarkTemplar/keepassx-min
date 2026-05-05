/*
 *  Copyright (C) 2019 KeePassXC Team <team@keepassxc.org>
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

#include "HtmlGuiExporter.h"

#include <QBuffer>

#include "gui/Icons.h"

namespace {

QString PixmapToHTML(const QPixmap &pixmap)
{
	if (pixmap.isNull())
	{
		return QString();
	}

	// Based on https://stackoverflow.com/a/6621278
	QByteArray a;
	QBuffer buffer(&a);
	pixmap.save(&buffer, "PNG");
	return QStringLiteral("<img src=\"data:image/png;base64,") + QString::fromUtf8(a.toBase64()) + QStringLiteral("\"/>");
}

} // namespace

QString HtmlGuiExporter::groupIconToHtml(const Group *group)
{
	return PixmapToHTML(Icons::groupIconPixmap(group, IconSize::Medium));
}

QString HtmlGuiExporter::entryIconToHtml(const Entry *entry)
{
	return PixmapToHTML(Icons::entryIconPixmap(entry, IconSize::Medium));
}
