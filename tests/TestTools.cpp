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

#include "TestTools.h"

#include "core/Clock.h"
#include "core/Tools.h"

#include <QFileInfo>
#include <QRegularExpression>
#include <QTest>
#include <QUuid>

QTEST_GUILESS_MAIN(TestTools)

namespace
{
	QString createDecimal(const QString &wholes, const QString &fractions, const QString &unit)
	{
		return wholes + QLocale().decimalPoint() + fractions + QStringLiteral(" ") + unit;
	}
} // namespace

void TestTools::testHumanReadableFileSize()
{
	constexpr auto kibibyte = 1024u;
	using namespace Tools;

	QCOMPARE(QStringLiteral("1 B"), humanReadableFileSize(1));
	QCOMPARE(createDecimal(QStringLiteral("1"), QStringLiteral("00"), QStringLiteral("KiB")), humanReadableFileSize(kibibyte));
	QCOMPARE(createDecimal(QStringLiteral("1"), QStringLiteral("00"), QStringLiteral("MiB")), humanReadableFileSize(kibibyte * kibibyte));
	QCOMPARE(createDecimal(QStringLiteral("1"), QStringLiteral("00"), QStringLiteral("GiB")), humanReadableFileSize(kibibyte * kibibyte * kibibyte));

	QCOMPARE(QStringLiteral("100 B"), humanReadableFileSize(100, 0));
	QCOMPARE(createDecimal(QStringLiteral("1"), QStringLiteral("10"), QStringLiteral("KiB")), humanReadableFileSize(kibibyte + 100));
	QCOMPARE(createDecimal(QStringLiteral("1"), QStringLiteral("001"), QStringLiteral("KiB")), humanReadableFileSize(kibibyte + 1, 3));
	QCOMPARE(createDecimal(QStringLiteral("15"), QStringLiteral("00"), QStringLiteral("KiB")), humanReadableFileSize(kibibyte * 15));
}

void TestTools::testIsHex()
{
	QVERIFY(Tools::isHex("0123456789abcdefABCDEF"));
	QVERIFY(!Tools::isHex(QByteArray("0xnothex")));
}

void TestTools::testIsBase64()
{
	QVERIFY(Tools::isBase64(QByteArray("1234")));
	QVERIFY(Tools::isBase64(QByteArray("123=")));
	QVERIFY(Tools::isBase64(QByteArray("12==")));
	QVERIFY(Tools::isBase64(QByteArray("abcd9876MN==")));
	QVERIFY(Tools::isBase64(QByteArray("abcd9876DEFGhijkMNO=")));
	QVERIFY(Tools::isBase64(QByteArray("abcd987/DEFGh+jk/NO=")));
	QVERIFY(!Tools::isBase64(QByteArray("abcd123==")));
	QVERIFY(!Tools::isBase64(QByteArray("abc_")));
	QVERIFY(!Tools::isBase64(QByteArray("123")));
}

void TestTools::testIsAsciiString()
{
	QVERIFY(Tools::isAsciiString(QStringLiteral("abcd9876DEFGhijkMNO")));
	QVERIFY(Tools::isAsciiString(QStringLiteral("-!&5a?`~")));
	QVERIFY(!Tools::isAsciiString(QStringLiteral("Štest")));
	QVERIFY(!Tools::isAsciiString(QStringLiteral("Ãß")));
}

void TestTools::testValidUuid()
{
	auto validUuid = Tools::uuidToHex(QUuid::createUuid());
	auto nonRfc4122Uuid = QStringLiteral("1234567890abcdef1234567890abcdef");
	auto emptyUuid = QString();
	auto shortUuid = validUuid.left(10);
	auto longUuid = validUuid + QStringLiteral("baddata");
	auto nonHexUuid = Tools::uuidToHex(QUuid::createUuid()).replace(0, 1, QLatin1Char('p'));

	QVERIFY(Tools::isValidUuid(validUuid));
	/* Before https://github.com/keepassxreboot/keepassxc/pull/1770/, entry
	 * UUIDs are simply random 16-byte strings. Such older entries should be
	 * accepted as well. */
	QVERIFY(Tools::isValidUuid(nonRfc4122Uuid));
	QVERIFY(!Tools::isValidUuid(emptyUuid));
	QVERIFY(!Tools::isValidUuid(shortUuid));
	QVERIFY(!Tools::isValidUuid(longUuid));
	QVERIFY(!Tools::isValidUuid(nonHexUuid));
}

