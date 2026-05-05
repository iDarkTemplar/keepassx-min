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
#include "OpData01.h"

#include "core/Group.h"
#include "core/Metadata.h"
#include "core/Tools.h"
#include "crypto/CryptoHash.h"

#include <QDebug>
#include <QJsonDocument>
#include <QJsonObject>

#include <botan/pwdhash.h>

OpVaultReader::OpVaultReader(QObject *parent)
	: QObject(parent)
{
}

QSharedPointer<Database> OpVaultReader::convert(QDir &opdataDir, const QString &password)
{
	if (!opdataDir.exists())
	{
		m_error = tr("Directory .opvault must exist");
		return {};
	}

	if (!opdataDir.isReadable())
	{
		m_error = tr("Directory .opvault must be readable");
		return {};
	}

	// https://support.1password.com/opvault-design/#directory-layout
	QDir defaultDir = QDir(opdataDir);
	if (!defaultDir.cd(QStringLiteral("default")))
	{
		m_error = tr("Directory .opvault/default must exist");
		return {};
	}

	if (!defaultDir.isReadable())
	{
		m_error = tr("Directory .opvault/default must be readable");
		return {};
	}

	auto vaultName = opdataDir.dirName();

	auto db = QSharedPointer<Database>::create();
	auto rootGroup = db->rootGroup();
	rootGroup->setName(vaultName.remove(QStringLiteral(".opvault")));

	populateCategoryGroups(rootGroup);

	QFile profileJsFile(defaultDir.absoluteFilePath(QStringLiteral("profile.js")));
	QJsonObject profileJson = readAndAssertJsonFile(profileJsFile, QStringLiteral("var profile="), QStringLiteral(";"));
	if (profileJson.isEmpty())
	{
		return {};
	}

	if (!processProfileJson(profileJson, password, rootGroup))
	{
		zeroKeys();
		return {};
	}

	QFile foldersJsFile(defaultDir.filePath(QStringLiteral("folders.js")));
	if (foldersJsFile.exists())
	{
		QJsonObject foldersJs = readAndAssertJsonFile(foldersJsFile, QStringLiteral("loadFolders("), QStringLiteral(");"));
		if (!processFolderJson(foldersJs, rootGroup))
		{
			zeroKeys();
			return {};
		}
	}

	const QString bandChars = QStringLiteral("0123456789ABCDEF");
	QString bandPattern = QStringLiteral("band_%1.js");
	for (QChar ch: bandChars)
	{
		QFile bandFile(defaultDir.filePath(bandPattern.arg(ch)));
		if (!bandFile.exists())
		{
			continue;
		}

		// https://support.1password.com/opvault-design/#band-files
		QJsonObject bandJs = readAndAssertJsonFile(bandFile, QStringLiteral("ld("), QStringLiteral(");"));
		const QStringList keys = bandJs.keys();
		for (const QString &entryKey: keys)
		{
			const QJsonObject bandEnt = bandJs[entryKey].toObject();
			const QString uuid = bandEnt[QStringLiteral("uuid")].toString();
			if (entryKey != uuid)
			{
				qWarning() << QStringLiteral("Mismatched Entry UUID, its JSON key <<%1>> and its UUID <<%2>>")
					.arg(entryKey)
					.arg(uuid);
			}

			QStringList requiredKeys({QStringLiteral("d"), QStringLiteral("k"), QStringLiteral("hmac")});
			bool ok = true;
			for (const QString &requiredKey: asConst(requiredKeys))
			{
				if (!bandEnt.contains(requiredKey))
				{
					qCritical() << "Skipping malformed Entry UUID " << uuid << " without key " << requiredKey;
					ok = false;
					break;
				}
			}

			if (!ok)
			{
				continue;
			}

			// https://support.1password.com/opvault-design/#items
			auto entry = processBandEntry(bandEnt, defaultDir, rootGroup);
			if (!entry)
			{
				qWarning() << "Unable to process Band Entry " << uuid;
			}
		}
	}

	// Remove empty categories (groups)
	for (auto group: rootGroup->children())
	{
		if (group->isEmpty())
		{
			delete group;
		}
	}

	zeroKeys();
	return db;
}

bool OpVaultReader::hasError()
{
	return !m_error.isEmpty();
}

