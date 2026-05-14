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
#include <QLibraryInfo>
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
	}

	if (m_translator_qtbase)
	{
		QCoreApplication::removeTranslator(m_translator_qtbase.get());
	}
}

/**
 * Perform early application bootstrapping such as setting up search paths,
 * configuration OS security properties, and loading translators.
 * A QApplication object has to be instantiated before calling this function.
 */
void Application::bootstrap(const QString &uiLanguage)
{
	kpxcApp->installTranslator(uiLanguage);

	Bootstrap::bootstrap();

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
	if (uiLanguage.isEmpty() || uiLanguage == QStringLiteral("system"))
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

	auto load_translation = [&languages](std::unique_ptr<QTranslator> &translator_holder, const QString &basename, const QString &path)
	{
		std::unique_ptr<QTranslator> old_translator = std::move(translator_holder);

		for (const auto &language: languages)
		{
			QLocale locale(language);
			std::unique_ptr<QTranslator> translator = std::make_unique<QTranslator>(qApp);
			if (translator->load(locale, basename, QStringLiteral("_"), path))
			{
				translator_holder = std::move(translator);
				QCoreApplication::installTranslator(translator_holder.get());
				break;
			}
		}

		if (old_translator)
		{
			QCoreApplication::removeTranslator(old_translator.get());
		}
	};

	load_translation(m_translator_qtbase, QStringLiteral("qtbase"), QLibraryInfo::path(QLibraryInfo::TranslationsPath));
	load_translation(m_translator, QStringLiteral("keepassxmin"), resources()->dataPath(QStringLiteral("translations")));
}

/**
 * @return list of pairs of available language codes and names
 */
QList<QPair<QString, QString>> Application::availableLanguages()
{
	QList<QPair<QString, QString>> languages;
	languages.append(QPair<QString, QString>(QStringLiteral("system"), tr("System default", "Language selection")));

	QRegularExpression regExp(QStringLiteral("^keepassxmin_([a-zA-Z_]+)\\.qm$"), QRegularExpression::CaseInsensitiveOption);
	const QStringList fileList = QDir(resources()->dataPath(QStringLiteral("translations"))).entryList();
	for (const QString &filename: fileList)
	{
		QRegularExpressionMatch match = regExp.match(filename);
		if (match.hasMatch())
		{
			QString langcode = match.captured(1);
			if (langcode == QStringLiteral("en"))
			{
				continue;
			}

			QLocale locale(langcode);
			QString languageStr = QLocale::languageToString(locale.language());
			if (langcode == QStringLiteral("la"))
			{
				// langcode "la" (Latin) is translated into "C" by QLocale::languageToString()
				languageStr = QStringLiteral("Latin");
			}

			if (langcode.contains(QStringLiteral("_")))
			{
				languageStr += QStringLiteral(" (%1)").arg(QLocale::territoryToString(locale.territory()));
			}

			QPair<QString, QString> language(langcode, languageStr);
			languages.append(language);
		}
	}

	return languages;
}