void TestTools::testBackupFilePatternSubstitution_data()
{
	QTest::addColumn<QString>("pattern");
	QTest::addColumn<QString>("dbFilePath");
	QTest::addColumn<QString>("expectedSubstitution");

	static const auto DEFAULT_DB_FILE_NAME = QStringLiteral("KeePassXC");
	static const auto DEFAULT_DB_FILE_PATH = QStringLiteral("/tmp/") + DEFAULT_DB_FILE_NAME + QStringLiteral(".kdbx");
	static const auto NOW = Clock::currentDateTime();
	auto DEFAULT_FORMATTED_TIME = NOW.toString(QStringLiteral("dd_MM_yyyy_hh-mm-ss"));

	QTest::newRow("Null pattern") << QString() << DEFAULT_DB_FILE_PATH << QString();
	QTest::newRow("Empty pattern") << QString() << DEFAULT_DB_FILE_PATH << QString();
	QTest::newRow("Null database path") << "valid_pattern" << QString() << QString();
	QTest::newRow("Empty database path") << "valid_pattern" << QString() << QString();
	QTest::newRow("Unclosed/invalid pattern") << "{DB_FILENAME" << DEFAULT_DB_FILE_PATH << "{DB_FILENAME";
	QTest::newRow("Unknown pattern") << "{NO_MATCH}" << DEFAULT_DB_FILE_PATH << "{NO_MATCH}";
	QTest::newRow("Do not replace escaped patterns (filename)")
		<< "\\{DB_FILENAME\\}" << DEFAULT_DB_FILE_PATH << "{DB_FILENAME}";
	QTest::newRow("Do not replace escaped patterns (time)")
		<< "\\{TIME:dd.MM.yyyy\\}" << DEFAULT_DB_FILE_PATH << "{TIME:dd.MM.yyyy}";
	QTest::newRow("Multiple patterns should be replaced")
		<< "{DB_FILENAME} {TIME} {DB_FILENAME}" << DEFAULT_DB_FILE_PATH
		<< DEFAULT_DB_FILE_NAME + QStringLiteral(" ") + DEFAULT_FORMATTED_TIME + QStringLiteral(" ")
			   + DEFAULT_DB_FILE_NAME;
	QTest::newRow("Default time pattern") << "{TIME}" << DEFAULT_DB_FILE_PATH << DEFAULT_FORMATTED_TIME;
	QTest::newRow("Default time pattern (empty formatter)")
		<< "{TIME:}" << DEFAULT_DB_FILE_PATH << DEFAULT_FORMATTED_TIME;
	QTest::newRow("Custom time pattern") << "{TIME:dd-ss}" << DEFAULT_DB_FILE_PATH << NOW.toString(QStringLiteral("dd-ss"));
	QTest::newRow("Time pattern twice") << "{TIME:yy} {TIME}" << DEFAULT_DB_FILE_PATH
										<< NOW.toString(QStringLiteral("yy")) + QStringLiteral(" ") + DEFAULT_FORMATTED_TIME;
	QTest::newRow("Complex custom time pattern")
		<< "./{TIME:yy}/{DB_FILENAME}_{TIME:yyyyMMdd_HHmmss}.old.kdbx" << DEFAULT_DB_FILE_PATH
		<< QStringLiteral("./") + NOW.toString(QStringLiteral("yy")) + QStringLiteral("/") + DEFAULT_DB_FILE_NAME + QStringLiteral("_")
			   + NOW.toString(QStringLiteral("yyyyMMdd_HHmmss")) + QStringLiteral(".old.kdbx");
	QTest::newRow("Invalid custom time pattern") << "{TIME:dd/-ss}" << DEFAULT_DB_FILE_PATH << NOW.toString(QStringLiteral("dd/-ss"));
	QTest::newRow("Recursive substitution") << "{TIME:'{TIME}'}" << DEFAULT_DB_FILE_PATH << DEFAULT_FORMATTED_TIME;
	QTest::newRow("{DB_FILENAME} substitution")
		<< "some {DB_FILENAME} thing" << DEFAULT_DB_FILE_PATH
		<< QStringLiteral("some ") + DEFAULT_DB_FILE_NAME + QStringLiteral(" thing");
	QTest::newRow("{DB_FILENAME} substitution with multiple extensions")
		<< "some {DB_FILENAME} thing" << "/tmp/KeePassXC.kdbx.ext" << "some KeePassXC.kdbx thing";
	// Not relevant right now, added test anyway
	QTest::newRow("There should be no substitution loops")
		<< "{DB_FILENAME}" << "{TIME:'{DB_FILENAME}'}.ext" << "{TIME:'{DB_FILENAME}'}";
}

