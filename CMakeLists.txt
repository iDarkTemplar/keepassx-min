#  Copyright (C) 2026 i.Dark_Templar <darktemplar@dark-templar-archives.net>
#  Copyright (C) 2018 KeePassXC Team <team@keepassxc.org>
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

cmake_minimum_required(VERSION 4.2.0)

project(KeePassX-min)

set(APP_ID "keepassx-min")

if(NOT CMAKE_BUILD_TYPE)
	set(CMAKE_BUILD_TYPE "Debug" CACHE STRING
		"Choose the type of build, options are: Debug Release RelWithDebInfo Profile"
		FORCE)
endif()

if(CMAKE_BUILD_TYPE STREQUAL "Debug" OR CMAKE_BUILD_TYPE STREQUAL "RelWithDebInfo")
	set(IS_DEBUG_BUILD TRUE)
endif()

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_CURRENT_SOURCE_DIR}/cmake)

set(CMAKE_C_STANDARD 99)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_SCAN_FOR_MODULES OFF)

include(FeatureSummary)
include(CheckCCompilerFlag)
include(CheckCXXCompilerFlag)
include(CheckCXXSourceCompiles)

option(WITH_TESTS "Enable building of unit tests" OFF)
option(WITH_GUI_TESTS "Enable building of GUI tests" OFF)
option(WITH_ASAN "Enable address sanitizer checks (Linux / macOS only)" OFF)
option(WITH_COVERAGE "Use to build with coverage tests (GCC only)." OFF)

option(WITH_XC_FDOSECRETS "Implement freedesktop.org Secret Storage Spec server side API." OFF)

# Retrieve git info
file(READ ${CMAKE_SOURCE_DIR}/VERSION GIT_VERSION)
if ("${GIT_VERSION}" STREQUAL "$Format:%(describe:tags)$\n")
	execute_process(COMMAND git describe HEAD
		WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
		OUTPUT_VARIABLE GIT_VERSION
		OUTPUT_STRIP_TRAILING_WHITESPACE
		ERROR_QUIET)
endif()
string(REGEX REPLACE "\n$" "" GIT_VERSION "${GIT_VERSION}")
message(STATUS "Found Git version info: ${GIT_VERSION}")

file(READ ${CMAKE_SOURCE_DIR}/HASH GIT_HASH)
if ("${GIT_HASH}" STREQUAL "$Format:%H$\n")
	execute_process(COMMAND git rev-parse HEAD
		WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
		OUTPUT_VARIABLE GIT_HASH
		OUTPUT_STRIP_TRAILING_WHITESPACE
		ERROR_QUIET)
endif()
string(REGEX REPLACE "\n$" "" GIT_HASH "${GIT_HASH}")
message(STATUS "Found Git hash: ${GIT_HASH}")

cmake_policy(SET CMP0083 NEW)
include(CheckPIESupported)
check_pie_supported()
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

set(CLANG_COMPILER_ID_REGEX "^(Apple)?[Cc]lang$")
if ("${CMAKE_C_COMPILER}" MATCHES "clang$"
		OR "${CMAKE_EXTRA_GENERATOR_C_SYSTEM_DEFINED_MACROS}" MATCHES "__clang__"
		OR "${CMAKE_C_COMPILER_ID}" MATCHES ${CLANG_COMPILER_ID_REGEX})
	set(CMAKE_COMPILER_IS_CLANG 1)
endif()

if ("${CMAKE_CXX_COMPILER}" MATCHES "clang(\\+\\+)?$"
		OR "${CMAKE_EXTRA_GENERATOR_CXX_SYSTEM_DEFINED_MACROS}" MATCHES "__clang__"
		OR "${CMAKE_CXX_COMPILER_ID}" MATCHES ${CLANG_COMPILER_ID_REGEX})
	set(CMAKE_COMPILER_IS_CLANGXX 1)
endif()

macro(add_gcc_compiler_cxxflags FLAGS)
	if (CMAKE_COMPILER_IS_GNUCXX OR CMAKE_COMPILER_IS_CLANGXX)
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${FLAGS}")
	endif()
endmacro(add_gcc_compiler_cxxflags)

macro(add_gcc_compiler_cflags FLAGS)
	if (CMAKE_COMPILER_IS_GNUCC OR CMAKE_COMPILER_IS_CLANG)
		set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${FLAGS}")
	endif()
endmacro(add_gcc_compiler_cflags)

macro(add_gcc_compiler_flags FLAGS)
	add_gcc_compiler_cxxflags("${FLAGS}")
	add_gcc_compiler_cflags("${FLAGS}")
endmacro(add_gcc_compiler_flags)

