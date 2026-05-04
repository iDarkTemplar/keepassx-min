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

#ifndef KEEPASSX_APPLICATION_H
#define KEEPASSX_APPLICATION_H

#include <QApplication>
#include <QString>
#include <QTranslator>
#include <QPointer>

class Application: public QApplication
{
	Q_OBJECT

public:
	Application(int &argc, char **argv);
	~Application() override;

	static void bootstrap(const QString &uiLanguage = "system");
	static void applyFontSize();

	bool event(QEvent *event) override;

	void installTranslator(const QString &uiLanguage = "system");

	static QList<QPair<QString, QString>> availableLanguages();

Q_SIGNALS:
	void openFile(const QString &filename);

private:
	QPointer<QTranslator> m_translator;
};

#define kpxcApp qobject_cast<Application*>(Application::instance())

#endif // KEEPASSX_APPLICATION_H