void TestTools::testBackupFilePatternSubstitution()
{
	QFETCH(QString, pattern);
	QFETCH(QString, dbFilePath);
	QFETCH(QString, expectedSubstitution);

	QCOMPARE(Tools::substituteBackupFilePath(pattern, dbFilePath), expectedSubstitution);
}

void TestTools::testEscapeRegex_data()
{
	QTest::addColumn<QString>("input");
	QTest::addColumn<QString>("expected");

	QString all_regular_characters = QStringLiteral("0123456789");
	for (char c = 'a'; c != 'z'; ++c)
	{
		all_regular_characters += QChar::fromLatin1(c);
	}
	for (char c = 'A'; c != 'Z'; ++c)
	{
		all_regular_characters += QChar::fromLatin1(c);
	}

	QTest::newRow("Regular characters should not be escaped") << all_regular_characters << all_regular_characters;
	QTest::newRow("Special characters should be escaped")
		<< R"(.^$*+-?()[]{}|\)" << R"(\.\^\$\*\+\-\?\(\)\[\]\{\}\|\\)";
	QTest::newRow("Null character") << QString::fromLatin1("ab\0c", 4) << "ab\\0c";
}

void TestTools::testEscapeRegex()
{
	QFETCH(QString, input);
	QFETCH(QString, expected);

	auto actual = Tools::escapeRegex(input);
	QCOMPARE(actual, expected);
}

void TestTools::testConvertToRegex()
{
	QFETCH(QString, input);
	QFETCH(int, options);
	QFETCH(QString, expected);

	auto regex = Tools::convertToRegex(input, options).pattern();
	QCOMPARE(regex, expected);
}

