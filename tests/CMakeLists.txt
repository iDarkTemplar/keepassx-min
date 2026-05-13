#  Copyright (C) 2026 i.Dark_Templar <darktemplar@dark-templar-archives.net>
#  Copyright (C) 2023 KeePassXC Team <team@keepassxc.org>
#  Copyright (C) 2010 Felix Geyer <debfx@fobos.de>
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

include_directories(
	${CMAKE_CURRENT_SOURCE_DIR}
	${CMAKE_CURRENT_BINARY_DIR}
	${CMAKE_SOURCE_DIR}/src
	${CMAKE_CURRENT_BINARY_DIR}/../src)

add_definitions(-DQT_TEST_LIB)

set(KEEPASSX_TEST_DATA_DIR ${CMAKE_CURRENT_SOURCE_DIR}/data)
configure_file(config-keepassx-tests.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/config-keepassx-tests.h)

macro(parse_arguments prefix arg_names option_names)
	set(DEFAULT_ARGS)

	foreach(arg_name ${arg_names})
		set(${prefix}_${arg_name})
	endforeach(arg_name)

	foreach(option ${option_names})
		set(${prefix}_${option} FALSE)
	endforeach(option)

	set(current_arg_name DEFAULT_ARGS)
	set(current_arg_list)

	foreach(arg ${ARGN})
		set(larg_names ${arg_names})
		list(FIND larg_names "${arg}" is_arg_name)
		if(is_arg_name GREATER -1)
			set(${prefix}_${current_arg_name} ${current_arg_list})
			set(current_arg_name ${arg})
			set(current_arg_list)
		else()
			set(loption_names ${option_names})
			list(FIND loption_names "${arg}" is_option)
			if(is_option GREATER -1)
				set(${prefix}_${arg} TRUE)
			else(is_option GREATER -1)
				set(current_arg_list ${current_arg_list} ${arg})
			endif()
		endif()
	endforeach(arg)

	set(${prefix}_${current_arg_name} ${current_arg_list})
endmacro(parse_arguments)

macro(add_unit_test)
	parse_arguments(TEST "NAME;SOURCES;HEADERS;QT_HEADERS;LIBS;LAUNCHER" "" ${ARGN})
	set(_test_NAME ${TEST_NAME})
	set(_test_LAUNCHER ${TEST_LAUNCHER})
	set(_srcList ${TEST_SOURCES})
	set(_hdrList ${TEST_HEADERS})
	set(_qthdrList ${TEST_QT_HEADERS})
	set(_mocqthdrList)

	if (_qthdrList)
		qt_wrap_cpp(_mocqthdrList ${_qthdrList})
	endif (_qthdrList)

	add_executable(${_test_NAME} ${_srcList} ${_hdrList} ${_qthdrList} ${_mocqthdrList})
	target_link_libraries(${_test_NAME} ${TEST_LIBS})
	add_test(${_test_NAME} ${_test_LAUNCHER} ${_test_NAME})
endmacro(add_unit_test)

set(TEST_LIBRARIES keepassx_core Qt6::Test)

set(testsupport_SOURCES
	FailDevice.cpp
	mock/MockClock.cpp
	util/TemporaryFile.cpp
	)

set(testsupport_QT_HEADERS
	FailDevice.h
	mock/MockClock.h
	util/TemporaryFile.h
	)

qt_wrap_cpp(testsupport_MOC_SOURCES ${testsupport_QT_HEADERS})

add_library(testsupport STATIC ${testsupport_SOURCES} ${testsupport_QT_HEADERS} ${testsupport_MOC_SOURCES})
target_link_libraries(testsupport Qt6::Core Qt6::Concurrent Qt6::Widgets Qt6::Test)

add_unit_test(NAME testgroup
	SOURCES
	TestGroup.cpp
	QT_HEADERS
	TestGroup.h
	LIBS testsupport ${TEST_LIBRARIES})

add_unit_test(NAME testkdbx2
	SOURCES
	TestKdbx2.cpp
	QT_HEADERS
	TestKdbx2.h
	LIBS testsupport ${TEST_LIBRARIES})

add_unit_test(NAME testkdbx3
	SOURCES
	TestKeePass2Format.cpp
	FailDevice.cpp
	TestKdbx3.cpp
	QT_HEADERS
	TestKeePass2Format.h
	FailDevice.h
	TestKdbx3.h
	LIBS testsupport ${TEST_LIBRARIES})

add_unit_test(NAME testkdbx4
	SOURCES
	TestKeePass2Format.cpp
	FailDevice.cpp
	TestKdbx4.cpp
	QT_HEADERS
	TestKeePass2Format.h
	FailDevice.h
	TestKdbx4.h
	LIBS testsupport ${TEST_LIBRARIES})