QString OpVaultReader::errorString()
{
	return m_error;
}

bool OpVaultReader::processProfileJson(QJsonObject &profileJson, const QString &password, Group *rootGroup)
{
	unsigned long iterations = profileJson[QStringLiteral("iterations")].toInt();
	// QString lastUpdatedBy = profileJson["lastUpdatedBy"].toString();
	QString masterKeyB64 = profileJson[QStringLiteral("masterKey")].toString();
	QString overviewKeyB64 = profileJson[QStringLiteral("overviewKey")].toString();
	// QString profileName = profileJs["profileName"].toString();

	QByteArray salt;
	{
		QString saltB64 = profileJson[QStringLiteral("salt")].toString();
		salt = QByteArray::fromBase64(saltB64.toUtf8());
	}

	auto rootGroupTime = rootGroup->timeInfo();
	auto createdAt = static_cast<uint>(profileJson[QStringLiteral("createdAt")].toInt());
	rootGroupTime.setCreationTime(QDateTime::fromSecsSinceEpoch(createdAt, QTimeZone::utc()));
	auto updatedAt = static_cast<uint>(profileJson[QStringLiteral("updatedAt")].toInt());
	rootGroupTime.setLastModificationTime(QDateTime::fromSecsSinceEpoch(updatedAt, QTimeZone::utc()));
	rootGroup->setUuid(Tools::hexToUuid(profileJson[QStringLiteral("uuid")].toString()));

	QScopedPointer derivedKeys(deriveKeysFromPassPhrase(salt, password, iterations));
	if (!derivedKeys->error.isEmpty())
	{
		m_error = derivedKeys->error;
		return false;
	}

	QByteArray encKey = derivedKeys->encrypt;
	QByteArray hmacKey = derivedKeys->hmac;

	QScopedPointer masterKeys(decodeB64CompositeKeys(masterKeyB64, encKey, hmacKey));
	if (!masterKeys->error.isEmpty())
	{
		m_error = masterKeys->error;
		return false;
	}

	m_masterKey = masterKeys->encrypt;
	m_masterHmacKey = masterKeys->hmac;
	QScopedPointer overviewKeys(decodeB64CompositeKeys(overviewKeyB64, encKey, hmacKey));
	if (!overviewKeys->error.isEmpty())
	{
		m_error = overviewKeys->error;
		return false;
	}

	m_overviewKey = overviewKeys->encrypt;
	m_overviewHmacKey = overviewKeys->hmac;

	return true;
}

bool OpVaultReader::processFolderJson(QJsonObject &foldersJson, Group *rootGroup)
{
	const QStringList keys = foldersJson.keys();

	bool result = true;
	for (const QString &key: keys)
	{
		const QJsonValueRef &folderValue = foldersJson[key];
		if (!folderValue.isObject())
		{
			qWarning() << "Found non-Object folder with key \"" << key << "\"";
			continue;
		}

		const QJsonObject folder = folderValue.toObject();
		QJsonObject overviewJs;
		const QString overviewStr = folder.value(QStringLiteral("overview")).toString();
		OpData01 foldOverview01;
		if (!foldOverview01.decodeBase64(overviewStr, m_overviewKey, m_overviewHmacKey))
		{
			qCritical() << "Unable to decipher folder UUID \"" << key << "\": " << foldOverview01.errorString();
			result = false;
			continue;
		}

		auto foldOverview = foldOverview01.getClearText();
		QJsonDocument fOverJSON = QJsonDocument::fromJson(foldOverview);
		overviewJs = fOverJSON.object();

		const QString &folderTitle = overviewJs[QStringLiteral("title")].toString();
		auto myGroup = new Group();
		myGroup->setParent(rootGroup);
		myGroup->setName(folderTitle);
		if (folder.contains(QStringLiteral("uuid")))
		{
			myGroup->setUuid(Tools::hexToUuid(folder[QStringLiteral("uuid")].toString()));
		}

		if (overviewJs.contains(QStringLiteral("smart")) && overviewJs[QStringLiteral("smart")].toBool())
		{
			if (!overviewJs.contains(QStringLiteral("predicate_b64")))
			{
				QString errMsg = QStringLiteral(R"(Expected a predicate in smart folder[uuid="%1"; title="%2"]))").arg(key, folderTitle);
				qWarning() << errMsg;
				myGroup->setNotes(errMsg);
			}
			else
			{
				QByteArray pB64 = QByteArray::fromBase64(overviewJs[QStringLiteral("predicate_b64")].toString().toUtf8());
				myGroup->setNotes(QString::fromUtf8(pB64.toHex()));
			}
		}

		TimeInfo ti;
		bool timeInfoOk = false;
		if (folder.contains(QStringLiteral("created")))
		{
			auto createdTime = static_cast<uint>(folder[QStringLiteral("created")].toInt());
			ti.setCreationTime(QDateTime::fromSecsSinceEpoch(createdTime, QTimeZone::utc()));
			timeInfoOk = true;
		}

		if (folder.contains(QStringLiteral("updated")))
		{
			auto updateTime = static_cast<uint>(folder[QStringLiteral("updated")].toInt());
			ti.setLastModificationTime(QDateTime::fromSecsSinceEpoch(updateTime, QTimeZone::utc()));
			timeInfoOk = true;
		}

		// "tx" is modified by sync, not by user; maybe a custom attribute?
		if (timeInfoOk)
		{
			myGroup->setTimeInfo(ti);
		}
	}

	return result;
}

