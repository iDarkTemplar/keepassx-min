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

cmake_minimum_required(VERSION 3.10.0)

project(KeePassXC)
set(APP_ID "org.keepassxc.${PROJECT_NAME}")

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "RelWithDebInfo" CACHE STRING
            "Choose the type of build, options are: Debug Release RelWithDebInfo Profile"
            FORCE)
endif()
string(TOLOWER "${CMAKE_BUILD_TYPE}" CMAKE_BUILD_TYPE_LOWER)
if(CMAKE_BUILD_TYPE_LOWER STREQUAL "debug" OR CMAKE_BUILD_TYPE_LOWER STREQUAL "relwithdebinfo")
    set(IS_DEBUG_BUILD TRUE)
endif()

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_CURRENT_SOURCE_DIR}/cmake)

# Support Visual Studio Code
include(FeatureSummary)

include(CheckCCompilerFlag)
include(CheckCXXCompilerFlag)
include(CheckCXXSourceCompiles)

option(WITH_TESTS "Enable building of unit tests" OFF)
option(WITH_GUI_TESTS "Enable building of GUI tests" OFF)
option(WITH_DEV_BUILD "Use only for development. Disables/warns about deprecated methods." OFF)
option(WITH_ASAN "Enable address sanitizer checks (Linux / macOS only)" OFF)
option(WITH_COVERAGE "Use to build with coverage tests (GCC only)." OFF)
option(WITH_APP_BUNDLE "Enable Application Bundle for macOS" ON)
option(WITH_CCACHE "Use ccache for build" OFF)

set(WITH_XC_ALL OFF CACHE BOOL "Build in all available plugins")

option(WITH_XC_AUTOTYPE "Include Auto-Type." OFF)
option(WITH_XC_NETWORKING "Include networking code (e.g. for downloading website icons)." OFF)
option(WITH_XC_YUBIKEY "Include YubiKey support." OFF)
option(WITH_XC_SSHAGENT "Include SSH agent support." OFF)
option(WITH_XC_FDOSECRETS "Implement freedesktop.org Secret Storage Spec server side API." OFF)
set(WITH_XC_X11 ON CACHE BOOL "Enable building with X11 deps")

set(KEEPASSXC_VERSION_MAJOR "2")
set(KEEPASSXC_VERSION_MINOR "7")
set(KEEPASSXC_VERSION_PATCH "11")
set(KEEPASSXC_VERSION "${KEEPASSXC_VERSION_MAJOR}.${KEEPASSXC_VERSION_MINOR}.${KEEPASSXC_VERSION_PATCH}")
set(OVERRIDE_VERSION "" CACHE STRING "Override the KeePassXC Version for Snapshot builds")

set(KEEPASSXC_BUILD_TYPE "Snapshot" CACHE STRING "Set KeePassXC build type to distinguish between stable releases and snapshots")
set_property(CACHE KEEPASSXC_BUILD_TYPE PROPERTY STRINGS Snapshot Release PreRelease)

# Retrieve git HEAD revision hash
set(GIT_HEAD_OVERRIDE "" CACHE STRING "Manually set the Git HEAD hash when missing (eg, when no .git folder exists)")
execute_process(COMMAND git rev-parse --short=7 HEAD
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        OUTPUT_VARIABLE GIT_HEAD
        ERROR_QUIET)
string(STRIP "${GIT_HEAD}" GIT_HEAD)
if(GIT_HEAD STREQUAL "" AND NOT GIT_HEAD_OVERRIDE STREQUAL "")
    string(SUBSTRING "${GIT_HEAD_OVERRIDE}" 0 7 GIT_HEAD)
elseif(EXISTS ${CMAKE_SOURCE_DIR}/.gitrev)
    file(READ ${CMAKE_SOURCE_DIR}/.gitrev GIT_HEAD)
endif()
message(STATUS "Found Git HEAD Revision: ${GIT_HEAD}\n")

# Check if on a tag, if so build as a release
execute_process(COMMAND git tag --points-at HEAD
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        OUTPUT_VARIABLE GIT_TAG
        ERROR_QUIET)
string(REGEX REPLACE "latest" "" GIT_TAG "${GIT_TAG}")
if(GIT_TAG MATCHES "[0-9]+\.[0-9]+\.[0-9]+")
    string(STRIP "${GIT_TAG}" GIT_TAG)
    set(OVERRIDE_VERSION ${GIT_TAG})
elseif(EXISTS ${CMAKE_SOURCE_DIR}/.version)
    file(READ ${CMAKE_SOURCE_DIR}/.version OVERRIDE_VERSION)