add_unit_test(NAME testkeys
	SOURCES
	TestKeys.cpp
	QT_HEADERS
	TestKeys.h
	LIBS ${TEST_LIBRARIES})

add_unit_test(NAME testgroupmodel
	SOURCES
	TestGroupModel.cpp
	QT_HEADERS
	TestGroupModel.h
	LIBS testsupport ${TEST_LIBRARIES})

add_unit_test(NAME testentrymodel
	SOURCES
	TestEntryModel.cpp
	QT_HEADERS
	TestEntryModel.h
	LIBS testsupport ${TEST_LIBRARIES})

add_unit_test(NAME testcryptohash
	SOURCES
	TestCryptoHash.cpp
	QT_HEADERS
	TestCryptoHash.h
	LIBS ${TEST_LIBRARIES})

add_unit_test(NAME testsymmetriccipher
	SOURCES
	TestSymmetricCipher.cpp
	QT_HEADERS
	TestSymmetricCipher.h
	LIBS ${TEST_LIBRARIES})

add_unit_test(NAME testhashedblockstream
	SOURCES
	TestHashedBlockStream.cpp
	QT_HEADERS
	TestHashedBlockStream.h
	LIBS testsupport ${TEST_LIBRARIES})

add_unit_test(NAME testkeepass2randomstream
	SOURCES
	TestKeePass2RandomStream.cpp
	QT_HEADERS
	TestKeePass2RandomStream.h
	LIBS ${TEST_LIBRARIES})

add_unit_test(NAME testmodified
	SOURCES
	TestModified.cpp
	QT_HEADERS
	TestModified.h
	LIBS testsupport ${TEST_LIBRARIES})

add_unit_test(NAME testdeletedobjects
	SOURCES
	TestDeletedObjects.cpp
	QT_HEADERS
	TestDeletedObjects.h
	LIBS ${TEST_LIBRARIES})

add_unit_test(NAME testimports
	SOURCES
	TestImports.cpp
	QT_HEADERS
	TestImports.h
	LIBS ${TEST_LIBRARIES})

add_unit_test(NAME testentry
	SOURCES
	TestEntry.cpp
	QT_HEADERS
	TestEntry.h
	LIBS ${TEST_LIBRARIES})

add_unit_test(NAME testmerge
	SOURCES
	TestMerge.cpp
	QT_HEADERS
	TestMerge.h
	LIBS testsupport ${TEST_LIBRARIES})

add_unit_test(NAME testpasswordgenerator
	SOURCES
	TestPasswordGenerator.cpp
	QT_HEADERS
	TestPasswordGenerator.h
	LIBS ${TEST_LIBRARIES})

add_unit_test(NAME testpasswordhealth
	SOURCES
	TestPasswordHealth.cpp
	QT_HEADERS
	TestPasswordHealth.h
	LIBS ${TEST_LIBRARIES})

add_unit_test(NAME testpassphrasegenerator
	SOURCES
	TestPassphraseGenerator.cpp
	QT_HEADERS
	TestPassphraseGenerator.h
	LIBS ${TEST_LIBRARIES})

add_unit_test(NAME testtotp
	SOURCES
	TestTotp.cpp
	QT_HEADERS
	TestTotp.h
	LIBS ${TEST_LIBRARIES})

add_unit_test(NAME testbase32
	SOURCES
	TestBase32.cpp
	QT_HEADERS
	TestBase32.h
	LIBS ${TEST_LIBRARIES})

add_unit_test(NAME testcsvparser
	SOURCES
	TestCsvParser.cpp
	QT_HEADERS
	TestCsvParser.h
	LIBS ${TEST_LIBRARIES})

add_unit_test(NAME testrandomgenerator
	SOURCES
	TestRandomGenerator.cpp
	QT_HEADERS
	TestRandomGenerator.h
	LIBS testsupport ${TEST_LIBRARIES})

add_unit_test(NAME testentrysearcher
	SOURCES
	TestEntrySearcher.cpp
	QT_HEADERS
	TestEntrySearcher.h
	LIBS ${TEST_LIBRARIES})

add_unit_test(NAME testcsvexporter
	SOURCES
	TestCsvExporter.cpp
	QT_HEADERS
	TestCsvExporter.h
	LIBS ${TEST_LIBRARIES})

add_unit_test(NAME testdatabase
	SOURCES
	TestDatabase.cpp
	QT_HEADERS
	TestDatabase.h
	LIBS testsupport ${TEST_LIBRARIES})

add_unit_test(NAME testtools
	SOURCES
	TestTools.cpp
	QT_HEADERS
	TestTools.h
	LIBS ${TEST_LIBRARIES})

add_unit_test(NAME testconfig
	SOURCES
	TestConfig.cpp
	QT_HEADERS
	TestConfig.h
	LIBS testsupport ${TEST_LIBRARIES})
