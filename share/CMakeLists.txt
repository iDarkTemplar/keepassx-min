#  Copyright (C) 2026 i.Dark_Templar <darktemplar@dark-templar-archives.net>
#  Copyright (C) 2011 Felix Geyer <debfx@fobos.de>
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

add_subdirectory(translations)

file(GLOB wordlists_files "wordlists/*.wordlist")
install(FILES ${wordlists_files} DESTINATION ${CMAKE_INSTALL_DATADIR}/keepassxm/wordlists)

# Copy wordlists to build dir for use in tests
file(COPY "wordlists" DESTINATION ${CMAKE_CURRENT_BINARY_DIR})

set(APP_ICON_NAME "keepassxm")
set(MIME_ICON "application-x-keepassxm")
configure_file(linux/keepassx-min.xml.in ${CMAKE_CURRENT_BINARY_DIR}/linux/keepassxm.xml @ONLY)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/linux/keepassxm.xml DESTINATION ${CMAKE_INSTALL_DATADIR}/mime/packages)

install(DIRECTORY icons/application/ DESTINATION ${CMAKE_INSTALL_DATADIR}/icons/hicolor
	FILES_MATCHING PATTERN "keepassx*.png" PATTERN "keepassx*.svg"
	PATTERN "status" EXCLUDE PATTERN "actions" EXCLUDE PATTERN "categories" EXCLUDE)

install(DIRECTORY icons/application/ DESTINATION ${CMAKE_INSTALL_DATADIR}/icons/hicolor
	FILES_MATCHING PATTERN "application-x-keepassxm.svg" PATTERN "status"
	EXCLUDE PATTERN "actions" EXCLUDE PATTERN "categories" EXCLUDE)

configure_file(linux/${APP_ID}.desktop.in ${CMAKE_CURRENT_BINARY_DIR}/linux/${APP_ID}.desktop @ONLY)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/linux/${APP_ID}.desktop DESTINATION ${CMAKE_INSTALL_DATADIR}/applications)
install(FILES linux/${APP_ID}.appdata.xml DESTINATION ${CMAKE_INSTALL_DATADIR}/metainfo)

install(FILES icons/application/256x256/apps/keepassxm.png DESTINATION ${CMAKE_INSTALL_DATADIR}/keepassxm/icons/application/256x256/apps)

add_custom_target(icons)
add_custom_command(TARGET icons
	POST_BUILD
	COMMAND bash ./icons/minify.sh
	WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