# Copies of above macros that first ensure the compiler understands a given flag
# Because check_*_compiler_flag() sets -D with name, need to provide "safe" FLAGNAME
macro(check_add_gcc_compiler_cxxflag FLAG FLAGNAME)
	check_cxx_compiler_flag("${FLAG}" CXX_HAS${FLAGNAME})
	if (CXX_HAS${FLAGNAME})
		add_gcc_compiler_cxxflags("${FLAG}")
	endif()
endmacro(check_add_gcc_compiler_cxxflag)

macro(check_add_gcc_compiler_cflag FLAG FLAGNAME)
	check_c_compiler_flag("${FLAG}" CC_HAS${FLAGNAME})
	if (CC_HAS${FLAGNAME})
		add_gcc_compiler_cflags("${FLAG}")
	endif()
endmacro(check_add_gcc_compiler_cflag)

# This is the "front-end" for the above macros
# Optionally takes additional parameter(s) with language to check (currently "C" or "CXX")
macro(check_add_gcc_compiler_flag FLAG)
	string(REGEX REPLACE "[-=]" "_" FLAGNAME "${FLAG}")
	set(check_lang_spec ${ARGN})
	list(LENGTH check_lang_spec num_extra_args)
	set(langs C CXX)
	if (num_extra_args GREATER 0)
		set(langs "${check_lang_spec}")
	endif()
	if ("C" IN_LIST langs)
		check_add_gcc_compiler_cflag("${FLAG}" "${FLAGNAME}")
	endif()
	if ("CXX" IN_LIST langs)
		check_add_gcc_compiler_cxxflag("${FLAG}" "${FLAGNAME}")
	endif()
endmacro(check_add_gcc_compiler_flag)

add_definitions(-DQT_NO_EXCEPTIONS -DQT_STRICT_ITERATORS -DQT_NO_CAST_TO_ASCII)
if (NOT IS_DEBUG_BUILD)
	add_definitions(-DQT_NO_DEBUG_OUTPUT)
endif()

add_gcc_compiler_flags("-fno-common")
find_package(OpenMP)
if (OpenMP_FOUND)
	add_gcc_compiler_cflags(${OpenMP_C_FLAGS})
	add_gcc_compiler_cxxflags(${OpenMP_CXX_FLAGS})
endif()
add_gcc_compiler_flags("-Wall -Wextra -Wundef -Wpointer-arith -Wno-long-long")
add_gcc_compiler_flags("-Wformat=2 -Wmissing-format-attribute")
add_gcc_compiler_flags("-fvisibility=hidden")
add_gcc_compiler_cxxflags("-fvisibility-inlines-hidden")

if (IS_DEBUG_BUILD)
	check_add_gcc_compiler_flag("-Wshadow-compatible-local")
	check_add_gcc_compiler_flag("-Wshadow-local")
	# TODO: re-enable Werror in debug builds
	# add_gcc_compiler_flags("-Werror")
	# This is needed since compiling against Botan3 requires compiling against C++20
	add_gcc_compiler_cxxflags("-Wno-error=deprecated-enum-enum-conversion -Wno-error=deprecated")
endif()

if ((CMAKE_COMPILER_IS_GNUCXX AND CMAKE_CXX_COMPILER_VERSION VERSION_GREATER 4.8.999) OR CMAKE_COMPILER_IS_CLANGXX)
	add_gcc_compiler_flags("-fstack-protector-strong")
else()
	add_gcc_compiler_flags("-fstack-protector --param=ssp-buffer-size=4")
endif()

add_gcc_compiler_cxxflags("-Wnon-virtual-dtor -Wold-style-cast -Woverloaded-virtual")
add_gcc_compiler_cflags("-Wchar-subscripts -Wwrite-strings")

if (WITH_ASAN)
	if (NOT CMAKE_SYSTEM_NAME STREQUAL "Linux")
		message(FATAL_ERROR "WITH_ASAN is only supported on Linux at the moment.")
	endif()

	add_gcc_compiler_flags("-fsanitize=address -DWITH_ASAN")

	if (NOT (CMAKE_COMPILER_IS_GNUCXX AND CMAKE_CXX_COMPILER_VERSION VERSION_LESS 4.9))
		add_gcc_compiler_flags("-fsanitize=leak -DWITH_LSAN")
	endif()
endif()

if (CMAKE_BUILD_TYPE MATCHES "(Release|RelWithDebInfo|MinSizeRel)")
	add_gcc_compiler_flags("-U_FORTIFY_SOURCE -D_FORTIFY_SOURCE=2")
endif()

check_add_gcc_compiler_flag("-Werror=format-security")
check_add_gcc_compiler_flag("-Werror=implicit-function-declaration" C)
check_add_gcc_compiler_flag("-Wcast-align")