endif()

string(REGEX REPLACE "(\r?\n)+" "" OVERRIDE_VERSION "${OVERRIDE_VERSION}")
if(OVERRIDE_VERSION)
    if(OVERRIDE_VERSION MATCHES "^[\\.0-9]+-beta[0-9]*")
        set(KEEPASSXC_BUILD_TYPE "PreRelease")
        set(KEEPASSXC_VERSION ${OVERRIDE_VERSION})
    elseif(OVERRIDE_VERSION MATCHES "^[\\.0-9]+$")
        set(KEEPASSXC_BUILD_TYPE "Release")
        set(KEEPASSXC_VERSION ${OVERRIDE_VERSION})
    else()
        set(KEEPASSXC_BUILD_TYPE "Snapshot")
        set(KEEPASSXC_VERSION ${OVERRIDE_VERSION})
    endif()
else()
    if(KEEPASSXC_BUILD_TYPE STREQUAL "PreRelease")
        set(KEEPASSXC_VERSION "${KEEPASSXC_VERSION}-preview")
    elseif(KEEPASSXC_BUILD_TYPE STREQUAL "Snapshot")
        set(KEEPASSXC_VERSION "${KEEPASSXC_VERSION}-snapshot")
    endif()
endif()

if(KEEPASSXC_BUILD_TYPE STREQUAL "Release")
    set(KEEPASSXC_BUILD_TYPE_RELEASE ON)
elseif(KEEPASSXC_BUILD_TYPE STREQUAL "PreRelease")
    set(KEEPASSXC_BUILD_TYPE_PRE_RELEASE ON)
else()
    set(KEEPASSXC_BUILD_TYPE_SNAPSHOT ON)
endif()

message(STATUS "Setting up build for KeePassXC v${KEEPASSXC_VERSION}\n")

cmake_policy(SET CMP0083 NEW)
include(CheckPIESupported)
check_pie_supported()

find_package(ZXCVBN REQUIRED)

# Find Botan early since the version affects subsequent compiler options
find_package(Botan REQUIRED)
if(BOTAN_VERSION VERSION_GREATER_EQUAL "3.0.0")
    set(WITH_XC_BOTAN3 TRUE)
elseif(BOTAN_VERSION VERSION_LESS "2.11.0")
    # Check for minimum Botan version
    message(FATAL_ERROR "Botan 2.11.0 or higher is required")
endif()
include_directories(SYSTEM ${BOTAN_INCLUDE_DIR})

# Create position independent code for shared libraries and executables
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

if("${CMAKE_SIZEOF_VOID_P}" EQUAL "4")
    set(IS_32BIT TRUE)
endif()

if("${CMAKE_CXX_COMPILER}" MATCHES "clang-cl(.exe)?$")
    # clang-cl uses MSVC compiler flags
    set(MSVC 1)
    set(CMAKE_COMPILER_IS_CLANG_MSVC 1)
else()
    set(CLANG_COMPILER_ID_REGEX "^(Apple)?[Cc]lang$")
    if("${CMAKE_C_COMPILER}" MATCHES "clang$"
            OR "${CMAKE_EXTRA_GENERATOR_C_SYSTEM_DEFINED_MACROS}" MATCHES "__clang__"
            OR "${CMAKE_C_COMPILER_ID}" MATCHES ${CLANG_COMPILER_ID_REGEX})
        set(CMAKE_COMPILER_IS_CLANG 1)
    endif()

    if("${CMAKE_CXX_COMPILER}" MATCHES "clang(\\+\\+)?$"
            OR "${CMAKE_EXTRA_GENERATOR_CXX_SYSTEM_DEFINED_MACROS}" MATCHES "__clang__"
            OR "${CMAKE_CXX_COMPILER_ID}" MATCHES ${CLANG_COMPILER_ID_REGEX})
        set(CMAKE_COMPILER_IS_CLANGXX 1)
    endif()
endif()

macro(add_gcc_compiler_cxxflags FLAGS)
    if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_COMPILER_IS_CLANGXX)
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${FLAGS}")
    endif()
endmacro(add_gcc_compiler_cxxflags)

macro(add_gcc_compiler_cflags FLAGS)
    if(CMAKE_COMPILER_IS_GNUCC OR CMAKE_COMPILER_IS_CLANG)
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
    if(CXX_HAS${FLAGNAME})
        add_gcc_compiler_cxxflags("${FLAG}")
    endif()
endmacro(check_add_gcc_compiler_cxxflag)

