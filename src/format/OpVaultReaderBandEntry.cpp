/*
 *  Copyright (C) 2023 KeePassXC Team <team@keepassxc.org>
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

#include "OpData01.h"
#include "OpVaultReader.h"

#include "core/Group.h"
#include "core/Tools.h"
#include "crypto/CryptoHash.h"
#include "crypto/SymmetricCipher.h"

#include <QDebug>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>

#include <memory>

bool OpVaultReader::decryptBandEntry(
	const QJsonObject &bandEntry,
	QJsonObject &data,
	QByteArray &key,
	QByteArray &hmacKey)
{
	if (!bandEntry.contains(QStringLiteral("d")))
	{
		qWarning() << tr("Band entries must contain a \"d\" key: %1").arg(bandEntry.keys().join(QStringLiteral(", ")));
		return false;
	}

	if (!bandEntry.contains(QStringLiteral("k")))
	{
		qWarning() << tr("Band entries must contain a \"k\" key: %1").arg(bandEntry.keys().join(QStringLiteral(", ")));
		return false;
	}

	const QString uuid = bandEntry.value(QStringLiteral("uuid")).toString();

	/*!
	 * This is the encrypted item and MAC keys.
	 * It is encrypted with the master encryption key and authenticated with the master MAC key.
	 *
	 * The last 32 bytes comprise the HMAC-SHA256 of the IV and the encrypted data.
	 * The MAC is computed with the master MAC key.
	 * The data before the MAC is the AES-CBC encrypted item keys using unique random 16-byte IV.
	 * \code
	 *   uint8_t crypto_key[32];
	 *   uint8_t mac_key[32];
	 * \endcode
	 * \sa https://support.1password.com/opvault-design/#k
	 */
	const QString &entKStr = bandEntry[QStringLiteral("k")].toString();
	QByteArray kBA = QByteArray::fromBase64(entKStr.toUtf8());
	const int wantKsize = 16 + 32 + 32 + 32;
	if (kBA.size() != wantKsize)
	{
		qCritical() << tr("Malformed \"k\" size; expected %1 got %2\n").arg(wantKsize).arg(kBA.size());
		return false;
	}

	QByteArray hmacSig = kBA.mid(kBA.size() - 32, 32);
	const QByteArray &realHmacSig = CryptoHash::hmac(kBA.mid(0, kBA.size() - hmacSig.size()), m_masterHmacKey, CryptoHash::Sha256);
	if (realHmacSig != hmacSig)
	{
		qCritical() << tr("Entry \"k\" failed its HMAC in UUID \"%1\", wanted \"%2\" got \"%3\"")
			.arg(uuid)
			.arg(QString::fromUtf8(hmacSig.toHex()))
			.arg(QString::fromUtf8(realHmacSig));

		return false;
	}

	QByteArray iv = kBA.mid(0, 16);
	QByteArray keyAndMacKey = kBA.mid(iv.size(), 64);
	SymmetricCipher cipher;
	if (!cipher.init(SymmetricCipher::Aes256_CBC, SymmetricCipher::Decrypt, m_masterKey, iv))
	{
		qCritical() << tr("Unable to init cipher using masterKey in UUID %1").arg(uuid);
		return false;
	}

	if (!cipher.process(keyAndMacKey))
	{
		qCritical() << tr("Unable to decipher \"k\"(key+hmac) in UUID %1").arg(uuid);
		return false;
	}

	key = keyAndMacKey.mid(0, 32);
	hmacKey = keyAndMacKey.mid(32);

	QString dKeyB64 = bandEntry.value(QStringLiteral("d")).toString();
	OpData01 entD01;
	if (!entD01.decodeBase64(dKeyB64, key, hmacKey))
	{
		qCritical() << tr("Unable to decipher \"d\" in UUID \"%1\": %2").arg(uuid).arg(entD01.errorString());
		return false;
	}

	auto clearText = entD01.getClearText();
	data = QJsonDocument::fromJson(clearText).object();

	return true;
}

