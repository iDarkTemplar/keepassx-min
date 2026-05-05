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

#include "OpVaultReader.h"

#include "core/Entry.h"
#include "core/Totp.h"

#include <QDebug>
#include <QJsonArray>
#include <QJsonObject>
#include <QLocale>
#include <QUrlQuery>

namespace {

QDateTime resolveDate(const QString &kind, const QJsonValue &value)
{
	QDateTime date;
	if (kind == QStringLiteral("monthYear"))
	{
		// 1Password programmers are sadistic...
		auto dateValue = QString::number(value.toInt());
		date = QDateTime::fromString(dateValue, QStringLiteral("yyyyMM"));
		date.setTimeSpec(Qt::UTC);
	}
	else if (value.isString())
	{
		date = QDateTime::fromSecsSinceEpoch(value.toString().toUInt(), QTimeZone::utc());
	}
	else
	{
		date = QDateTime::fromSecsSinceEpoch(value.toInt(), QTimeZone::utc());
	}

	return date;
}

} // namespace

void OpVaultReader::fillFromSection(Entry *entry, const QJsonObject &section)
{
	const auto uuid = entry->uuid();
	auto sectionTitle = section[QStringLiteral("title")].toString();

	if (!section.contains(QStringLiteral("fields")))
	{
		auto sectionName = section[QStringLiteral("name")].toString();
		if (!(sectionName.toLower() == QStringLiteral("linked items") && sectionTitle.toLower() == QStringLiteral("related items")))
		{
			qWarning() << tr("Skipping \"fields\"-less Section in UUID \"%1\": << %2 >>").arg(uuid.toString()).arg(QJsonDocument(section).toJson(QJsonDocument::Compact));
		}

		return;
	}
	else if (!section[QStringLiteral("fields")].isArray())
	{
		qWarning() << tr("Skipping non-Array \"fields\" in UUID \"%1\"").arg(uuid.toString());
		return;
	}

	QJsonArray sectionFields = section[QStringLiteral("fields")].toArray();
	for (const QJsonValue sectionField: sectionFields)
	{
		if (!sectionField.isObject())
		{
			qWarning() << tr("Skipping non-Object \"fields\" in UUID \"%1\": << %2 >>").arg(uuid.toString()).arg(sectionField.toString());
			continue;
		}

		fillFromSectionField(entry, sectionTitle, sectionField.toObject());
	}
}

void OpVaultReader::fillFromSectionField(Entry *entry, const QString &sectionName, const QJsonObject &field)
{
	if (!field.contains(QStringLiteral("v")))
	{
		// for our purposes, we don't care if there isn't a value in the field
		return;
	}

	// Ignore "a" and "inputTraits" fields, they don't apply to KPXC

	auto attrName = resolveAttributeName(sectionName, field[QStringLiteral("n")].toString(), field[QStringLiteral("t")].toString());
	auto attrValue = field.value(QStringLiteral("v")).toString();
	auto kind = field[QStringLiteral("k")].toString();

	if (attrName.startsWith(QStringLiteral("TOTP_")))
	{
		if (entry->hasTotp())
		{
			// Store multiple TOTP definitions as additional otp attributes
			int i = 0;
			QString name = QStringLiteral("otp");
			auto attributes = entry->attributes()->keys();
			while (attributes.contains(name))
			{
				name = QStringLiteral("otp_%1").arg(++i);
			}

			entry->attributes()->set(name, attrValue, true);
		}
		else if (attrValue.startsWith(QStringLiteral("otpauth://")))
		{
			QUrlQuery query(attrValue);
			// at least as of 1Password 7, they don't append the digits= and period= which totp.cpp requires
			if (!query.hasQueryItem(QStringLiteral("digits")))
			{
				query.addQueryItem(QStringLiteral("digits"), QStringLiteral("%1").arg(Totp::DEFAULT_DIGITS));
			}

			if (!query.hasQueryItem(QStringLiteral("period")))
			{
				query.addQueryItem(QStringLiteral("period"), QStringLiteral("%1").arg(Totp::DEFAULT_STEP));
			}

			attrValue = query.toString(QUrl::FullyEncoded);
			entry->setTotp(Totp::parseSettings(attrValue));
		}
		else
		{
			entry->setTotp(Totp::parseSettings({}, attrValue));
		}
	}
	else if (attrName.startsWith(QStringLiteral("expir"), Qt::CaseInsensitive))
	{
		QDateTime expiry = resolveDate(kind, field.value(QStringLiteral("v")));
		if (expiry.isValid())
		{
			entry->setExpiryTime(expiry);
			entry->setExpires(true);
		}
		else
		{
			qWarning() << tr("[%1] Invalid expiration date found: %2").arg(entry->title(), attrValue);
		}
	}
	else
	{
		if (kind == QStringLiteral("date") || kind == QStringLiteral("monthYear"))
		{
			QDateTime date = resolveDate(kind, field.value(QStringLiteral("v")));
			if (date.isValid())
			{
				entry->attributes()->set(attrName, QLocale::system().toString(date, QLocale::ShortFormat));
			}
			else
			{
				qWarning() << tr("[%1] Invalid date attribute found: %2 = %3").arg(entry->title(), attrName, attrValue);
			}
		}
		else if (kind == QStringLiteral("address"))
		{
			// Expand address into multiple attributes
			auto addrFields = field.value(QStringLiteral("v")).toObject().toVariantMap();
			for (auto &part: addrFields.keys())
			{
				entry->attributes()->set(attrName + QStringLiteral("_%1").arg(part), addrFields.value(part).toString());
			}
		}
		else
		{
			if (entry->attributes()->hasKey(attrName))
			{
				// Append a random string to the attribute name to avoid collisions
				attrName += QStringLiteral("_%1").arg(QUuid::createUuid().toString().mid(1, 5));
			}

			entry->attributes()->set(attrName, attrValue, (kind == QStringLiteral("password") || kind == QStringLiteral("concealed")));
		}
	}
}

QString OpVaultReader::resolveAttributeName(const QString &section, const QString &name, const QString &text)
{
	// Special case for TOTP
	if (name.startsWith(QStringLiteral("TOTP_")))
	{
		return name;
	}

	auto lowName = name.toLower();
	auto lowText = text.toLower();
	if (section.isEmpty() || name.startsWith(QStringLiteral("address")))
	{
		// Empty section implies these are core attributes
		// try to find username, password, url
		if (lowName == QStringLiteral("password") || lowText == QStringLiteral("password"))
		{
			return EntryAttributes::PasswordKey;
		}
		else if (lowName == QStringLiteral("username") || lowText == QStringLiteral("username"))
		{
			return EntryAttributes::UserNameKey;
		}
		else if (lowName == QStringLiteral("url")
			|| lowText == QStringLiteral("url")
			|| lowName == QStringLiteral("hostname")
			|| lowText == QStringLiteral("server")
			|| lowName == QStringLiteral("website"))
		{
			return EntryAttributes::URLKey;
		}

		return text;
	}

	return QStringLiteral("%1_%2").arg(section, text);
}
