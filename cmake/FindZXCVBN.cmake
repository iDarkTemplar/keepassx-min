#  Copyright (C) 2026 i.Dark_Templar <darktemplar@dark-templar-archives.net>
#  Copyright (C) 2017 KeePassXC Team <team@keepassxc.org>
#
#  This program is free software: you can redistribute it and/or modify
#  it under the terms of the GNU General Public License as published by
#  the Free Software Foundation, version 3.
#
#  This program is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU General Public License for more details.
#
#  You should have received a copy of the GNU General Public License
#  along with this program.  If not, see <http://www.gnu.org/licenses/>.

find_path(ZXCVBN_INCLUDE_DIR
	NAMES
	zxcvbn.h
	PATH_SUFFIXES
	zxcvbn
	)

find_library(ZXCVBN_LIBRARY zxcvbn)

mark_as_advanced(ZXCVBN_LIBRARY ZXCVBN_INCLUDE_DIR)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(ZXCVBN DEFAULT_MSG ZXCVBN_LIBRARY ZXCVBN_INCLUDE_DIR)
