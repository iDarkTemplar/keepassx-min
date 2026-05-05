/*
 *  Copyright (C) 2018 KeePassXC Team <team@keepassxc.org>
 *  Copyright (C) 2010 Felix Geyer <debfx@fobos.de>
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

#ifndef KEEPASSX_COMPOSITEKEY_H
#define KEEPASSX_COMPOSITEKEY_H

#include <QSharedPointer>
#include <QList>

#include "keys/Key.h"

class Kdf;

class CompositeKey: public Key
{
public:
	static QUuid UUID;

	CompositeKey();
	~CompositeKey() override;

	void clear();
	bool isEmpty() const;

	QByteArray rawKey() const override;
	void setRawKey(const QByteArray &data) override;

	Q_REQUIRED_RESULT bool transform(const Kdf &kdf, QByteArray &result) const;

	void addKey(const QSharedPointer<Key> &key);
	QSharedPointer<Key> getKey(const QUuid keyType) const;
	const QList<QSharedPointer<Key>> &keys() const;

	QByteArray serialize() const override;
	void deserialize(const QByteArray &data) override;

private:
	QList<QSharedPointer<Key>> m_keys;
};

#endif // KEEPASSX_COMPOSITEKEY_H
