/*
 *  Copyright (C) 2026 i.Dark_Templar <darktemplar@dark-templar-archives.net>
 *  Copyright (C) 2010 Felix Geyer <debfx@fobos.de>
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

#include <QCommandLineParser>
#include <QDir>
#include <QFile>
#include <QThreadPool>
#include <QWindow>

#include "config-keepassx.h"
#include "core/Tools.h"
#include "crypto/Crypto.h"
#include "gui/Application.h"
#include "gui/MainWindow.h"
#include "gui/MessageBox.h"

#if defined(WITH_ASAN) && defined(WITH_LSAN)
#include <sanitizer/lsan_interface.h>
#endif

int main(int argc, char **argv)
{
	QT_REQUIRE_VERSION(argc, argv, QT_VERSION_STR)

	Application app(argc, argv);
	// don't set organizationName as that changes the return value of
	// QStandardPaths::writableLocation(QDesktopServices::DataLocation)
	Application::setApplicationName(QStringLiteral("KeePassX-min"));
	Application::setApplicationVersion(QStringLiteral(KEEPASSXM_VERSION));

	// HACK: Prevent long-running threads from deadlocking the program with only 1 CPU
	// See https://github.com/keepassxreboot/keepassxc/issues/10391
	if (QThreadPool::globalInstance()->maxThreadCount() < 2)
	{
		QThreadPool::globalInstance()->setMaxThreadCount(2);
	}

	Application::bootstrap();

	QCommandLineParser parser;
	parser.setApplicationDescription(QObject::tr("KeePassX-min - password manager"));
	parser.addPositionalArgument(QStringLiteral("filename(s)"), QObject::tr("filenames of the password databases to open (*.kdbxm)"), QStringLiteral("[filename(s)]"));

	QCommandLineOption configOption(QStringLiteral("config"), QObject::tr("path to a custom config file"), QStringLiteral("config"));
	QCommandLineOption localConfigOption(QStringLiteral("localconfig"), QObject::tr("path to a custom local config file"), QStringLiteral("localconfig"));
	QCommandLineOption keyfileOption(QStringLiteral("keyfile"), QObject::tr("key file of the database"), QStringLiteral("keyfile"));
	QCommandLineOption startMinimized(QStringLiteral("minimized"), QObject::tr("start minimized to the system tray"));

	QCommandLineOption helpOption = parser.addHelpOption();
	QCommandLineOption versionOption = parser.addVersionOption();
	QCommandLineOption debugInfoOption(QStringList() << QStringLiteral("debug-info"), QObject::tr("Displays debugging information."));

	parser.addOption(configOption);
	parser.addOption(localConfigOption);
	parser.addOption(keyfileOption);
	parser.addOption(debugInfoOption);
	parser.addOption(startMinimized);

	parser.process(app);

	// Exit early if we're only showing the help / version
	if (parser.isSet(versionOption) || parser.isSet(helpOption))
	{
		return EXIT_SUCCESS;
	}

	// Show debug information and then exit
	if (parser.isSet(debugInfoOption))
	{
		QTextStream out(stdout, QIODevice::WriteOnly);
		QString debugInfo = Tools::debugInfo().append(QStringLiteral("\n")).append(Crypto::debugInfo());
		out << debugInfo << Qt::endl;
		return EXIT_SUCCESS;
	}

	// Process config file options early
	if (parser.isSet(configOption) || parser.isSet(localConfigOption))
	{
		Config::createConfigFromFile(parser.value(configOption), parser.value(localConfigOption));
	}

	// Extract file names provided on the command line for opening
	QStringList fileNames;
	for (const auto &file: parser.positionalArguments())
	{
		if (QFile::exists(file))
		{
			fileNames << QDir::toNativeSeparators(file);
		}
	}

	if (!Crypto::init())
	{
		QString error = QObject::tr("Fatal error while testing the cryptographic functions.");
		error.append(QStringLiteral("\n"));
		error.append(Crypto::errorString());
		MessageBox::critical(nullptr, QObject::tr("KeePassX-min - Error"), error);
		return EXIT_FAILURE;
	}

	QGuiApplication::setDesktopFileName(QStringLiteral("keepassx-min.desktop"));

	MainWindow mainWindow;

	// start minimized if configured
	if (parser.isSet(startMinimized) || config()->get(Config::GUI_MinimizeOnStartup).toBool())
	{
		mainWindow.hideWindow();
	}
	else
	{
		mainWindow.bringToFront();
		Application::processEvents();
	}

	int exitCode = Application::exec();

#if defined(WITH_ASAN) && defined(WITH_LSAN)
	// do leak check here to prevent massive tail of end-of-process leak errors from third-party libraries
	__lsan_do_leak_check();
	__lsan_disable();
#endif

	return exitCode;
}
