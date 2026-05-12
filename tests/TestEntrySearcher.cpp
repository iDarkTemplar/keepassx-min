/*
 *  Copyright (C) 2014 Florian Geyer <blueice@fobos.de>
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

#include "TestEntrySearcher.h"
#include "core/Group.h"
#include "core/Tools.h"
#include "core/Totp.h"

#include <QTest>

QTEST_GUILESS_MAIN(TestEntrySearcher)

void TestEntrySearcher::init()
{
	m_rootGroup = new Group();
	m_entrySearcher = EntrySearcher();
}

void TestEntrySearcher::cleanup()
{
	delete m_rootGroup;
}

void TestEntrySearcher::testSearch()
{
	/**
	 * Root
	 * - group1 (search disabled)
	 *   - group11
	 * - group2
	 *   - group21
	 *     - group211
	 *       - group2111
	 */
	Group *group1 = new Group();
	Group *group2 = new Group();
	Group *group3 = new Group();

	group1->setParent(m_rootGroup);
	group2->setParent(m_rootGroup);
	group3->setParent(m_rootGroup);

	Group *group11 = new Group();

	group11->setParent(group1);

	Group *group21 = new Group();
	Group *group211 = new Group();
	Group *group2111 = new Group();

	group21->setParent(group2);
	group211->setParent(group21);
	group2111->setParent(group211);

	group1->setSearchingEnabled(Group::Disable);

	Entry *eRoot = new Entry();
	eRoot->setTitle(QStringLiteral("test search term test"));
	eRoot->setGroup(m_rootGroup);

	Entry *eRoot2 = new Entry();
	eRoot2->setNotes(QStringLiteral("test term test"));
	eRoot2->setGroup(m_rootGroup);

	// Searching is disabled for these
	Entry *e1 = new Entry();
	e1->setUsername(QStringLiteral("test search term test"));
	e1->setGroup(group1);

	Entry *e11 = new Entry();
	e11->setNotes(QStringLiteral("test search term test"));
	e11->setGroup(group11);
	// End searching disabled

	Entry *e2111 = new Entry();
	e2111->setTitle(QStringLiteral("test search term test"));
	e2111->setGroup(group2111);

	Entry *e2111b = new Entry();
	e2111b->setNotes(QStringLiteral("test search test"));
	e2111b->setUsername(QStringLiteral("user123"));
	e2111b->setPassword(QStringLiteral("testpass"));
	e2111b->setGroup(group2111);

	Entry *e3 = new Entry();
	e3->setUrl(QStringLiteral("test search term test"));
	e3->setGroup(group3);

	Entry *e3b = new Entry();
	e3b->setTitle(QStringLiteral("test search test 123"));
	e3b->setUsername(QStringLiteral("test@email.com"));
	e3b->setPassword(QStringLiteral("realpass"));
	e3b->setGroup(group3);

	// Simple search term testing
	m_searchResult = m_entrySearcher.search(QStringLiteral("search"), m_rootGroup);
	QCOMPARE(m_searchResult.count(), 5);

	m_searchResult = m_entrySearcher.search(QStringLiteral("search term"), m_rootGroup);
	QCOMPARE(m_searchResult.count(), 3);

	m_searchResult = m_entrySearcher.search(QStringLiteral("123"), m_rootGroup);
	QCOMPARE(m_searchResult.count(), 2);

	m_searchResult = m_entrySearcher.search(QStringLiteral("search term"), group211);
	QCOMPARE(m_searchResult.count(), 1);

	// Test advanced search terms
	m_searchResult = m_entrySearcher.search(QStringLiteral("title:123"), m_rootGroup);
	QCOMPARE(m_searchResult.count(), 1);

	m_searchResult = m_entrySearcher.search(QStringLiteral("t:123"), m_rootGroup);
	QCOMPARE(m_searchResult.count(), 1);

	m_searchResult = m_entrySearcher.search(QStringLiteral("password:testpass"), m_rootGroup);
	QCOMPARE(m_searchResult.count(), 1);

	m_searchResult = m_entrySearcher.search(QStringLiteral("pw:testpass"), m_rootGroup);
	QCOMPARE(m_searchResult.count(), 1);

	m_searchResult = m_entrySearcher.search(QStringLiteral("!user:email.com"), m_rootGroup);
	QCOMPARE(m_searchResult.count(), 5);

	m_searchResult = m_entrySearcher.search(QStringLiteral("!u:email.com"), m_rootGroup);
	QCOMPARE(m_searchResult.count(), 5);

	m_searchResult = m_entrySearcher.search(QStringLiteral("*user:\".*@.*\\.com\""), m_rootGroup);
	QCOMPARE(m_searchResult.count(), 1);

	m_searchResult = m_entrySearcher.search(QStringLiteral("+user:email"), m_rootGroup);
	QCOMPARE(m_searchResult.count(), 0);

	// Terms are logical AND together
	m_searchResult = m_entrySearcher.search(QStringLiteral("password:pass user:user"), m_rootGroup);
	QCOMPARE(m_searchResult.count(), 1);

	// Parent group has search disabled
	m_searchResult = m_entrySearcher.search(QStringLiteral("search term"), group11);
	QCOMPARE(m_searchResult.count(), 0);
}

