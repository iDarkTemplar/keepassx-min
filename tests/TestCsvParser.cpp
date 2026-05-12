/*
 *  Copyright (C) 2015 Enrico Mariotti <enricomariotti@yahoo.it>
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

#include "TestCsvParser.h"

#include <QTest>

QTEST_GUILESS_MAIN(TestCsvParser)

void TestCsvParser::writeToFile(const QString &contents)
{
	if (!file->open())
	{
		QFAIL("Cannot open temporary file!");
	}

	QTextStream out(file.data());
	out << contents;
	out.flush();
	file->close();
}

void TestCsvParser::initTestCase()
{
	parser.reset(new CsvParser());
}

void TestCsvParser::init()
{
	file.reset(new QTemporaryFile());

	parser->setBackslashSyntax(false);
	parser->setComment(QLatin1Char('#'));
	parser->setFieldSeparator(QLatin1Char(','));
	parser->setTextQualifier(QLatin1Char('"'));
}

void TestCsvParser::cleanup()
{
	file->remove();
}

/****************** TEST CASES ******************/
void TestCsvParser::testMissingQuote()
{
	writeToFile(QStringLiteral("A,B\n:BM,1"));
	parser->setTextQualifier(QLatin1Char(':'));

	QVERIFY(!parser->parse(file.data()));
	qWarning() << parser->getStatus();
}

void TestCsvParser::testMalformed()
{
	writeToFile(QStringLiteral("A,B,C\n:BM::,1,:2:"));
	parser->setTextQualifier(QLatin1Char(':'));

	QVERIFY(!parser->parse(file.data()));
	qWarning() << parser->getStatus();
}

void TestCsvParser::testBackslashSyntax()
{
	// attended result: one"\t\"wo
	writeToFile(QStringLiteral(
		"Xone\\\"\\\\t\\\\\\\"w\noX\n"
		"X13X,X2\\X,X,\"\"3\"X\r"
		"3,X\"4\"X,,\n"
		"XX\n"
		"\\"));

	parser->setBackslashSyntax(true);
	parser->setTextQualifier(QLatin1Char('X'));

	QVERIFY(parser->parse(file.data()));
	t = parser->getCsvTable();
	QVERIFY(t.at(0).at(0) == QStringLiteral("one\"\\t\\\"w\no"));
	QVERIFY(t.at(1).at(0) == QStringLiteral("13"));
	QVERIFY(t.at(1).at(1) == QStringLiteral("2X,"));
	QVERIFY(t.at(1).at(2) == QStringLiteral("\"\"3\"X"));
	QVERIFY(t.at(2).at(0) == QStringLiteral("3"));
	QVERIFY(t.at(2).at(1) == QStringLiteral("\"4\""));
	QVERIFY(t.at(2).at(2) == QString());
	QVERIFY(t.at(2).at(3) == QString());
	QVERIFY(t.at(3).at(0) == QStringLiteral("\\"));
	QVERIFY(t.size() == 4);
}

void TestCsvParser::testQuoted()
{
	writeToFile(QStringLiteral(
		"ro,w,\"end, of \"\"\"\"\"\"row\"\"\"\"\"\n"
		"2\n"));

	QVERIFY(parser->parse(file.data()));
	t = parser->getCsvTable();

	QVERIFY(t.at(0).at(0) == QStringLiteral("ro"));
	QVERIFY(t.at(0).at(1) == QStringLiteral("w"));
	QVERIFY(t.at(0).at(2) == QStringLiteral("end, of \"\"\"row\"\""));
	QVERIFY(t.at(1).at(0) == QStringLiteral("2"));
	QVERIFY(t.size() == 2);
}

void TestCsvParser::testEmptySimple()
{
	QVERIFY(parser->parse(file.data()));
	t = parser->getCsvTable();
	QVERIFY(t.isEmpty());
}

void TestCsvParser::testEmptyQuoted()
{
	writeToFile(QStringLiteral("\"\""));

	QVERIFY(parser->parse(file.data()));
	t = parser->getCsvTable();
	QVERIFY(t.isEmpty());
}

void TestCsvParser::testEmptyNewline()
{
	writeToFile(QStringLiteral("\"\n\""));

	QVERIFY(parser->parse(file.data()));
	t = parser->getCsvTable();
	QVERIFY(t.isEmpty());
}