void TestTools::testConvertToRegex_data()
{
	const QString input = QStringLiteral(R"(te|st*t?[5]^(test);',.)");

	QTest::addColumn<QString>("input");
	QTest::addColumn<int>("options");
	QTest::addColumn<QString>("expected");

	QTest::newRow("No Options") << input << static_cast<int>(Tools::RegexConvertOpts::DEFAULT)
								<< QStringLiteral(R"(te|st*t?[5]^(test);',.)");
	// Escape regex
	QTest::newRow("Escape Regex") << input << static_cast<int>(Tools::RegexConvertOpts::ESCAPE_REGEX)
								  << Tools::escapeRegex(input);
	QTest::newRow("Escape Regex and exact match")
		<< input << static_cast<int>(Tools::RegexConvertOpts::ESCAPE_REGEX | Tools::RegexConvertOpts::EXACT_MATCH)
		<< QStringLiteral("^(?:") + Tools::escapeRegex(input) + QStringLiteral(")$");

	// Exact match does not escape the pattern
	QTest::newRow("Exact Match") << input << static_cast<int>(Tools::RegexConvertOpts::EXACT_MATCH)
								 << QStringLiteral(R"(^(?:te|st*t?[5]^(test);',.)$)");

	// Exact match with improper regex
	QTest::newRow("Exact Match") << ")av(" << static_cast<int>(Tools::RegexConvertOpts::EXACT_MATCH)
								 << QStringLiteral(R"(^(?:)av()$)");

	QTest::newRow("Exact Match & Wildcard")
		<< input << static_cast<int>(Tools::RegexConvertOpts::EXACT_MATCH | Tools::RegexConvertOpts::WILDCARD_ALL)
		<< QStringLiteral(R"(^(?:te|st.*t.\[5\]\^\(test\)\;\'\,\.)$)");
	QTest::newRow("Wildcard Single Match") << input << static_cast<int>(Tools::RegexConvertOpts::WILDCARD_SINGLE_MATCH)
										   << QStringLiteral(R"(te\|st\*t.\[5\]\^\(test\)\;\'\,\.)");
	QTest::newRow("Wildcard OR") << input << static_cast<int>(Tools::RegexConvertOpts::WILDCARD_LOGICAL_OR)
								 << QStringLiteral(R"(te|st\*t\?\[5\]\^\(test\)\;\'\,\.)");
	QTest::newRow("Wildcard Unlimited Match")
		<< input << static_cast<int>(Tools::RegexConvertOpts::WILDCARD_UNLIMITED_MATCH)
		<< QStringLiteral(R"(te\|st.*t\?\[5\]\^\(test\)\;\'\,\.)");
}

void TestTools::testArrayContainsValues()
{
	const auto values = QStringList() << QStringLiteral("first") << QStringLiteral("second") << QStringLiteral("third");

	// One missing
	const auto result1 = Tools::getMissingValuesFromList<QString>(values, QStringList() << QStringLiteral("first") << QStringLiteral("second") << QStringLiteral("none"));
	QCOMPARE(result1.length(), 1);
	QCOMPARE(result1.first(), QStringLiteral("none"));

	// All found
	const auto result2 = Tools::getMissingValuesFromList<QString>(values, QStringList() << QStringLiteral("first") << QStringLiteral("second") << QStringLiteral("third"));
	QCOMPARE(result2.length(), 0);

	// None are found
	const auto numberValues = QList<int>({1, 2, 3, 4, 5});
	const auto result3 = Tools::getMissingValuesFromList<int>(numberValues, QList<int>({6, 7, 8}));
	QCOMPARE(result3.length(), 3);
}

