/*
 *  Copyright (C) 2026 i.Dark_Templar <darktemplar@dark-templar-archives.net>
 *  Copyright (C) 2012 Tobias Tangemann
 *  Copyright (C) 2012 Felix Geyer <debfx@fobos.de>
 *  Copyright (C) 2020 KeePassXC Team <team@keepassxc.org>
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

#include "Application.h"

#include "core/Bootstrap.h"
#include "core/Tools.h"
#include "gui/MainWindow.h"
#include "gui/MessageBox.h"

#include <QFileInfo>
#include <QFileOpenEvent>
#include <QPixmapCache>
#include <QRegularExpression>
#include <QStandardPaths>

#include <signal.h>
#include <sys/socket.h>
#include <unistd.h>

namespace {

constexpr int WaitTimeoutMSec = 150;
const char BlockSizeProperty[] = "blockSize";
int g_OriginalFontSize = 0;

} // namespace

Application::Application(int &argc, char **argv)
	: QApplication(argc, argv)
{
}

Application::~Application()
{
}

/**
 * Perform early application bootstrapping such as setting up search paths,
 * configuration OS security properties, and loading translators.
 * A QApplication object has to be instantiated before calling this function.
 */
void Application::bootstrap(const QString &uiLanguage)
{
	Bootstrap::bootstrap(uiLanguage);

	applyFontSize();

	MessageBox::initializeButtonDefs();
}

void Application::applyFontSize()
{
	auto font = QApplication::font();

	// Store the original font size on first call
	if (g_OriginalFontSize <= 0)
	{
		g_OriginalFontSize = font.pointSize();
	}

	// Adjust application wide default font size
	auto newSize = g_OriginalFontSize + qBound(-2, config()->get(Config::GUI_FontSizeOffset).toInt(), 4);
	font.setPointSize(newSize);
	QApplication::setFont(font);
	QApplication::setFont(font, "QWidget");
}

bool Application::event(QEvent *event)
{
	// Handle Apple QFileOpenEvent from finder (double click on .kdbx file)
	if (event->type() == QEvent::FileOpen)
	{
		emit openFile(static_cast<QFileOpenEvent*>(event)->file());
		return true;
	}

	return QApplication::event(event);
}
