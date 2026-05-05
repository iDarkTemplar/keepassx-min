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

#include "ProtonPassReader.h"

#include "core/Database.h"
#include "core/Entry.h"
#include "core/Group.h"
#include "core/Metadata.h"
#include "core/Tools.h"
#include "core/Totp.h"
#include "crypto/CryptoHash.h"

#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QMap>
#include <QScopedPointer>
#include <QUrl>

#include <memory>

namespace {

Entry* readItem(const QJsonObject &item)
{
	const auto itemMap = item.toVariantMap();
	const auto dataMap = itemMap.value(QStringLiteral("data")).toMap();
	const auto metadataMap = dataMap.value(QStringLiteral("metadata")).toMap();

	// Create entry and assign basic values
	std::unique_ptr<Entry> entry = std::make_unique<Entry>();
	entry->setUuid(QUuid::createUuid());
	entry->setTitle(metadataMap.value(QStringLiteral("name")).toString());
	entry->setNotes(metadataMap.value(QStringLiteral("note")).toString());

	if (itemMap.value(QStringLiteral("pinned")).toBool())
	{
		entry->addTag(QObject::tr("Favorite", "Tag for favorite entries"));
	}

	// Handle specific item types
	auto type = dataMap.value(QStringLiteral("type")).toString();

	// Login
	if (type.compare(QStringLiteral("login"), Qt::CaseInsensitive) == 0)
	{
		const auto loginMap = dataMap.value(QStringLiteral("content")).toMap();
		entry->setUsername(loginMap.value(QStringLiteral("itemUsername")).toString());
		entry->setPassword(loginMap.value(QStringLiteral("password")).toString());
		if (loginMap.contains(QStringLiteral("totpUri")))
		{
			auto totp = loginMap.value(QStringLiteral("totpUri")).toString();
			if (!totp.startsWith(QStringLiteral("otpauth://")))
			{
				QUrl url(QStringLiteral("otpauth://totp/%1:%2?secret=%3")
					.arg(QString::fromUtf8(QUrl::toPercentEncoding(entry->title())),
						QString::fromUtf8(QUrl::toPercentEncoding(entry->username())),
						QString::fromUtf8(QUrl::toPercentEncoding(totp))));

				totp = url.toString(QUrl::FullyEncoded);
			}

			entry->setTotp(Totp::parseSettings(totp));
		}

		if (loginMap.contains(QStringLiteral("itemEmail")))
		{
			// Place the email value as the username if empty, otherwise set it as an attribute
			const auto email = loginMap.value(QStringLiteral("itemEmail")).toString();
			if (entry->username().isEmpty())
			{
				entry->setUsername(email);
			}
			else if (!email.isEmpty())
			{
				entry->attributes()->set(QStringLiteral("login_email"), email);
			}
		}

		// Set the entry url(s)
		int i = 1;
		for (const auto &urlObj: loginMap.value(QStringLiteral("urls")).toList())
		{
			const auto url = urlObj.toString();
			if (entry->url().isEmpty())
			{
				// First url encountered is set as the primary url
				entry->setUrl(url);
			}
			else
			{
				// Subsequent urls
				entry->attributes()->set(QStringLiteral("%1_%2").arg(EntryAttributes::AdditionalUrlAttribute, QString::number(i)), url);
				++i;
			}
		}
	}
	// Credit Card
	else if (type.compare(QStringLiteral("creditCard"), Qt::CaseInsensitive) == 0)
	{
		const auto cardMap = dataMap.value(QStringLiteral("content")).toMap();
		entry->setUsername(cardMap.value(QStringLiteral("number")).toString());
		entry->setPassword(cardMap.value(QStringLiteral("verificationNumber")).toString());
		const QStringList attrs({QStringLiteral("cardholderName"), QStringLiteral("pin"), QStringLiteral("expirationDate")});
		const QStringList sensitive({QStringLiteral("pin")});

		for (const auto &attr: attrs)
		{
			auto value = cardMap.value(attr).toString();
			if (!value.isEmpty())
			{
				entry->attributes()->set(QStringLiteral("card_") + attr, value, sensitive.contains(attr));
			}
		}
	}

	// Parse extra fields
	for (const auto &field: dataMap.value(QStringLiteral("extraFields")).toList())
	{
		// Derive a prefix for attribute names using the title or uuid if missing
		const auto fieldMap = field.toMap();
		auto name = fieldMap.value(QStringLiteral("fieldName")).toString();
		if (entry->attributes()->hasKey(name))
		{
			name = QStringLiteral("%1_%2").arg(name, QUuid::createUuid().toString().mid(1, 5));
		}

		QString value;
		const auto fieldType = fieldMap.value(QStringLiteral("type")).toString();
		if (fieldType.compare(QStringLiteral("totp"), Qt::CaseInsensitive) == 0)
		{
			value = fieldMap.value(QStringLiteral("data")).toJsonObject().value(QStringLiteral("totpUri")).toString();
		}
		else
		{
			value = fieldMap.value(QStringLiteral("data")).toJsonObject().value(QStringLiteral("content")).toString();
		}

		entry->attributes()->set(name, value, fieldType.compare(QStringLiteral("hidden"), Qt::CaseInsensitive) == 0);
	}

	// Checked expired/deleted state
	if (itemMap.value(QStringLiteral("state")).toInt() == 2)
	{
		entry->setExpires(true);
		entry->setExpiryTime(QDateTime::currentDateTimeUtc());
	}

	// Collapse any accumulated history
	entry->removeHistoryItems(entry->historyItems());

	// Adjust the created and modified times
	auto timeInfo = entry->timeInfo();
	const auto createdTime = QDateTime::fromSecsSinceEpoch(itemMap.value(QStringLiteral("createTime")).toULongLong(), QTimeZone::utc());
	const auto modifiedTime = QDateTime::fromSecsSinceEpoch(itemMap.value(QStringLiteral("modifyTime")).toULongLong(), QTimeZone::utc());
	timeInfo.setCreationTime(createdTime);
	timeInfo.setLastModificationTime(modifiedTime);
	timeInfo.setLastAccessTime(modifiedTime);
	entry->setTimeInfo(timeInfo);

	return entry.release();
}

void writeVaultToDatabase(const QJsonObject &vault, QSharedPointer<Database> db)
{
	// Create groups from vaults and store a temporary map of id -> uuid
	const auto vaults = vault.value(QStringLiteral("vaults")).toObject().toVariantMap();
	for (const auto &vaultId: vaults.keys())
	{
		auto vaultObj = vaults.value(vaultId).toJsonObject();
		auto group = new Group();
		group->setUuid(QUuid::createUuid());
		group->setName(vaultObj.value(QStringLiteral("name")).toString());
		group->setNotes(vaultObj.value(QStringLiteral("description")).toString());
		group->setParent(db->rootGroup());

		const auto items = vaultObj.value(QStringLiteral("items")).toArray();
		for (const auto &item: items)
		{
			auto entry = readItem(item.toObject());
			if (entry)
			{
				entry->setGroup(group, false);
			}
		}
	}
}

} // namespace

