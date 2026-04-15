/*
 *  Copyright (C) 2025 KeePassXC Team <team@keepassxc.org>
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

#include "UrlTools.h"
#include <QRegularExpression>
#include <QUrl>

const QString UrlTools::URL_WILDCARD = "1kpxcwc1";

Q_GLOBAL_STATIC(UrlTools, s_urlTools)

UrlTools *UrlTools::instance()
{
	return s_urlTools;
}

QUrl UrlTools::convertVariantToUrl(const QVariant &var) const
{
	QUrl url;
	if (var.canConvert<QUrl>())
	{
		url = var.toUrl();
	}
	return url;
}

// Returns true if URLs are identical. Paths with "/" are removed during comparison.
// URLs without scheme reverts to https.
// Special handling is needed because QUrl::matches() with QUrl::StripTrailingSlash does not strip "/" paths.
bool UrlTools::isUrlIdentical(const QString &first, const QString &second) const
{
	auto trimUrl = [](QString url) {
		url = url.trimmed();
		if (url.endsWith("/"))
		{
			url.remove(url.length() - 1, 1);
		}

		return url;
	};

	if (first.isEmpty() || second.isEmpty())
	{
		return false;
	}

	// Replace URL wildcards for comparison if found
	const auto firstUrl = trimUrl(QString(first).replace("*", UrlTools::URL_WILDCARD));
	const auto secondUrl = trimUrl(QString(second).replace("*", UrlTools::URL_WILDCARD));
	if (firstUrl == secondUrl)
	{
		return true;
	}

	return QUrl(firstUrl).matches(QUrl(secondUrl), QUrl::StripTrailingSlash);
}

bool UrlTools::isUrlValid(const QString &urlField, bool looseComparison) const
{
	if (urlField.isEmpty() || urlField.startsWith("cmd://", Qt::CaseInsensitive)
	    || urlField.startsWith("kdbx://", Qt::CaseInsensitive) || urlField.startsWith("{REF:A", Qt::CaseInsensitive))
	{
		return true;
	}

	auto url = urlField;

	// Loose comparison that allows wildcards and exact URL inside " characters
	if (looseComparison)
	{
		// Exact URL
		if (url.startsWith("\"") && url.endsWith("\""))
		{
			// Do not allow exact URL with wildcards, or empty exact URL
			if (url.contains("*") || url.length() == 2)
			{
				return false;
			}

			// Get the URL inside ""
			url.remove(0, 1);
			url.remove(url.length() - 1, 1);
		}
		else
		{
			// Do not allow URL with just wildcards, or double wildcards
			if (url.length() == url.count("*") || url.contains("**") || url.contains("*.*"))
			{
				return false;
			}

			url.replace("*", UrlTools::URL_WILDCARD);
		}
	}

	QUrl qUrl;
	if (urlField.contains("://"))
	{
		qUrl = url;
	}
	else
	{
		qUrl = QUrl::fromUserInput(url);
	}

	if (qUrl.scheme() != "file" && qUrl.host().isEmpty())
	{
		return false;
	}

	// Check for illegal characters. Adds also the wildcard * to the list
	QRegularExpression re("[<>\\^`{|}\\*]");
	auto match = re.match(url);
	if (match.hasMatch())
	{
		return false;
	}

	return true;
}

bool UrlTools::domainHasIllegalCharacters(const QString &domain) const
{
	QRegularExpression re(R"([\s\^#|/:<>\?@\[\]\\])");
	return re.match(domain).hasMatch();
}