check_add_gcc_compiler_flag("-Qunused-arguments")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,--as-needed -Wl,--no-undefined")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,-z,relro,-z,now -pie")
set(CMAKE_MODULE_LINKER_FLAGS "${CMAKE_MODULE_LINKER_FLAGS} -Wl,--as-needed")
set(CMAKE_MODULE_LINKER_FLAGS "${CMAKE_MODULE_LINKER_FLAGS} -Wl,-z,relro,-z,now")

check_cxx_compiler_flag("-fsized-deallocation" CXX_HAS_fsized_deallocation)
if (CXX_HAS_fsized_deallocation)
	# Do additional check: the deallocation functions must be there too.
	set(CMAKE_REQUIRED_FLAGS "-fsized-deallocation")
	file(READ ${CMAKE_SOURCE_DIR}/cmake/dealloc_function.cpp SOURCE_FILE)
	check_cxx_source_compiles("${SOURCE_FILE}" HAVE_DEALLOCATION_FUNCTIONS)
	if (HAVE_DEALLOCATION_FUNCTIONS)
		check_add_gcc_compiler_flag("-fsized-deallocation" CXX)
	endif()
	unset(CMAKE_REQUIRED_FLAGS)
endif()

file(READ ${CMAKE_SOURCE_DIR}/cmake/pr_set_dumpable.c SOURCE_FILE)
check_c_source_compiles("${SOURCE_FILE}" HAVE_PR_SET_DUMPABLE)

file(READ ${CMAKE_SOURCE_DIR}/cmake/have_malloc_h.c SOURCE_FILE)
check_c_source_compiles("${SOURCE_FILE}" HAVE_MALLOC_H)

file(READ ${CMAKE_SOURCE_DIR}/cmake/have_malloc_usable_size.c SOURCE_FILE)
check_c_source_compiles("${SOURCE_FILE}" HAVE_MALLOC_USABLE_SIZE)

file(READ ${CMAKE_SOURCE_DIR}/cmake/have_rlimit_core.c SOURCE_FILE)
check_c_source_compiles("${SOURCE_FILE}" HAVE_RLIMIT_CORE)

if (IS_DEBUG_BUILD)
	add_definitions(-DQT_DEPRECATED_WARNINGS)
endif()

if (WITH_COVERAGE)
	# Include code coverage, use with -DCMAKE_BUILD_TYPE=Debug
	include(CodeCoverage)
	append_coverage_compiler_flags()

	set(COVERAGE_EXCLUDES
		"'^(.+/)?main\\.cpp$$'")
	if (WITH_COVERAGE AND CMAKE_COMPILER_IS_CLANGXX)
		set(MAIN_BINARIES
			"$<TARGET_FILE:keepassx-min>")
		setup_target_for_coverage_llvm(
			NAME coverage
			BINARY ${MAIN_BINARIES}
			SOURCES_ROOT ${CMAKE_SOURCE_DIR}/src
		)
	else()
		setup_target_for_coverage_gcovr(
			NAME coverage
			SOURCES_ROOT ${CMAKE_SOURCE_DIR}/src
		)
	endif()
endif()

find_package(Botan 3.0.0 REQUIRED)
include_directories(SYSTEM ${BOTAN_INCLUDE_DIR})

# Find Argon2 -- Botan 2.18 and below does not support threaded Argon2
find_library(ARGON2_LIBRARIES NAMES argon2)
find_path(ARGON2_INCLUDE_DIR NAMES argon2.h PATH_SUFFIXES local/include)
include_directories(SYSTEM ${ARGON2_INCLUDE_DIR})

# Find zlib
find_package(ZLIB 1.2.0 REQUIRED)
include_directories(SYSTEM ${ZLIB_INCLUDE_DIR})

# Find Minizip
find_package(Minizip REQUIRED)

include_directories(SYSTEM ${ZLIB_INCLUDE_DIR})

find_package(ZXCVBN REQUIRED)

find_package(QREncode REQUIRED)

set(QT_COMPONENTS Core DBus Network Concurrent Gui Svg Widgets LinguistTools)
if (WITH_TESTS)
	list(APPEND QT_COMPONENTS Test)
endif()
find_package(Qt5 5.12.0 COMPONENTS ${QT_COMPONENTS} REQUIRED)

include(GNUInstallDirs)

add_subdirectory(src)
add_subdirectory(share)

if (WITH_TESTS)
	enable_testing()
	add_subdirectory(tests)
endif(WITH_TESTS)

# This will only print ENABLED and DISABLED feature
feature_summary(WHAT ENABLED_FEATURES DESCRIPTION "Enabled features:")
feature_summary(WHAT DISABLED_FEATURES DESCRIPTION "Disabled features:")