macro(check_add_gcc_compiler_cflag FLAG FLAGNAME)
    check_c_compiler_flag("${FLAG}" CC_HAS${FLAGNAME})
    if(CC_HAS${FLAGNAME})
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
    if(num_extra_args GREATER 0)
        set(langs "${check_lang_spec}")
    endif()
    if("C" IN_LIST langs)
        check_add_gcc_compiler_cflag("${FLAG}" "${FLAGNAME}")
    endif()
    if("CXX" IN_LIST langs)
        check_add_gcc_compiler_cxxflag("${FLAG}" "${FLAGNAME}")
    endif()
endmacro(check_add_gcc_compiler_flag)

add_definitions(-DQT_NO_EXCEPTIONS -DQT_STRICT_ITERATORS -DQT_NO_CAST_TO_ASCII)
if(NOT IS_DEBUG_BUILD)
    add_definitions(-DQT_NO_DEBUG_OUTPUT)
endif()

add_gcc_compiler_flags("-fno-common")
find_package(OpenMP)
if(OpenMP_FOUND)
    add_gcc_compiler_cflags(${OpenMP_C_FLAGS})
    add_gcc_compiler_cxxflags(${OpenMP_CXX_FLAGS})
endif()
add_gcc_compiler_flags("-Wall -Wextra -Wundef -Wpointer-arith -Wno-long-long")
add_gcc_compiler_flags("-Wformat=2 -Wmissing-format-attribute")
add_gcc_compiler_flags("-fvisibility=hidden")
add_gcc_compiler_cxxflags("-fvisibility-inlines-hidden")

if(CMAKE_BUILD_TYPE_LOWER STREQUAL "debug")
    check_add_gcc_compiler_flag("-Wshadow-compatible-local")
    check_add_gcc_compiler_flag("-Wshadow-local")
    add_gcc_compiler_flags("-Werror")
    # This is needed since compiling aginst Botan3 requires compiling against C++20
    if(WITH_XC_BOTAN3)
        add_gcc_compiler_cxxflags("-Wno-error=deprecated-enum-enum-conversion -Wno-error=deprecated")
    endif()
endif()

if((CMAKE_COMPILER_IS_GNUCXX AND CMAKE_CXX_COMPILER_VERSION VERSION_GREATER 4.8.999) OR CMAKE_COMPILER_IS_CLANGXX)
    add_gcc_compiler_flags("-fstack-protector-strong")
else()
    add_gcc_compiler_flags("-fstack-protector --param=ssp-buffer-size=4")
endif()

add_gcc_compiler_cxxflags("-Wnon-virtual-dtor -Wold-style-cast -Woverloaded-virtual")
add_gcc_compiler_cflags("-Wchar-subscripts -Wwrite-strings")

if(WITH_ASAN)
    if(NOT (CMAKE_SYSTEM_NAME STREQUAL "Linux" OR APPLE))
        message(FATAL_ERROR "WITH_ASAN is only supported on Linux / macOS at the moment.")
    endif()

    add_gcc_compiler_flags("-fsanitize=address -DWITH_ASAN")

    if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
        if(NOT (CMAKE_COMPILER_IS_GNUCXX AND CMAKE_CXX_COMPILER_VERSION VERSION_LESS 4.9))
            add_gcc_compiler_flags("-fsanitize=leak -DWITH_LSAN")
        endif()
    endif()

endif()

if(CMAKE_BUILD_TYPE_LOWER MATCHES "(release|relwithdebinfo|minsizerel)")
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

set(CMAKE_C_STANDARD 99)
if(WITH_XC_BOTAN3)
    set(CMAKE_CXX_STANDARD 20)
else()
    set(CMAKE_CXX_STANDARD 17)
endif()
set(CMAKE_CXX_STANDARD_REQUIRED ON)

check_cxx_compiler_flag("-fsized-deallocation" CXX_HAS_fsized_deallocation)
if(CXX_HAS_fsized_deallocation)
    # Do additional check: the deallocation functions must be there too.
    set(CMAKE_REQUIRED_FLAGS "-fsized-deallocation")
    check_cxx_source_compiles("#include <new>
        int main() { void * ptr = nullptr; std::size_t size = 1; ::operator delete(ptr, size); }"
        HAVE_DEALLOCATION_FUNCTIONS)
    if(HAVE_DEALLOCATION_FUNCTIONS)
        check_add_gcc_compiler_flag("-fsized-deallocation" CXX)
    endif()
    unset(CMAKE_REQUIRED_FLAGS)
endif()