bool ProtonPassReader::hasError()
{
	return !m_error.isEmpty();
}

QString ProtonPassReader::errorString()
{
	return m_error;
}

QSharedPointer<Database> ProtonPassReader::convert(const QString &path)
{
	m_error.clear();

	QFileInfo fileinfo(path);
	if (!fileinfo.exists())
	{
		m_error = QObject::tr("File does not exist.").arg(path);
		return {};
	}

	// Bitwarden uses a json file format
	QFile file(fileinfo.absoluteFilePath());
	if (!file.open(QFile::ReadOnly))
	{
		m_error = QObject::tr("Cannot open file: %1").arg(file.errorString());
		return {};
	}

	QJsonParseError error;
	auto json = QJsonDocument::fromJson(file.readAll(), &error).object();
	if (error.error != QJsonParseError::NoError)
	{
		m_error = QObject::tr("Cannot parse file: %1 at position %2").arg(error.errorString(), QString::number(error.offset));
		return {};
	}

	file.close();

	if (json.value(QStringLiteral("encrypted")).toBool())
	{
		m_error = QObject::tr("Encrypted files are not supported.");
		return {};
	}

	auto db = QSharedPointer<Database>::create();
	db->rootGroup()->setName(QObject::tr("Proton Pass Import"));

	writeVaultToDatabase(json, db);

	return db;
}