void TestCsvParser::testEmptyFile()
{
	QVERIFY(parser->parse(file.data()));
	t = parser->getCsvTable();
	QVERIFY(t.isEmpty());
}

void TestCsvParser::testNewline()
{
	writeToFile(QStringLiteral("1,2\n\n\n"));

	QVERIFY(parser->parse(file.data()));
	t = parser->getCsvTable();
	QVERIFY(t.size() == 1);
	QVERIFY(t.at(0).at(0) == QStringLiteral("1"));
	QVERIFY(t.at(0).at(1) == QStringLiteral("2"));
}

void TestCsvParser::testCR()
{
	writeToFile(QStringLiteral("1,2\r3,4"));

	QVERIFY(parser->parse(file.data()));
	t = parser->getCsvTable();
	QVERIFY(t.size() == 2);
	QVERIFY(t.at(0).at(0) == QStringLiteral("1"));
	QVERIFY(t.at(0).at(1) == QStringLiteral("2"));
	QVERIFY(t.at(1).at(0) == QStringLiteral("3"));
	QVERIFY(t.at(1).at(1) == QStringLiteral("4"));
}

void TestCsvParser::testLF()
{
	writeToFile(QStringLiteral("1,2\n3,4"));

	QVERIFY(parser->parse(file.data()));
	t = parser->getCsvTable();

	QVERIFY(t.size() == 2);
	QVERIFY(t.at(0).at(0) == QStringLiteral("1"));
	QVERIFY(t.at(0).at(1) == QStringLiteral("2"));
	QVERIFY(t.at(1).at(0) == QStringLiteral("3"));
	QVERIFY(t.at(1).at(1) == QStringLiteral("4"));
}

void TestCsvParser::testCRLF()
{
	writeToFile(QStringLiteral("1,2\r\n3,4"));

	QVERIFY(parser->parse(file.data()));
	t = parser->getCsvTable();
	QVERIFY(t.size() == 2);
	QVERIFY(t.at(0).at(0) == QStringLiteral("1"));
	QVERIFY(t.at(0).at(1) == QStringLiteral("2"));
	QVERIFY(t.at(1).at(0) == QStringLiteral("3"));
	QVERIFY(t.at(1).at(1) == QStringLiteral("4"));
}

void TestCsvParser::testComments()
{
	writeToFile(QStringLiteral(
		"  #one\n"
		" \t  # two, three \r\n"
		" #, sing\t with\r"
		" #\t  me!\n"
		"useful,text #1!"));

	QVERIFY(parser->parse(file.data()));
	t = parser->getCsvTable();

	QVERIFY(t.size() == 1);
	QVERIFY(t.at(0).at(0) == QStringLiteral("useful"));
	QVERIFY(t.at(0).at(1) == QStringLiteral("text #1!"));
}

void TestCsvParser::testColumns()
{
	writeToFile(QStringLiteral(
		"1,2\n"
		",,,,,,,,,a\n"
		"a,b,c,d\n"));

	QVERIFY(parser->parse(file.data()));
	t = parser->getCsvTable();
	QVERIFY(parser->getCsvCols() == 10);
}

void TestCsvParser::testSimple()
{
	writeToFile(QStringLiteral(
		",,2\r,2,3\n"
		"A,,B\"\n"
		" ,,\n"));

	QVERIFY(parser->parse(file.data()));
	t = parser->getCsvTable();
	QVERIFY(t.size() == 4);

	QVERIFY(t.at(0).at(0) == QString());
	QVERIFY(t.at(0).at(1) == QString());
	QVERIFY(t.at(0).at(2) == QStringLiteral("2"));
	QVERIFY(t.at(1).at(0) == QString());
	QVERIFY(t.at(1).at(1) == QStringLiteral("2"));
	QVERIFY(t.at(1).at(2) == QStringLiteral("3"));
	QVERIFY(t.at(2).at(0) == QStringLiteral("A"));
	QVERIFY(t.at(2).at(1) == QString());
	QVERIFY(t.at(2).at(2) == QStringLiteral("B\""));
	QVERIFY(t.at(3).at(0) == QStringLiteral(" "));
	QVERIFY(t.at(3).at(1) == QString());
	QVERIFY(t.at(3).at(2) == QString());
}

