/*
 *  Copyright (C) 2026 i.Dark_Templar <darktemplar@dark-templar-archives.net>
 *  Copyright (C) 2021 KeePassXC Team <team@keepassxc.org>
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

#pragma once

#include <QString>

namespace ColorRole {

	constexpr QStringView Error = u"#FF7D7D";
	constexpr QStringView Warning = u"#FFD30F";
	constexpr QStringView Info = u"#84D0E1";
	constexpr QStringView Incomplete = u"#FFD30F";

	constexpr QStringView HealthCritical = u"#C43F31";
	constexpr QStringView HealthBad = u"#E07F16";
	constexpr QStringView HealthWeak = u"#FFD30F";
	constexpr QStringView HealthOk = u"#5EA10E";
	constexpr QStringView HealthExcellent = u"#118f17";

	constexpr QStringView True = u"#5EA10E";
	constexpr QStringView False = u"#C43F31";

} // namespace ColorRole
