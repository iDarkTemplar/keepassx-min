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

#ifndef TEST_IMPORTS_H
#define TEST_IMPORTS_H

#include <QObject>

class TestImports: public QObject
{
	Q_OBJECT

private Q_SLOTS:
	void initTestCase();
	void testOPUX();
	void testOPVault();
	void testBitwarden();
	void testBitwardenEncrypted();
	void testBitwardenPasskey();
	void testProtonPass();
};

#endif /* TEST_IMPORTS_H */
