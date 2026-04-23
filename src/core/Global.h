/*
 *  Copyright (C) 2012 Felix Geyer <debfx@fobos.de>
 *  Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies)
 *  Copyright (C) 2012 Intel Corporation
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

#ifndef KEEPASSX_GLOBAL_H
#define KEEPASSX_GLOBAL_H

#include <QString>
#include <QTextStream>

#ifndef QUINT32_MAX
#define QUINT32_MAX 4294967295U
#endif

#define FILE_CASE_SENSITIVE Qt::CaseSensitive

static const auto TRUE_STR = QStringLiteral("true");
static const auto FALSE_STR = QStringLiteral("false");

enum class AuthDecision
{
	Undecided,
	Allowed,
	AllowedOnce,
	Denied,
	DeniedOnce,
};

template <typename T>
struct AddConst
{
	typedef const T Type;
};

// this adds const to non-const objects (like std::as_const)
template <typename T>
constexpr typename AddConst<T>::Type &asConst(T &t) noexcept
{
	return t;
}

// prevent rvalue arguments:
template <typename T>
void asConst(const T &&) = delete;

#endif // KEEPASSX_GLOBAL_H