Entry* OpVaultReader::processBandEntry(const QJsonObject &bandEntry, const QDir &attachmentDir, Group *rootGroup)
{
	const QString uuid = bandEntry.value(QStringLiteral("uuid")).toString();
	if (!(uuid.size() == 32 || uuid.size() == 36))
	{
		qWarning() << tr("Skipping suspicious band UUID <<%1>> with length %2").arg(uuid).arg(uuid.size());
		return nullptr;
	}

	std::unique_ptr<Entry> entry = std::make_unique<Entry>();

	if (bandEntry.contains(QStringLiteral("trashed")) && bandEntry[QStringLiteral("trashed")].toBool())
	{
		// Send this entry to the recycle bin
		rootGroup->database()->recycleEntry(entry.get());
	}
	else if (bandEntry.contains(QStringLiteral("category")))
	{
		const QJsonValue &categoryValue = bandEntry[QStringLiteral("category")];
		if (categoryValue.isString())
		{
			bool found = false;
			const QString category = categoryValue.toString();
			for (Group *group: rootGroup->children())
			{
				const QVariant &groupCode = group->property("code");
				if (category == groupCode.toString())
				{
					entry->setGroup(group);
					found = true;
					break;
				}
			}

			if (!found)
			{
				qWarning() << tr("Unable to place Entry.Category \"%1\" so using the Root instead").arg(category);
				entry->setGroup(rootGroup);
			}
		}
		else
		{
			qWarning() << tr("Skipping non-String Category type \"%1\" in UUID \"%2\"")
				.arg(categoryValue.type())
				.arg(uuid);

			entry->setGroup(rootGroup);
		}
	}
	else
	{
		qWarning() << tr("Using the root group because the entry is category-less: <<\n%1\n>> in UUID %2").arg(QJsonDocument(bandEntry).toJson(QJsonDocument::Compact)).arg(uuid);
		entry->setGroup(rootGroup);
	}

	entry->setUpdateTimeinfo(false);
	TimeInfo ti;
	bool timeInfoOk = false;
	if (bandEntry.contains(QStringLiteral("created")))
	{
		auto createdTime = static_cast<uint>(bandEntry[QStringLiteral("created")].toInt());
		ti.setCreationTime(QDateTime::fromSecsSinceEpoch(createdTime, QTimeZone::utc()));
		timeInfoOk = true;
	}

	if (bandEntry.contains(QStringLiteral("updated")))
	{
		auto updateTime = static_cast<uint>(bandEntry[QStringLiteral("updated")].toInt());
		ti.setLastModificationTime(QDateTime::fromSecsSinceEpoch(updateTime, QTimeZone::utc()));
		timeInfoOk = true;
	}

	// "tx" is modified by sync, not by user; maybe a custom attribute?
	if (timeInfoOk)
	{
		entry->setTimeInfo(ti);
	}

	entry->setUuid(Tools::hexToUuid(uuid));

	if (!fillAttributes(entry.get(), bandEntry))
	{
		return nullptr;
	}

	QJsonObject data;
	QByteArray entryKey;
	QByteArray entryHmacKey;

	if (!decryptBandEntry(bandEntry, data, entryKey, entryHmacKey))
	{
		return nullptr;
	}

	if (data.contains(QStringLiteral("notesPlain")))
	{
		entry->setNotes(data.value(QStringLiteral("notesPlain")).toString());
	}

	// it seems sometimes the password is a top-level field, and not in "fields" themselves
	if (data.contains(QStringLiteral("password")))
	{
		entry->setPassword(data.value(QStringLiteral("password")).toString());
	}

	for (const auto fieldValue: data.value(QStringLiteral("fields")).toArray())
	{
		if (!fieldValue.isObject())
		{
			continue;
		}

		auto field = fieldValue.toObject();
		auto designation = field[QStringLiteral("designation")].toString();
		auto value = field[QStringLiteral("value")].toString();

		if (designation == QStringLiteral("password"))
		{
			entry->setPassword(value);
		}
		else if (designation == QStringLiteral("username"))
		{
			entry->setUsername(value);
		}
	}

	const QJsonArray &sectionsArray = data[QStringLiteral("sections")].toArray();
	for (const QJsonValue &sectionValue: sectionsArray)
	{
		if (!sectionValue.isObject())
		{
			qWarning() << tr("Skipping non-Object in \"sections\" for UUID \"%1\" << %2 >>").arg(uuid).arg(QJsonDocument(sectionsArray).toJson(QJsonDocument::Compact));
			continue;
		}

		const QJsonObject &section = sectionValue.toObject();

		fillFromSection(entry.get(), section);
	}

	fillAttachments(entry.get(), attachmentDir, entryKey, entryHmacKey);

	return entry.release();
}

bool OpVaultReader::fillAttributes(Entry *entry, const QJsonObject &bandEntry)
{
	const QString overviewStr = bandEntry.value(QStringLiteral("o")).toString();
	OpData01 entOver01;
	if (!entOver01.decodeBase64(overviewStr, m_overviewKey, m_overviewHmacKey))
	{
		qCritical() << tr("Unable to decipher \"o\" in UUID \"%1\": %2").arg(entry->uuid().toString()).arg(entOver01.errorString());
		return false;
	}

	auto overviewJsonBytes = entOver01.getClearText();
	auto overviewDoc = QJsonDocument::fromJson(overviewJsonBytes);
	auto overviewJson = overviewDoc.object();

	QString title = overviewJson.value(QStringLiteral("title")).toString();
	entry->setTitle(title);

	QString url = overviewJson[QStringLiteral("url")].toString();
	entry->setUrl(url);

	int i = 1;
	for (const auto urlV: overviewJson[QStringLiteral("URLs")].toArray())
	{
		const auto &urlObj = urlV.toObject();
		if (urlObj.contains(QStringLiteral("u")))
		{
			auto newUrl = urlObj[QStringLiteral("u")].toString();
			if (newUrl != url)
			{
				// Add this url if it isn't the base one
				entry->attributes()->set(QStringLiteral("%1_%2").arg(EntryAttributes::AdditionalUrlAttribute, QString::number(i)), newUrl);
				++i;
			}
		}
	}

	QStringList tagsList;
	for (const auto tagV: overviewJson[QStringLiteral("tags")].toArray())
	{
		if (tagV.isString())
		{
			tagsList << tagV.toString();
		}
	}

	entry->setTags(tagsList.join(QLatin1Char(',')));

	return true;
}
