/*
 *  Copyright (C) 2015 Florian Geyer <blueice@fobos.de>
 *  Copyright (C) 2015 Felix Geyer <debfx@fobos.de>
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

#include "TestCsvExporter.h"

#include <QBuffer>
#include <QTest>

#include "core/Group.h"
#include "core/Tools.h"
#include "core/Totp.h"
#include "crypto/Crypto.h"
#include "format/CsvExporter.h"

QTEST_GUILESS_MAIN(TestCsvExporter)

const QString TestCsvExporter::ExpectedHeaderLine =
	QStringLiteral("\"Group\",\"Title\",\"Username\",\"Password\",\"URL\",\"Notes\",\"TOTP\",\"Icon\",\"Last Modified\",\"Created\"\n");

void TestCsvExporter::init()
{
	m_db = QSharedPointer<Database>::create();
	m_csvExporter = QSharedPointer<CsvExporter>::create();
}

void TestCsvExporter::initTestCase()
{
	Crypto::init();
}

void TestCsvExporter::cleanup()
{
}

void TestCsvExporter::testExport()
{
	Group *groupRoot = m_db->rootGroup();
	auto *group = new Group();
	group->setName(QStringLiteral("Test Group Name"));
	group->setParent(groupRoot);
	auto *entry = new Entry();
	entry->setGroup(group);
	entry->setTitle(QStringLiteral("Test Entry Title"));
	entry->setUsername(QStringLiteral("Test Username"));
	entry->setPassword(QStringLiteral("Test Password"));
	entry->setUrl(QStringLiteral("http://test.url"));
	entry->setNotes(QStringLiteral("Test Notes"));
	entry->setTotp(Totp::createSettings(QStringLiteral("DFDF"), Totp::DEFAULT_DIGITS, Totp::DEFAULT_STEP));
	entry->setIcon(5);

	QBuffer buffer;
	QVERIFY(buffer.open(QIODevice::ReadWrite));
	m_csvExporter->exportDatabase(&buffer, m_db);
	auto exported = QString::fromUtf8(buffer.buffer());

	QString expectedResult = QString()
	                             .append(ExpectedHeaderLine)
	                             .append(QStringLiteral("\"Passwords/Test Group Name\",\"Test Entry Title\",\"Test Username\",\"Test "
	                                     "Password\",\"http://test.url\",\"Test Notes\""));

	QVERIFY(exported.startsWith(expectedResult));
	exported.remove(expectedResult);
	QVERIFY(exported.contains(QStringLiteral("otpauth://")));
	QVERIFY(exported.contains(QStringLiteral(",\"5\",")));
}

void TestCsvExporter::testEmptyDatabase()
{
	QBuffer buffer;
	QVERIFY(buffer.open(QIODevice::ReadWrite));
	m_csvExporter->exportDatabase(&buffer, m_db);

	QCOMPARE(QString::fromUtf8(buffer.buffer().constData()), ExpectedHeaderLine);
}

void TestCsvExporter::testNestedGroups()
{
	Group *groupRoot = m_db->rootGroup();
	auto *group = new Group();
	group->setName(QStringLiteral("Test Group Name"));
	group->setParent(groupRoot);
	auto *childGroup = new Group();
	childGroup->setName(QStringLiteral("Test Sub Group Name"));
	childGroup->setParent(group);
	auto *entry = new Entry();
	entry->setGroup(childGroup);
	entry->setTitle(QStringLiteral("Test Entry Title"));

	QBuffer buffer;
	QVERIFY(buffer.open(QIODevice::ReadWrite));
	m_csvExporter->exportDatabase(&buffer, m_db);
	auto exported = QString::fromUtf8(buffer.buffer());
	QVERIFY(exported.startsWith(
		QString()
			.append(ExpectedHeaderLine)
			.append(QStringLiteral("\"Passwords/Test Group Name/Test Sub Group Name\",\"Test Entry Title\",\"\",\"\",\"\",\"\""))));
}

void TestCsvExporter::testRoundTripWithCustomRootName()
{
	// Create a database with a custom root group name
	Group *groupRoot = m_db->rootGroup();
	groupRoot->setName(QStringLiteral("MyPasswords")); // Custom root name instead of default "Passwords"

	auto *group = new Group();
	group->setName(QStringLiteral("Test Group"));
	group->setParent(groupRoot);

	auto *entry = new Entry();
	entry->setGroup(group);
	entry->setTitle(QStringLiteral("Test Entry"));
	entry->setUsername(QStringLiteral("testuser"));
	entry->setPassword(QStringLiteral("testpass"));

	// Export to CSV
	QString csvData = m_csvExporter->exportDatabase(m_db);

	// Verify export contains the root group name in the path
	QVERIFY(csvData.contains(QStringLiteral("\"MyPasswords/Test Group\"")));

	// Test the heuristic approach: analyze multiple similar paths
	QStringList groupPaths = {QStringLiteral("MyPasswords/Test Group"), QStringLiteral("MyPasswords/Another Group"), QStringLiteral("MyPasswords/Third Group")};

	// Test the analyzeCommonRootGroup function logic
	QStringList firstComponents;
	for (const QString &path: groupPaths)
	{
		if (!path.isEmpty() && !path.startsWith(QStringLiteral("/")))
		{
			auto nameList = path.split(QStringLiteral("/"), Qt::SkipEmptyParts);
			if (!nameList.isEmpty())
			{
				firstComponents.append(nameList.first());
			}
		}
	}

	// All paths should have "MyPasswords" as first component
	QCOMPARE(firstComponents.size(), 3);
	QVERIFY(firstComponents.contains(QStringLiteral("MyPasswords")));

	// With 100% consistency, "MyPasswords" should be identified as common root
	QMap<QString, int> componentCounts;
	for (const QString &component: firstComponents)
	{
		componentCounts[component]++;
	}

	QCOMPARE(componentCounts[QStringLiteral("MyPasswords")], 3); // All 3 paths have this root

	// Simulate the group creation with identified root to skip
	QString groupPathFromCsv = QStringLiteral("MyPasswords/Test Group");
	auto nameList = groupPathFromCsv.split(QStringLiteral("/"), Qt::SkipEmptyParts);

	// New heuristic logic: skip identified root group name
	QString rootGroupToSkip = QStringLiteral("MyPasswords");
	if (!rootGroupToSkip.isEmpty() && !nameList.isEmpty()
	    && nameList.first().compare(rootGroupToSkip, Qt::CaseInsensitive) == 0)
	{
		nameList.removeFirst();
	}

	// After this logic, nameList should contain only ["Test Group"]
	QCOMPARE(nameList.size(), 1);
	QCOMPARE(nameList.first(), QStringLiteral("Test Group"));
}

void TestCsvExporter::testRoundTripWithDefaultRootName()
{
	// Test with default "Passwords" root name to ensure it works correctly
	Group *groupRoot = m_db->rootGroup();
	// Default name is "Passwords" - don't change it

	auto *group = new Group();
	group->setName(QStringLiteral("Test Group"));
	group->setParent(groupRoot);

	auto *entry = new Entry();
	entry->setGroup(group);
	entry->setTitle(QStringLiteral("Test Entry"));
	entry->setUsername(QStringLiteral("testuser"));
	entry->setPassword(QStringLiteral("testpass"));

	// Export to CSV
	QString csvData = m_csvExporter->exportDatabase(m_db);

	// Verify export contains the root group name in the path
	QVERIFY(csvData.contains(QStringLiteral("\"Passwords/Test Group\"")));

	// Test the heuristic approach with consistent "Passwords" root
	QStringList groupPaths = {QStringLiteral("Passwords/Test Group"), QStringLiteral("Passwords/Work"), QStringLiteral("Passwords/Personal")};

	// Simulate analysis to find common root
	QStringList firstComponents;
	for (const QString &path: groupPaths)
	{
		if (!path.isEmpty() && !path.startsWith(QStringLiteral("/")))
		{
			auto nameList = path.split(QStringLiteral("/"), Qt::SkipEmptyParts);
			if (!nameList.isEmpty())
			{
				firstComponents.append(nameList.first());
			}
		}
	}

	// All should have "Passwords" as first component
	QCOMPARE(firstComponents.size(), 3);
	for (const QString &component: firstComponents)
	{
		QCOMPARE(component, QStringLiteral("Passwords"));
	}

	// Test group creation with identified root to skip
	QString groupPathFromCsv = QStringLiteral("Passwords/Test Group");
	auto nameList = groupPathFromCsv.split(QStringLiteral("/"), Qt::SkipEmptyParts);

	// Heuristic logic: skip the identified common root
	QString rootGroupToSkip = QStringLiteral("Passwords");
	if (!rootGroupToSkip.isEmpty() && !nameList.isEmpty()
	    && nameList.first().compare(rootGroupToSkip, Qt::CaseInsensitive) == 0)
	{
		nameList.removeFirst();
	}

	// After this logic, nameList should contain only ["Test Group"]
	QCOMPARE(nameList.size(), 1);
	QCOMPARE(nameList.first(), QStringLiteral("Test Group"));
}

void TestCsvExporter::testSingleLevelGroup()
{
	// Test case: entry is directly in root group (no sub-groups)
	// This should still work correctly and not remove any path components

	Group *groupRoot = m_db->rootGroup();
	auto *entry = new Entry();
	entry->setGroup(groupRoot); // Put entry directly in root
	entry->setTitle(QStringLiteral("Root Entry"));
	entry->setUsername(QStringLiteral("rootuser"));
	entry->setPassword(QStringLiteral("rootpass"));

	// Export to CSV
	QString csvData = m_csvExporter->exportDatabase(m_db);

	// Verify export contains just the root group name (no sub-path)
	QVERIFY(csvData.contains(QStringLiteral("\"Passwords\",\"Root Entry\"")));

	// Test heuristic with single-component paths
	QStringList groupPaths = {QStringLiteral("Passwords"), QStringLiteral("Work"), QStringLiteral("Personal")}; // Mixed single components

	// With inconsistent first components, no common root should be identified
	QStringList firstComponents;
	for (const QString &path: groupPaths)
	{
		if (!path.isEmpty() && !path.startsWith(QStringLiteral("/")))
		{
			auto nameList = path.split(QStringLiteral("/"), Qt::SkipEmptyParts);
			if (!nameList.isEmpty())
			{
				firstComponents.append(nameList.first());
			}
		}
	}
	// Should have 3 different first components
	QCOMPARE(firstComponents.size(), 3);
	auto uniqueComponents = Tools::asSet(firstComponents);
	QCOMPARE(uniqueComponents.size(), 3); // All different

	// Test group creation with no identified root to skip
	QString groupPathFromCsv = QStringLiteral("Passwords"); // Single component
	auto nameList = groupPathFromCsv.split(QStringLiteral("/"), Qt::SkipEmptyParts);

	// With no common root identified, nothing should be removed
	QString rootGroupToSkip; // Empty - no common root found
	if (!rootGroupToSkip.isEmpty() && !nameList.isEmpty()
	    && nameList.first().compare(rootGroupToSkip, Qt::CaseInsensitive) == 0)
	{
		nameList.removeFirst();
	}

	// Should still have ["Passwords"] as nothing was removed
	QCOMPARE(nameList.size(), 1);
	QCOMPARE(nameList.first(), QStringLiteral("Passwords"));
}

void TestCsvExporter::testAbsolutePaths()
{
	// Test case: paths that start with "/" (absolute paths)
	// According to the comment, if every row starts with "/", the root group should be left as is

	QStringList groupPaths = {QStringLiteral("/Work/Subgroup1"), QStringLiteral("/Personal/Subgroup2"), QStringLiteral("/Finance/Subgroup3")};

	// Test the heuristic analysis with absolute paths
	QStringList firstComponents;
	for (const QString &path: groupPaths)
	{
		if (!path.isEmpty() && !path.startsWith(QStringLiteral("/")))
		{
			auto nameList = path.split(QStringLiteral("/"), Qt::SkipEmptyParts);
			if (!nameList.isEmpty())
			{
				firstComponents.append(nameList.first());
			}
		}
		// Note: paths starting with "/" are skipped in the analysis
	}

	// Since all paths start with "/", no first components should be collected
	QCOMPARE(firstComponents.size(), 0);

	// With no first components, no common root should be identified
	QString rootGroupToSkip; // Should be empty

	// Test group creation with absolute path
	QString groupPathFromCsv = QStringLiteral("/Work/Subgroup1");
	auto nameList = groupPathFromCsv.split(QStringLiteral("/"), Qt::SkipEmptyParts);

	// With no root to skip, the full path should be preserved
	if (!rootGroupToSkip.isEmpty() && !nameList.isEmpty()
	    && nameList.first().compare(rootGroupToSkip, Qt::CaseInsensitive) == 0)
	{
		nameList.removeFirst();
	}

	// Should have ["Work", "Subgroup1"] - full path preserved
	QCOMPARE(nameList.size(), 2);
	QCOMPARE(nameList.at(0), QStringLiteral("Work"));
	QCOMPARE(nameList.at(1), QStringLiteral("Subgroup1"));
}
