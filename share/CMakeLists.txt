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

# Copy wordlists to build dir for use in tests
add_custom_target(wordlists)

file(GLOB filelist RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}/wordlists ${CMAKE_CURRENT_SOURCE_DIR}/wordlists/*.wordlist)
foreach(filename ${filelist})
	if(NOT IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/wordlists/${filename}")
		list(APPEND WORDLISTS_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/wordlists/${filename}")
		list(APPEND WORDLISTS_CLEAN_FILES "${CMAKE_BINARY_DIR}/share/keepassxmin/${filename}")

		add_custom_command(OUTPUT "${CMAKE_BINARY_DIR}/share/keepassxmin/wordlists/${filename}"
			COMMAND ${CMAKE_COMMAND} -E echo Copying wordlists ${filename}
			COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_BINARY_DIR}/share/keepassxmin/wordlists"
			COMMAND ${CMAKE_COMMAND} -E copy_if_different "${CMAKE_CURRENT_SOURCE_DIR}/wordlists/${filename}" "${CMAKE_BINARY_DIR}/share/keepassxmin/wordlists/${filename}"
			DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/wordlists/${filename}")
		target_sources(wordlists PRIVATE "${CMAKE_BINARY_DIR}/share/keepassxmin/wordlists/${filename}")
	endif()
endforeach()

target_sources(wordlists PRIVATE ${WORDLISTS_SOURCES})
set_target_properties(wordlists PROPERTIES ADDITIONAL_CLEAN_FILES "${WORDLISTS_CLEAN_FILES}")

add_dependencies(keepassx-min wordlists)

file(GLOB wordlists_files "wordlists/*.wordlist")
install(FILES ${wordlists_files} DESTINATION ${CMAKE_INSTALL_DATADIR}/keepassxmin/wordlists)

set(APP_ICON_NAME "keepassxmin")
set(MIME_ICON "application-x-keepassxmin")
configure_file(linux/keepassx-min.xml.in ${CMAKE_CURRENT_BINARY_DIR}/linux/keepassxmin.xml @ONLY)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/linux/keepassxmin.xml DESTINATION ${CMAKE_INSTALL_DATADIR}/mime/packages)

install(DIRECTORY icons/application/ DESTINATION ${CMAKE_INSTALL_DATADIR}/icons/hicolor
	FILES_MATCHING PATTERN "keepassxmin.png" PATTERN "keepassxmin.svg"
	PATTERN "status" EXCLUDE PATTERN "actions" EXCLUDE PATTERN "categories" EXCLUDE)

install(DIRECTORY icons/application/ DESTINATION ${CMAKE_INSTALL_DATADIR}/icons/hicolor
	FILES_MATCHING PATTERN "application-x-keepassxmin.svg" PATTERN "status"
	EXCLUDE PATTERN "actions" EXCLUDE PATTERN "categories" EXCLUDE)

configure_file(linux/${APP_ID}.desktop.in ${CMAKE_CURRENT_BINARY_DIR}/linux/${APP_ID}.desktop @ONLY)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/linux/${APP_ID}.desktop DESTINATION ${CMAKE_INSTALL_DATADIR}/applications)