/*
 * Asserts that the given file is an existing file, able to be read, contains JSON, and that
 * the payload is a JSON object. Currently it just returns an empty QJsonObject as a means of
 * indicating the error, although it will qCritical() if unable to actually open the file for reading.
 *
 * @param file the path containing the JSON file
 * @param stripLeading any leading characters that might be present in file which should be removed
 * @param stripTrailing the trailing characters that might be present in file which should be removed
 * @return
 */
QJsonObject OpVaultReader::readAndAssertJsonFile(QFile &file, const QString &stripLeading, const QString &stripTrailing)
{
	QByteArray filePayload;
	const QFileInfo &fileInfo = QFileInfo(file);
	auto absFilePath = fileInfo.absoluteFilePath();
	if (!fileInfo.exists())
	{
		qCritical() << QObject::tr("File \"%1\" must exist").arg(absFilePath);
		return QJsonObject();
	}

	if (!fileInfo.isReadable())
	{
		qCritical() << QObject::tr("File \"%1\" must be readable").arg(absFilePath);
		return QJsonObject();
	}

	if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
	{
		qCritical() << QObject::tr("Unable to open \"%1\" readonly+text").arg(absFilePath);
	}

	filePayload = file.readAll();
	file.close();
	if (!stripLeading.isEmpty())
	{
		QByteArray prefix = stripLeading.toUtf8();
		if (filePayload.startsWith(prefix))
		{
			filePayload = filePayload.remove(0, prefix.size());
		}
	}

	if (!stripTrailing.isEmpty())
	{
		QByteArray suffix = stripTrailing.toUtf8();
		if (filePayload.endsWith(suffix))
		{
			const int delBytes = suffix.size();
			filePayload = filePayload.remove(filePayload.length() - delBytes, delBytes);
		}
	}

	QJsonParseError *error = Q_NULLPTR;
	QJsonDocument jDoc = QJsonDocument::fromJson(filePayload, error);
	if (!jDoc.isObject())
	{
		qCritical() << "Expected " << filePayload << "to be a JSON Object";
		return QJsonObject();
	}

	return jDoc.object();
}

/* Convenience method for calling decodeCompositeKeys when you have a base64 encrypted composite key. */
OpVaultReader::DerivedKeyHMAC* OpVaultReader::decodeB64CompositeKeys(const QString &b64, const QByteArray &encKey, const QByteArray &hmacKey)
{

	OpData01 keyKey01;
	if (!keyKey01.decodeBase64(b64, encKey, hmacKey))
	{
		auto result = new DerivedKeyHMAC();
		result->error = tr("Unable to decode masterKey: %1").arg(keyKey01.errorString());
		return result;
	}

	const QByteArray keyKey = keyKey01.getClearText();

	return decodeCompositeKeys(keyKey);
}