void TestEntrySearcher::testAndConcatenationInSearch()
{
	Entry *entry = new Entry();
	entry->setNotes(QStringLiteral("abc def ghi"));
	entry->setTitle(QStringLiteral("jkl"));
	entry->setGroup(m_rootGroup);

	m_searchResult = m_entrySearcher.search(QString(), m_rootGroup);
	QCOMPARE(m_searchResult.count(), 1);

	m_searchResult = m_entrySearcher.search(QStringLiteral("def"), m_rootGroup);
	QCOMPARE(m_searchResult.count(), 1);

	m_searchResult = m_entrySearcher.search(QStringLiteral("  abc    ghi  "), m_rootGroup);
	QCOMPARE(m_searchResult.count(), 1);

	m_searchResult = m_entrySearcher.search(QStringLiteral("ghi ef"), m_rootGroup);
	QCOMPARE(m_searchResult.count(), 1);

	m_searchResult = m_entrySearcher.search(QStringLiteral("abc ef xyz"), m_rootGroup);
	QCOMPARE(m_searchResult.count(), 0);

	m_searchResult = m_entrySearcher.search(QStringLiteral("abc kl"), m_rootGroup);
	QCOMPARE(m_searchResult.count(), 1);
}

void TestEntrySearcher::testAllAttributesAreSearched()
{
	Entry *entry = new Entry();
	entry->setGroup(m_rootGroup);

	entry->setTitle(QStringLiteral("testTitle"));
	entry->setUsername(QStringLiteral("testUsername"));
	entry->setUrl(QStringLiteral("testUrl"));
	entry->setNotes(QStringLiteral("testNote"));

	// Default is to AND all terms together
	m_searchResult = m_entrySearcher.search(QStringLiteral("testTitle testUsername testUrl testNote"), m_rootGroup);
	QCOMPARE(m_searchResult.count(), 1);
}

void TestEntrySearcher::testSearchTermParser()
{
	// Test standard search terms
	m_entrySearcher.parseSearchTerms(QStringLiteral("-test \"quoted \\\"string\\\"\"  user:user pass:\"test me\" noquote  "));
	auto terms = m_entrySearcher.m_searchTerms;

	QCOMPARE(terms.length(), 5);

	QCOMPARE(terms[0].field, EntrySearcher::Field::Undefined);
	QCOMPARE(terms[0].word, QStringLiteral("test"));
	QCOMPARE(terms[0].exclude, true);

	QCOMPARE(terms[1].field, EntrySearcher::Field::Undefined);
	QCOMPARE(terms[1].word, QStringLiteral("quoted \"string\""));
	QCOMPARE(terms[1].exclude, false);

	QCOMPARE(terms[2].field, EntrySearcher::Field::Username);
	QCOMPARE(terms[2].word, QStringLiteral("user"));

	QCOMPARE(terms[3].field, EntrySearcher::Field::Password);
	QCOMPARE(terms[3].word, QStringLiteral("test me"));

	QCOMPARE(terms[4].field, EntrySearcher::Field::Undefined);
	QCOMPARE(terms[4].word, QStringLiteral("noquote"));

	// Test wildcard and regex search terms
	m_entrySearcher.parseSearchTerms(QStringLiteral("+url:*.google.com *user:\\d+\\w{2}"));
	terms = m_entrySearcher.m_searchTerms;

	QCOMPARE(terms.length(), 2);

	QCOMPARE(terms[0].field, EntrySearcher::Field::Url);
	QCOMPARE(terms[0].regex.pattern(), QStringLiteral("^(?:.*\\.google\\.com)$"));

	QCOMPARE(terms[1].field, EntrySearcher::Field::Username);
	QCOMPARE(terms[1].regex.pattern(), QStringLiteral("\\d+\\w{2}"));

	// Test custom attribute search terms
	m_entrySearcher.parseSearchTerms(QStringLiteral("+_abc:efg _def:\"ddd\""));
	terms = m_entrySearcher.m_searchTerms;

	QCOMPARE(terms.length(), 2);

	QCOMPARE(terms[0].field, EntrySearcher::Field::AttributeValue);
	QCOMPARE(terms[0].word, QStringLiteral("abc"));
	QCOMPARE(terms[0].regex.pattern(), QStringLiteral("^(?:efg)$"));

	QCOMPARE(terms[1].field, EntrySearcher::Field::AttributeValue);
	QCOMPARE(terms[1].word, QStringLiteral("def"));
	QCOMPARE(terms[1].regex.pattern(), QStringLiteral("ddd"));
}

