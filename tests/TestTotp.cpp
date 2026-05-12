/*
 *  Copyright (C) 2017 Weslly Honorato <﻿weslly@protonmail.com>
 *  Copyright (C) 2017 KeePassXC Team <team@keepassxc.org>
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

#include "TestTotp.h"

#include "core/Entry.h"
#include "core/Totp.h"
#include "crypto/Crypto.h"

#include <QTest>

QTEST_GUILESS_MAIN(TestTotp)

void TestTotp::initTestCase()
{
	QVERIFY(Crypto::init());
}

void TestTotp::testParseSecret()
{
	// OTP URL Parsing
	QString secret = QStringLiteral(
		"otpauth://totp/"
		"ACME%20Co:john@example.com?secret=HXDMVJECJJWSRB3HWIZR4IFUGFTMXBOZ&issuer=ACME%20Co&algorithm="
		"SHA1&digits=6&period=30");
	auto settings = Totp::parseSettings(secret);
	QVERIFY(!settings.isNull());
	QCOMPARE(settings->key, QStringLiteral("HXDMVJECJJWSRB3HWIZR4IFUGFTMXBOZ"));
	QCOMPARE(settings->format, Totp::StorageFormat::OTPURL);
	QCOMPARE(settings->digits, 6u);
	QCOMPARE(settings->step, 30u);
	QCOMPARE(settings->algorithm, Totp::Algorithm::Sha1);
	QCOMPARE(Totp::hasCustomSettings(settings), false);

	// OTP URL with non-default hash type
	secret = QStringLiteral(
		"otpauth://totp/"
		"ACME%20Co:john@example.com?secret=HXDMVJECJJWSRB3HWIZR4IFUGFTMXBOZ&issuer=ACME%20Co&algorithm="
		"SHA512&digits=6&period=30");
	settings = Totp::parseSettings(secret);
	QVERIFY(!settings.isNull());
	QCOMPARE(settings->key, QStringLiteral("HXDMVJECJJWSRB3HWIZR4IFUGFTMXBOZ"));
	QCOMPARE(settings->format, Totp::StorageFormat::OTPURL);
	QCOMPARE(settings->digits, 6u);
	QCOMPARE(settings->step, 30u);
	QCOMPARE(settings->algorithm, Totp::Algorithm::Sha512);
	QCOMPARE(Totp::hasCustomSettings(settings), true);

	// Max TOTP step of 24-hours
	secret.replace(QStringLiteral("period=30"), QStringLiteral("period=90000"));
	settings = Totp::parseSettings(secret);
	QVERIFY(!settings.isNull());
	QCOMPARE(settings->step, 86400u);

	// KeeOTP Parsing
	secret = QStringLiteral("key=HXDMVJECJJWSRBY%3d&step=25&size=8&otpHashMode=Sha256");
	settings = Totp::parseSettings(secret);
	QVERIFY(!settings.isNull());
	QCOMPARE(settings->key, QStringLiteral("HXDMVJECJJWSRBY="));
	QCOMPARE(settings->format, Totp::StorageFormat::KEEOTP);
	QCOMPARE(settings->digits, 8u);
	QCOMPARE(settings->step, 25u);
	QCOMPARE(settings->algorithm, Totp::Algorithm::Sha256);
	QCOMPARE(Totp::hasCustomSettings(settings), true);

	// Semi-colon delineated "TOTP Settings"
	secret = QStringLiteral("gezdgnbvgy3tqojqgezdgnbvgy3tqojq");
	settings = Totp::parseSettings(QStringLiteral("30;8"), secret);
	QVERIFY(!settings.isNull());
	QCOMPARE(settings->key, QStringLiteral("gezdgnbvgy3tqojqgezdgnbvgy3tqojq"));
	QCOMPARE(settings->format, Totp::StorageFormat::LEGACY);
	QCOMPARE(settings->digits, 8u);
	QCOMPARE(settings->step, 30u);
	QCOMPARE(settings->algorithm, Totp::Algorithm::Sha1);
	QCOMPARE(Totp::hasCustomSettings(settings), true);

	// Bare secret (no "TOTP Settings" attribute)
	secret = QStringLiteral("gezdgnbvgy3tqojqgezdgnbvgy3tqojq");
	settings = Totp::parseSettings(QString(), secret);
	QVERIFY(!settings.isNull());
	QCOMPARE(settings->key, QStringLiteral("gezdgnbvgy3tqojqgezdgnbvgy3tqojq"));
	QCOMPARE(settings->format, Totp::StorageFormat::LEGACY);
	QCOMPARE(settings->digits, 6u);
	QCOMPARE(settings->step, 30u);
	QCOMPARE(settings->algorithm, Totp::Algorithm::Sha1);
	QCOMPARE(Totp::hasCustomSettings(settings), false);

	// Blank settings (expected failure)
	settings = Totp::parseSettings(QString(), QString());
	QVERIFY(settings.isNull());

	// TOTP Settings with blank secret (expected failure)
	settings = Totp::parseSettings(QStringLiteral("30;8"), QString());
	QVERIFY(settings.isNull());
}

void TestTotp::testTotpCode()
{
	// Test vectors from RFC 6238
	// https://tools.ietf.org/html/rfc6238#appendix-B
	auto settings = Totp::createSettings(QStringLiteral("GEZDGNBVGY3TQOJQGEZDGNBVGY3TQOJQ"), Totp::DEFAULT_DIGITS, Totp::DEFAULT_STEP);

	// Test 6 digit TOTP (default)
	quint64 time = 1234567890;
	QCOMPARE(Totp::generateTotp(settings, nullptr, time), QStringLiteral("005924"));

	time = 1111111109;
	QCOMPARE(Totp::generateTotp(settings, nullptr, time), QStringLiteral("081804"));

	// Test 8 digit TOTP (custom)
	settings->digits = 8;
	time = 1111111111;
	QCOMPARE(Totp::generateTotp(settings, nullptr, time), QStringLiteral("14050471"));

	time = 2000000000;
	QCOMPARE(Totp::generateTotp(settings, nullptr, time), QStringLiteral("69279037"));
}

void TestTotp::testSteamTotp()
{
	// Legacy parsing
	auto settings = Totp::parseSettings(QStringLiteral("30;S"), QStringLiteral("63BEDWCQZKTQWPESARIERL5DTTQFCJTK"));
	QCOMPARE(settings->key, QStringLiteral("63BEDWCQZKTQWPESARIERL5DTTQFCJTK"));
	QCOMPARE(settings->encoder.shortName, Totp::STEAM_SHORTNAME);
	QCOMPARE(settings->format, Totp::StorageFormat::LEGACY);
	QCOMPARE(settings->digits, Totp::STEAM_DIGITS);
	QCOMPARE(settings->step, 30u);

	// OTP URL Parsing
	QString secret = QStringLiteral(
		"otpauth://totp/"
		"test:test@example.com?secret=63BEDWCQZKTQWPESARIERL5DTTQFCJTK&issuer=Valve&algorithm="
		"SHA1&digits=5&period=30&encoder=steam");
	settings = Totp::parseSettings(secret);

	QCOMPARE(settings->key, QStringLiteral("63BEDWCQZKTQWPESARIERL5DTTQFCJTK"));
	QCOMPARE(settings->encoder.shortName, Totp::STEAM_SHORTNAME);
	QCOMPARE(settings->format, Totp::StorageFormat::OTPURL);
	QCOMPARE(settings->digits, Totp::STEAM_DIGITS);
	QCOMPARE(settings->step, 30u);

	// These time/value pairs were created by running the Steam Guard function of the
	// Steam mobile app with a throw-away steam account. The above secret was extracted
	// from the Steam app's data for use in testing here.
	quint64 time = 1511200518;
	QCOMPARE(Totp::generateTotp(settings, nullptr, time), QStringLiteral("FR8RV"));
	time = 1511200714;
	QCOMPARE(Totp::generateTotp(settings, nullptr, time), QStringLiteral("9P3VP"));
}

void TestTotp::testEntryHistory()
{
	Entry entry;
	uint step = 16;
	uint digits = 6;
	auto settings = Totp::createSettings(QStringLiteral("GEZDGNBVGY3TQOJQGEZDGNBVGY3TQOJQ"), digits, step);
	// Test that entry starts without TOTP
	QCOMPARE(entry.historyItems().size(), 0);
	QVERIFY(!entry.hasTotp());
	// Add TOTP to entry
	entry.setTotp(settings);
	QCOMPARE(entry.historyItems().size(), 1);
	QVERIFY(entry.hasTotp());
	QCOMPARE(entry.totpSettings()->key, QStringLiteral("GEZDGNBVGY3TQOJQGEZDGNBVGY3TQOJQ"));
	// Change key and verify settings changed
	settings->key = QStringLiteral("foo");
	entry.setTotp(settings);
	QCOMPARE(entry.historyItems().size(), 2);
	QCOMPARE(entry.totpSettings()->key, QStringLiteral("foo"));
	// Nullptr Settings (expected reset of TOTP)
	entry.setTotp(nullptr);
	QVERIFY(!entry.hasTotp());
	QCOMPARE(entry.historyItems().size(), 3);
}

void TestTotp::testKeePass2()
{
	Entry entry;
	auto attr = entry.attributes();

	// Default settings
	attr->set(QStringLiteral("TimeOtp-Secret-Base32"), QStringLiteral("GEZDGNBVGY3TQOJQGEZDGNBVGY3TQOJQ"));

	auto settings = entry.totpSettings();
	QVERIFY(settings);
	QCOMPARE(settings->key, QStringLiteral("GEZDGNBVGY3TQOJQGEZDGNBVGY3TQOJQ"));
	QCOMPARE(settings->algorithm, Totp::Algorithm::Sha1);
	QCOMPARE(settings->digits, 6u);
	QCOMPARE(settings->step, 30u);
	QCOMPARE(Totp::hasCustomSettings(settings), false);

	// Custom settings
	attr->set(QStringLiteral("TimeOtp-Algorithm"), QStringLiteral("HMAC-SHA-256"));
	attr->set(QStringLiteral("TimeOtp-Length"), QStringLiteral("8"));

	settings = entry.totpSettings();
	QVERIFY(settings);
	QCOMPARE(settings->key, QStringLiteral("GEZDGNBVGY3TQOJQGEZDGNBVGY3TQOJQ"));
	QCOMPARE(settings->algorithm, Totp::Algorithm::Sha256);
	QCOMPARE(settings->digits, 8u);
	QCOMPARE(settings->step, 30u);
	QCOMPARE(Totp::hasCustomSettings(settings), true);

	// Base64 and other encodings are not supported
	attr->remove(QStringLiteral("TimeOtp-Secret-Base32"));
	attr->set(QStringLiteral("TimeOtp-Secret-Base64"), QStringLiteral("GEZDGNBVGY3TQOJQGEZDGNBVGY3TQOJQ"));

	settings = entry.totpSettings();
	QVERIFY(!settings);
}