void TestTools::testMimeTypes()
{
	const QStringList TextMimeTypes = {
		QStringLiteral("text/plain"), // Plain text
		QStringLiteral("text/css"), // CSS stylesheets
		QStringLiteral("text/javascript"), // JavaScript files
		QStringLiteral("text/xml"), // XML documents
		QStringLiteral("text/rtf"), // Rich Text Format
		QStringLiteral("text/vcard"), // vCard files
		QStringLiteral("text/tab-separated-values"), // Tab-separated values
		QStringLiteral("application/json"), // JSON data
		QStringLiteral("application/xml"), // XML data
		QStringLiteral("application/soap+xml"), // SOAP messages
		QStringLiteral("application/x-yaml"), // YAML data
		QStringLiteral("application/protobuf"), // Protocol Buffers
	};

	const QStringList ImageMimeTypes = {
		QStringLiteral("image/jpeg"), // JPEG images
		QStringLiteral("image/png"), // PNG images
		QStringLiteral("image/gif"), // GIF images
		QStringLiteral("image/bmp"), // BMP images
		QStringLiteral("image/webp"), // WEBP images
		QStringLiteral("image/svg+xml") // SVG images
	};

	const QStringList UnknownMimeTypes = {
		QStringLiteral("audio/mpeg"), // MPEG audio files
		QStringLiteral("video/mp4"), // MP4 video files
		QStringLiteral("application/pdf"), // PDF documents
		QStringLiteral("application/zip"), // ZIP archives
		QStringLiteral("application/x-tar"), // TAR archives
		QStringLiteral("application/x-rar-compressed"), // RAR archives
		QStringLiteral("application/x-7z-compressed"), // 7z archives
		QStringLiteral("application/x-shockwave-flash"), // Adobe Flash files
		QStringLiteral("application/vnd.ms-excel"), // Microsoft Excel files
		QStringLiteral("application/vnd.openxmlformats-officedocument.spreadsheetml.sheet"), // Microsoft Excel (OpenXML) files
		QStringLiteral("application/vnd.ms-powerpoint"), // Microsoft PowerPoint files
		QStringLiteral("application/vnd.openxmlformats-officedocument.presentationml.presentation"), // Microsoft PowerPoint (OpenXML) files
		QStringLiteral("application/msword"), // Microsoft Word files
		QStringLiteral("application/vnd.openxmlformats-officedocument.wordprocessingml.document"), // Microsoft Word (OpenXML) files
		QStringLiteral("application/vnd.oasis.opendocument.text"), // OpenDocument Text
		QStringLiteral("application/vnd.oasis.opendocument.spreadsheet"), // OpenDocument Spreadsheet
		QStringLiteral("application/vnd.oasis.opendocument.presentation"), // OpenDocument Presentation
		QStringLiteral("application/x-httpd-php"), // PHP files
		QStringLiteral("application/x-perl"), // Perl scripts
		QStringLiteral("application/x-python"), // Python scripts
		QStringLiteral("application/x-ruby"), // Ruby scripts
		QStringLiteral("application/x-shellscript"), // Shell scripts
	};

	QCOMPARE(Tools::toMimeType(QStringLiteral("text/html")), Tools::MimeType::Html);
	QCOMPARE(Tools::toMimeType(QStringLiteral("text/markdown")), Tools::MimeType::Markdown);

	for (const auto &mime: TextMimeTypes)
	{
		QCOMPARE(Tools::toMimeType(mime), Tools::MimeType::PlainText);
	}

	for (const auto &mime: ImageMimeTypes)
	{
		QCOMPARE(Tools::toMimeType(mime), Tools::MimeType::Image);
	}

	for (const auto &mime: UnknownMimeTypes)
	{
		QCOMPARE(Tools::toMimeType(mime), Tools::MimeType::Unknown);
	}
}

void TestTools::testGetMimeType()
{
	const QStringList Text = {QStringLiteral("0x42"), QString()};

	for (const auto &text: Text)
	{
		QCOMPARE(Tools::getMimeType(text.toUtf8()), Tools::MimeType::PlainText);
	}

	const QByteArrayList ImageHeaders = {
		// JPEG: starts with 0xFF 0xD8 0xFF (Start of Image marker)
		QByteArray::fromHex("FFD8FF"),
		// PNG: starts with 0x89 0x50 0x4E 0x47 0D 0A 1A 0A (PNG signature)
		QByteArray::fromHex("89504E470D0A1A0A"),
		// GIF87a: original GIF format (1987 standard)
		QByteArray("GIF87a"),
		// GIF89a: extended GIF format (1989, supports animation, transparency, etc.)
		QByteArray("GIF89a"),
	};

	for (const auto &image: ImageHeaders)
	{
		QCOMPARE(Tools::getMimeType(image), Tools::MimeType::Image);
	}

	const QByteArrayList UnknownHeaders = {
		// MP3: typically starts with ID3 tag (ID3v2)
		QByteArray("ID3"),
		// MP4: usually starts with a 'ftyp' box (ISO base media file format)
		// Common major brands: isom, mp42, avc1, etc.
		QByteArray::fromHex("000000186674797069736F6D"), // size + 'ftyp' + 'isom'
		// PDF: starts with "%PDF-" followed by version (e.g., %PDF-1.7)
		QByteArray("%PDF-"),
	};

	for (const auto &unknown: UnknownHeaders)
	{
		QCOMPARE(Tools::getMimeType(unknown), Tools::MimeType::Unknown);
	}
}