/*
 * Given a string of bytes, decompose it into its constituent parts, an encryption key and a HMAC key.
 * The plaintext of the masterKey is 256 bytes of data selected randomly when the keychain was first created.
 *
 * The 256 byte (2048 bit) plaintext content of the masterKey is then hashed with SHA-512.
 * The first 32 bytes (256-bits) of the resulting hash are the master encryption key,
 * and the second 32 bytes are the master hmac key.
 */
OpVaultReader::DerivedKeyHMAC* OpVaultReader::decodeCompositeKeys(const QByteArray &keyKey)
{
	auto result = new DerivedKeyHMAC;

	auto digest = CryptoHash::hash(keyKey, CryptoHash::Sha512);
	result->encrypt = digest.left(32);
	result->hmac = digest.right(32);

	return result;
}

/*
 * Translates the provided salt and passphrase into a derived set of keys, one for encryption
 * and one for use as a HMAC key. See https://support.1password.com/opvault-design/#key-derivation
 * @param iterations the number of rounds to apply the derivation formula
 * @return a non-null structure containing either the error or the two password-derived keys
 */
OpVaultReader::DerivedKeyHMAC* OpVaultReader::deriveKeysFromPassPhrase(QByteArray &salt, const QString &password, unsigned long iterations)
{
	auto result = new DerivedKeyHMAC;

	QByteArray out(64, '\0');
	try
	{
		auto pwhash = Botan::PasswordHashFamily::create_or_throw("PBKDF2(SHA-512)")->from_iterations(iterations);
		pwhash->derive_key(reinterpret_cast<uint8_t*>(out.data()),
			out.size(),
			password.toUtf8().constData(),
			password.size(),
			reinterpret_cast<const uint8_t*>(salt.constData()),
			salt.size());
	}
	catch (const std::exception &e)
	{
		result->error = tr("Unable to derive master key: %1").arg(e.what());
		return result;
	}

	result->encrypt = out.left(32);
	result->hmac = out.right(32);

	return result;
}

/*!
 * \sa https://support.1password.com/opvault-design/#category
 */
void OpVaultReader::populateCategoryGroups(Group *rootGroup)
{
	QMap<QString, QString> categoryMap;

	categoryMap.insert(QStringLiteral("001"), QStringLiteral("Login"));
	categoryMap.insert(QStringLiteral("002"), QStringLiteral("Credit Card"));
	categoryMap.insert(QStringLiteral("003"), QStringLiteral("Secure Note"));
	categoryMap.insert(QStringLiteral("004"), QStringLiteral("Identity"));
	categoryMap.insert(QStringLiteral("005"), QStringLiteral("Password"));
	categoryMap.insert(QStringLiteral("099"), QStringLiteral("Tombstone"));
	categoryMap.insert(QStringLiteral("100"), QStringLiteral("Software License"));
	categoryMap.insert(QStringLiteral("101"), QStringLiteral("Bank Account"));
	categoryMap.insert(QStringLiteral("102"), QStringLiteral("Database"));
	categoryMap.insert(QStringLiteral("103"), QStringLiteral("Driver License"));
	categoryMap.insert(QStringLiteral("104"), QStringLiteral("Outdoor License"));
	categoryMap.insert(QStringLiteral("105"), QStringLiteral("Membership"));
	categoryMap.insert(QStringLiteral("106"), QStringLiteral("Passport"));
	categoryMap.insert(QStringLiteral("107"), QStringLiteral("Rewards"));
	categoryMap.insert(QStringLiteral("108"), QStringLiteral("SSN"));
	categoryMap.insert(QStringLiteral("109"), QStringLiteral("Router"));
	categoryMap.insert(QStringLiteral("110"), QStringLiteral("Server"));
	categoryMap.insert(QStringLiteral("111"), QStringLiteral("Email"));

	for (const QString &catNum: categoryMap.keys())
	{
		const QString &category = categoryMap[catNum];
		auto g = new Group();
		g->setName(category);
		g->setProperty("code", catNum);
		g->setUpdateTimeinfo(false);
		// maybe make these stable, so folks can depend on them?
		g->setUuid(QUuid::createUuid());
		g->setParent(rootGroup);
	}
}

void OpVaultReader::zeroKeys()
{
	m_masterKey.fill('\0');
	m_masterHmacKey.fill('\0');
	m_overviewKey.fill('\0');
	m_overviewHmacKey.fill('\0');
}