void TestEntrySearcher::testCustomAttributesAreSearched()
{
	QScopedPointer<Entry> e1(new Entry());
	e1->setGroup(m_rootGroup);

	e1->attributes()->set(QStringLiteral("testAttribute"), QStringLiteral("testE1"));
	e1->attributes()->set(QStringLiteral("testProtected"), QStringLiteral("testP"), true);

	QScopedPointer<Entry> e2(new Entry());
	e2->setGroup(m_rootGroup);
	e2->attributes()->set(QStringLiteral("testAttribute"), QStringLiteral("testE2"));
	e2->attributes()->set(QStringLiteral("testProtected"), QStringLiteral("testP2"), true);

	// search for custom entries
	m_searchResult = m_entrySearcher.search(QStringLiteral("_testAttribute:test"), m_rootGroup);
	QCOMPARE(m_searchResult.count(), 2);

	// protected attributes are ignored
	m_entrySearcher = EntrySearcher(false, true);
	m_searchResult = m_entrySearcher.search(QStringLiteral("_testAttribute:test _testProtected:testP2"), m_rootGroup);
	QCOMPARE(m_searchResult.count(), 2);
}

void TestEntrySearcher::testGroup()
{
	/**
	 * Root
	 * - group1 (1 entry)
	 *   - subgroup1 (2 entries)
	 * - group2
	 *   - subgroup2 (1 entry)
	 */
	Group *group1 = new Group();
	Group *group2 = new Group();

	group1->setParent(m_rootGroup);
	group1->setName(QStringLiteral("group1"));
	group2->setParent(m_rootGroup);
	group2->setName(QStringLiteral("group2"));

	Group *subgroup1 = new Group();
	subgroup1->setName(QStringLiteral("subgroup1"));
	subgroup1->setParent(group1);

	Group *subgroup2 = new Group();
	subgroup2->setName(QStringLiteral("subgroup2"));
	subgroup2->setParent(group2);

	Entry *eGroup1 = new Entry();
	eGroup1->setTitle(QStringLiteral("Entry Group 1"));
	eGroup1->setGroup(group1);

	Entry *eSub1 = new Entry();
	eSub1->setTitle(QStringLiteral("test search term test"));
	eSub1->setGroup(subgroup1);

	Entry *eSub2 = new Entry();
	eSub2->setNotes(QStringLiteral("test test"));
	eSub2->setGroup(subgroup1);

	Entry *eSub3 = new Entry();
	eSub3->setNotes(QStringLiteral("test term test"));
	eSub3->setGroup(subgroup2);

	m_searchResult = m_entrySearcher.search(QStringLiteral("group:subgroup"), m_rootGroup);
	QCOMPARE(m_searchResult.count(), 3);

	m_searchResult = m_entrySearcher.search(QStringLiteral("g:subgroup1"), m_rootGroup);
	QCOMPARE(m_searchResult.count(), 2);

	m_searchResult = m_entrySearcher.search(QStringLiteral("g:subgroup1 search"), m_rootGroup);
	QCOMPARE(m_searchResult.count(), 1);

	m_searchResult = m_entrySearcher.search(QStringLiteral("g:*1/sub*1"), m_rootGroup);
	QCOMPARE(m_searchResult.count(), 2);

	m_searchResult = m_entrySearcher.search(QStringLiteral("g:/group1 search"), m_rootGroup);
	QCOMPARE(m_searchResult.count(), 1);
}

