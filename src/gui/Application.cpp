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
#include "core/Resources.h"
#include "core/Tools.h"
#include "gui/MainWindow.h"
#include "gui/MessageBox.h"

#include <QDir>
#include <QFileInfo>
#include <QFileOpenEvent>
#include <QPixmapCache>
#include <QRegularExpression>
#include <QStandardPaths>

namespace {

int g_OriginalFontSize = 0;

} // namespace

Application::Application(int &argc, char **argv)
	: QApplication(argc, argv)
{
}

Application::~Application()
{
	if (m_translator)
	{
		QCoreApplication::removeTranslator(m_translator.get());
		delete m_translator;
	}
}

/**
 * Perform early application bootstrapping such as setting up search paths,
 * configuration OS security properties, and loading translators.
 * A QApplication object has to be instantiated before calling this function.
 */
void Application::bootstrap(const QString &uiLanguage)
{
	Bootstrap::bootstrap();

	kpxcApp->installTranslator(uiLanguage);

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
	// Handle Apple QFileOpenEvent from finder (double click on .kdbxm file)
	if (event->type() == QEvent::FileOpen)
	{
		Q_EMIT openFile(static_cast<QFileOpenEvent*>(event)->file());
		return true;
	}

	return QApplication::event(event);
}

void Application::installTranslator(const QString &uiLanguage)
{
	QStringList languages;
	if (uiLanguage.isEmpty() || uiLanguage == "system")
	{
		// NOTE: this is a workaround for the terrible way Qt loads languages
		// using the QLocale::uiLanguages() approach. Instead, we search each
		// language and all country variants in order before moving to the next.
		QLocale locale = QLocale::system();
		languages = locale.uiLanguages();
	}
	else
	{
		languages << uiLanguage;
	}

	QPointer<QTranslator> old_translator = m_translator;

	const auto path = resources()->dataPath("translations");

	for (const auto &language: languages)
	{
		QLocale locale(language);
		QScopedPointer<QTranslator> translator(new QTranslator(qApp));
		if (translator->load(locale, "keepassxmin_", "", path))
		{
			m_translator = translator.take();
			QCoreApplication::installTranslator(m_translator);
			break;
		}
	}

	if (old_translator)
	{
		QCoreApplication::removeTranslator(old_translator);
		delete old_translator;
	}
}

/**
 * @return list of pairs of available language codes and names
 */
QList<QPair<QString, QString>> Application::availableLanguages()
{
	QList<QPair<QString, QString>> languages;
	languages.append(QPair<QString, QString>("system", "System default"));

	QRegularExpression regExp("^keepassxmin_([a-zA-Z_]+)\\.qm$", QRegularExpression::CaseInsensitiveOption);
	const QStringList fileList = QDir(resources()->dataPath("translations")).entryList();
	for (const QString &filename: fileList)
	{
		QRegularExpressionMatch match = regExp.match(filename);
		if (match.hasMatch())
		{
			QString langcode = match.captured(1);
			if (langcode == "en")
			{
				continue;
			}

			QLocale locale(langcode);
			QString languageStr = QLocale::languageToString(locale.language());
			if (langcode == "la")
			{
				// langcode "la" (Latin) is translated into "C" by QLocale::languageToString()
				languageStr = "Latin";
			}

			if (langcode.contains("_"))
			{
				languageStr += QString(" (%1)").arg(QLocale::countryToString(locale.country()));
			}

			QPair<QString, QString> language(langcode, languageStr);
			languages.append(language);
		}
	}

	return languages;
}