void TestTools::testGetMimeTypeByFileInfo()
{
	const QStringList Text = {QStringLiteral("test.txt"), QStringLiteral("test.csv"), QStringLiteral("test.xml"), QStringLiteral("test.json")};

	for (const auto &text: Text)
	{
		QCOMPARE(Tools::getMimeType(QFileInfo(text)), Tools::MimeType::PlainText);
	}

	const QStringList Images = {QStringLiteral("test.jpg"), QStringLiteral("test.png"), QStringLiteral("test.bmp"), QStringLiteral("test.svg")};

	for (const auto &image: Images)
	{
		QCOMPARE(Tools::getMimeType(QFileInfo(image)), Tools::MimeType::Image);
	}

	const QStringList Htmls = {QStringLiteral("test.html"), QStringLiteral("test.htm")};

	for (const auto &html: Htmls)
	{
		QCOMPARE(Tools::getMimeType(QFileInfo(html)), Tools::MimeType::Html);
	}

	const QStringList Markdowns = {QStringLiteral("test.md"), QStringLiteral("test.markdown")};

	for (const auto &markdown: Markdowns)
	{
		QCOMPARE(Tools::getMimeType(QFileInfo(markdown)), Tools::MimeType::Markdown);
	}

	const QStringList UnknownHeaders = {QStringLiteral("test.doc"), QStringLiteral("test.pdf"), QStringLiteral("test.docx")};

	for (const auto &unknown: UnknownHeaders)
	{
		QCOMPARE(Tools::getMimeType(QFileInfo(unknown)), Tools::MimeType::Unknown);
	}
}

void TestTools::testIsTextMimeType()
{
	const auto Text = {Tools::MimeType::PlainText, Tools::MimeType::Html, Tools::MimeType::Markdown};

	for (const auto &text: Text)
	{
		QVERIFY(Tools::isTextMimeType(text));
	}

	const auto NoText = {Tools::MimeType::Image, Tools::MimeType::Unknown};

	for (const auto &noText: NoText)
	{
		QVERIFY(!Tools::isTextMimeType(noText));
	}
}

// Test sanitization logic for Tools::cleanUsername
void TestTools::testCleanUsername()
{
	// Test vars
	QFETCH(QString, input);
	QFETCH(QString, expected);

	qputenv("USER", input.toUtf8());
	qputenv("USERNAME", input.toUtf8());
	QCOMPARE(Tools::cleanUsername(), expected);
}

void TestTools::testCleanUsername_data()
{
	QTest::addColumn<QString>("input");
	QTest::addColumn<QString>("expected");

	QTest::newRow("Leading and trailing spaces") << "  user  " << "user";
	QTest::newRow("Special characters") << R"(user<>:"/\|?*name)" << "user_________name";
	QTest::newRow("Trailing dots and spaces") << "username...   " << "username";
	QTest::newRow("Combination of issues") << R"(  user<>:"/\|?*name...   )" << "user_________name";
}

void TestTools::testEscapeAccelerators()
{
	QCOMPARE(Tools::escapeAccelerators(QString()), QStringLiteral(""));
	QCOMPARE(Tools::escapeAccelerators(QStringLiteral("NoAccelerator")), QStringLiteral("NoAccelerator"));
	QCOMPARE(Tools::escapeAccelerators(QStringLiteral("&Accelerator")), QStringLiteral("&&Accelerator"));
	QCOMPARE(Tools::escapeAccelerators(QStringLiteral("Accelerator&")), QStringLiteral("Accelerator&&"));
	QCOMPARE(Tools::escapeAccelerators(QStringLiteral("Accel&erator&")), QStringLiteral("Accel&&erator&&"));
	QCOMPARE(Tools::escapeAccelerators(QStringLiteral("Accel&&erator")), QStringLiteral("Accel&&&&erator"));
	QCOMPARE(Tools::escapeAccelerators(QStringLiteral("Some & text")), QStringLiteral("Some && text"));
}