void TestEntrySearcher::testSkipProtected()
{
	QScopedPointer<Entry> e1(new Entry());
	e1->setGroup(m_rootGroup);

	e1->attributes()->set(QStringLiteral("testAttribute"), QStringLiteral("testE1"));
	e1->attributes()->set(QStringLiteral("testProtected"), QStringLiteral("apple"), true);

	QScopedPointer<Entry> e2(new Entry());
	e2->setGroup(m_rootGroup);
	e2->attributes()->set(QStringLiteral("testAttribute"), QStringLiteral("testE2"));
	e2->attributes()->set(QStringLiteral("testProtected"), QStringLiteral("banana"), true);

	const QList<Entry*> expectE1{e1.data()};
	const QList<Entry*> expectE2{e2.data()};
	const QList<Entry*> expectBoth{e1.data(), e2.data()};

	// when not skipping protected, empty term matches everything
	m_searchResult = m_entrySearcher.search(QString(), m_rootGroup);
	QCOMPARE(m_searchResult, expectBoth);

	// now test the searcher with skipProtected = true
	m_entrySearcher = EntrySearcher(false, true);

	// when skipping protected, empty term matches nothing
	m_searchResult = m_entrySearcher.search(QString(), m_rootGroup);
	QCOMPARE(m_searchResult, {});

	// having a protected entry in terms should not affect the results in anyways
	m_searchResult = m_entrySearcher.search(QStringLiteral("_testProtected:apple"), m_rootGroup);
	QCOMPARE(m_searchResult, {});
	m_searchResult = m_entrySearcher.search(QStringLiteral("_testProtected:apple _testAttribute:testE2"), m_rootGroup);
	QCOMPARE(m_searchResult, expectE2);
	m_searchResult = m_entrySearcher.search(QStringLiteral("_testProtected:apple _testAttribute:testE1"), m_rootGroup);
	QCOMPARE(m_searchResult, expectE1);
	m_searchResult =
		m_entrySearcher.search(QStringLiteral("_testProtected:apple _testAttribute:testE1 _testAttribute:testE2"), m_rootGroup);
	QCOMPARE(m_searchResult, {});

	// also move the protected term around to execurise the short-circut logic
	m_searchResult = m_entrySearcher.search(QStringLiteral("_testAttribute:testE2 _testProtected:apple"), m_rootGroup);
	QCOMPARE(m_searchResult, expectE2);
	m_searchResult = m_entrySearcher.search(QStringLiteral("_testAttribute:testE1 _testProtected:apple"), m_rootGroup);
	QCOMPARE(m_searchResult, expectE1);
	m_searchResult =
		m_entrySearcher.search(QStringLiteral("_testAttribute:testE1 _testProtected:apple _testAttribute:testE2"), m_rootGroup);
	QCOMPARE(m_searchResult, {});
}

void TestEntrySearcher::testUUIDSearch()
{
	auto entry1 = new Entry();
	entry1->setGroup(m_rootGroup);
	entry1->setTitle(QStringLiteral("testTitle"));
	auto uuid1 = QUuid::createUuid();
	entry1->setUuid(uuid1);

	auto entry2 = new Entry();
	entry2->setGroup(m_rootGroup);
	entry2->setTitle(QStringLiteral("testTitle2"));
	auto uuid2 = QUuid::createUuid();
	entry2->setUuid(uuid2);

	m_searchResult = m_entrySearcher.search(QStringLiteral("uuid:"), m_rootGroup);
	QCOMPARE(m_searchResult.count(), 2);

	m_searchResult = m_entrySearcher.search(QStringLiteral("uuid:") + Tools::uuidToHex(uuid1), m_rootGroup);
	QCOMPARE(m_searchResult.count(), 1);
}

void TestEntrySearcher::testTotpSearch()
{
	auto entry1 = new Entry();
	entry1->setGroup(m_rootGroup);
	entry1->setTitle(QStringLiteral("Regular Entry"));

	auto entry2 = new Entry();
	entry2->setGroup(m_rootGroup);
	entry2->setTitle(QStringLiteral("TOTP Entry"));
	// Set up TOTP on entry2
	auto totpSettings = Totp::createSettings(QStringLiteral("GEZDGNBVGY3TQOJQGEZDGNBVGY3TQOJQ"), 6, 30);
	entry2->setTotp(totpSettings);

	auto entry3 = new Entry();
	entry3->setGroup(m_rootGroup);
	entry3->setTitle(QStringLiteral("Another TOTP Entry"));
	// Set up TOTP on entry3
	auto totpSettings2 = Totp::createSettings(QStringLiteral("MFRGG43UEBUXGIDBKRWXAZLSMUQGG6LQ"), 6, 30);
	entry3->setTotp(totpSettings2);

	// Test searching for TOTP entries
	m_searchResult = m_entrySearcher.search(QStringLiteral("has:totp"), m_rootGroup);
	QCOMPARE(m_searchResult.count(), 2);
	QVERIFY(m_searchResult.contains(entry2));
	QVERIFY(m_searchResult.contains(entry3));
	QVERIFY(!m_searchResult.contains(entry1));

	// Test case insensitive search
	m_searchResult = m_entrySearcher.search(QStringLiteral("has:TOTP"), m_rootGroup);
	QCOMPARE(m_searchResult.count(), 2);

	// Test excluding TOTP entries
	m_searchResult = m_entrySearcher.search(QStringLiteral("!has:totp"), m_rootGroup);
	QCOMPARE(m_searchResult.count(), 1);
	QVERIFY(m_searchResult.contains(entry1));
	QVERIFY(!m_searchResult.contains(entry2));
	QVERIFY(!m_searchResult.contains(entry3));
}