void TestCsvParser::testSeparator()
{
	writeToFile(QStringLiteral(
		"\t\t2\r\t2\t3\n"
		"A\t\tB\"\n"
		" \t\t\n"));

	parser->setFieldSeparator(QLatin1Char('\t'));

	QVERIFY(parser->parse(file.data()));
	t = parser->getCsvTable();
	QVERIFY(t.size() == 4);
	QVERIFY(t.at(0).at(0) == QString());
	QVERIFY(t.at(0).at(1) == QString());
	QVERIFY(t.at(0).at(2) == QStringLiteral("2"));
	QVERIFY(t.at(1).at(0) == QString());
	QVERIFY(t.at(1).at(1) == QStringLiteral("2"));
	QVERIFY(t.at(1).at(2) == QStringLiteral("3"));
	QVERIFY(t.at(2).at(0) == QStringLiteral("A"));
	QVERIFY(t.at(2).at(1) == QString());
	QVERIFY(t.at(2).at(2) == QStringLiteral("B\""));
	QVERIFY(t.at(3).at(0) == QStringLiteral(" "));
	QVERIFY(t.at(3).at(1) == QString());
	QVERIFY(t.at(3).at(2) == QString());
}

void TestCsvParser::testMultiline()
{
	writeToFile(QStringLiteral(
		":1\r\n2a::b:,:3\r4:\n"
		"2\n"));

	parser->setTextQualifier(QLatin1Char(':'));

	QVERIFY(parser->parse(file.data()));
	t = parser->getCsvTable();
	QVERIFY(t.at(0).at(0) == QStringLiteral("1\n2a:b"));
	QVERIFY(t.at(0).at(1) == QStringLiteral("3\n4"));
	QVERIFY(t.at(1).at(0) == QStringLiteral("2"));
	QVERIFY(t.size() == 2);
}

void TestCsvParser::testReparsing()
{
	writeToFile(QStringLiteral(
		":te\r\nxt1:,:te\rxt2:,:end of \"this\n string\":\n"
		"2\n"));

	QVERIFY(parser->parse(file.data()));
	t = parser->getCsvTable();

	QCOMPARE(t.at(0).at(0), QStringLiteral(":te"));

	parser->setTextQualifier(QLatin1Char(':'));

	QVERIFY(parser->reparse());
	t = parser->getCsvTable();
	QCOMPARE(t.at(0).at(0), QStringLiteral("te\nxt1"));
	QCOMPARE(t.at(0).at(1), QStringLiteral("te\nxt2"));
	QCOMPARE(t.at(0).at(2), QStringLiteral("end of \"this\n string\""));
	QCOMPARE(t.at(1).at(0), QStringLiteral("2"));
	QCOMPARE(t.size(), 2);
}

void TestCsvParser::testQualifier()
{
	writeToFile(QStringLiteral(
		"X1X,X2XX,X,\"\"3\"\"\"X\r"
		"3,X\"4\"X,,\n"));

	parser->setTextQualifier(QLatin1Char('X'));

	QVERIFY(parser->parse(file.data()));
	t = parser->getCsvTable();
	QVERIFY(t.size() == 2);
	QVERIFY(t.at(0).at(0) == QStringLiteral("1"));
	QVERIFY(t.at(0).at(1) == QStringLiteral("2X,"));
	QVERIFY(t.at(0).at(2) == QStringLiteral("\"\"3\"\"\"X"));
	QVERIFY(t.at(1).at(0) == QStringLiteral("3"));
	QVERIFY(t.at(1).at(1) == QStringLiteral("\"4\""));
	QVERIFY(t.at(1).at(2) == QString());
	QVERIFY(t.at(1).at(3) == QString());
}

void TestCsvParser::testUnicode()
{
	// QString m("Texte en fran\u00e7ais");
	// CORRECT QString g("\u20AC");
	// CORRECT QChar g(0x20AC);
	// ERROR QChar g("\u20AC");
	writeToFile(QStringLiteral("€1A2śA\"3śAż\"Ażac"));

	parser->setFieldSeparator(QLatin1Char('A'));

	QVERIFY(parser->parse(file.data()));
	t = parser->getCsvTable();
	QVERIFY(t.size() == 1);
	QVERIFY(t.at(0).at(0) == QStringLiteral("€1"));
	QVERIFY(t.at(0).at(1) == QStringLiteral("2ś"));
	QVERIFY(t.at(0).at(2) == QStringLiteral("3śAż"));
	QVERIFY(t.at(0).at(3) == QStringLiteral("żac"));
}