if(WITH_DEV_BUILD)
    add_definitions(-DQT_DEPRECATED_WARNINGS)
else()
    add_definitions(-DQT_NO_DEPRECATED_WARNINGS)
    add_gcc_compiler_cxxflags("-Wno-deprecated-declarations")
endif()

set(PROGNAME keepassxc)

include(GNUInstallDirs)

set(CLI_INSTALL_DIR "${CMAKE_INSTALL_BINDIR}")
set(PROXY_INSTALL_DIR "${CMAKE_INSTALL_BINDIR}")
set(BIN_INSTALL_DIR "${CMAKE_INSTALL_BINDIR}")
set(PLUGIN_INSTALL_DIR "${CMAKE_INSTALL_LIBDIR}/keepassxc")
set(DATA_INSTALL_DIR "${CMAKE_INSTALL_DATADIR}/keepassxc")

if(WITH_TESTS)
    enable_testing()
endif(WITH_TESTS)

if(WITH_COVERAGE)
    # Include code coverage, use with -DCMAKE_BUILD_TYPE=Debug
    include(CodeCoverage)
    append_coverage_compiler_flags()

    set(COVERAGE_EXCLUDES
            "'^(.+/)?thirdparty/.*'"
            "'^(.+/)?main\\.cpp$$'"
            "'^(.+/)?cli/keepassxc-cli\\.cpp$$'"
            "'^(.+/)?proxy/keepassxc-proxy\\.cpp$$'")
    if(WITH_COVERAGE AND CMAKE_COMPILER_IS_CLANGXX)
        set(MAIN_BINARIES
                "$<TARGET_FILE:${PROGNAME}>"
                "$<TARGET_FILE:keepassxc-cli>"
                "$<TARGET_FILE:keepassxc-proxy>")
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

set(QT_COMPONENTS Core Network Concurrent Gui Svg Widgets Test LinguistTools)
if(WITH_XC_X11)
    list(APPEND QT_COMPONENTS X11Extras)
endif()
find_package(Qt5 COMPONENTS ${QT_COMPONENTS} DBus REQUIRED)

if(Qt5Core_VERSION VERSION_LESS "5.12.0")
    message(FATAL_ERROR "Qt version 5.12.0 or higher is required")
endif()

# Process moc automatically
set(CMAKE_AUTOMOC ON)
# Process .qrc files automatically
set(CMAKE_AUTORCC ON)

# Debian sets the build type to None for package builds.
# Make sure we don't enable asserts there.
set_property(DIRECTORY APPEND PROPERTY COMPILE_DEFINITIONS_NONE QT_NO_DEBUG)

# Find Argon2 -- Botan 2.18 and below does not support threaded Argon2
find_library(ARGON2_LIBRARIES NAMES argon2)
find_path(ARGON2_INCLUDE_DIR NAMES argon2.h PATH_SUFFIXES local/include)
include_directories(SYSTEM ${ARGON2_INCLUDE_DIR})

# Find zlib
find_package(ZLIB REQUIRED)
if(ZLIB_VERSION_STRING VERSION_LESS "1.2.0")
    message(FATAL_ERROR "zlib 1.2.0 or higher is required to use the gzip format")
endif()
include_directories(SYSTEM ${ZLIB_INCLUDE_DIR})

# Find Minizip
find_package(Minizip REQUIRED)

check_cxx_source_compiles("#include <sys/prctl.h>
int main() { prctl(PR_SET_DUMPABLE, 0); return 0; }"
        HAVE_PR_SET_DUMPABLE)

check_cxx_source_compiles("#include <malloc.h>
int main() { return 0; }"
        HAVE_MALLOC_H)

check_cxx_source_compiles("#include <malloc.h>
int main() { malloc_usable_size(NULL); return 0; }"
        HAVE_MALLOC_USABLE_SIZE)

check_cxx_source_compiles("#include <sys/resource.h>
int main() {
  struct rlimit limit;
  limit.rlim_cur = 0;
  limit.rlim_max = 0;
  setrlimit(RLIMIT_CORE, &limit);
  return 0;
}" HAVE_RLIMIT_CORE)

include_directories(SYSTEM ${ZLIB_INCLUDE_DIR})

add_subdirectory(src)
add_subdirectory(share)
if(WITH_TESTS)
    add_subdirectory(tests)
endif(WITH_TESTS)

# This will only print ENABLED and DISABLED feature
feature_summary(WHAT ENABLED_FEATURES DESCRIPTION "Enabled features:")
feature_summary(WHAT DISABLED_FEATURES DESCRIPTION "Disabled features:")
