#  Copyright (C) 2019 KeePassXC Team
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

set(cli_SOURCES
        Add.cpp
        Add.h
        AddGroup.cpp
        AddGroup.h
        Analyze.cpp
        Analyze.h
        AttachmentExport.cpp
        AttachmentExport.h
        AttachmentImport.cpp
        AttachmentImport.h
        AttachmentRemove.cpp
        AttachmentRemove.h
        Clip.cpp
        Clip.h
        Close.cpp
        Close.h
        Command.cpp
        Command.h
        DatabaseCommand.cpp
        DatabaseCommand.h
        DatabaseCreate.cpp
        DatabaseCreate.h
        DatabaseEdit.cpp
        DatabaseEdit.h
        DatabaseInfo.cpp
        DatabaseInfo.h
        Diceware.cpp
        Diceware.h
        Edit.cpp
        Edit.h
        Estimate.cpp
        Estimate.h
        Exit.cpp
        Exit.h
        Export.cpp
        Export.h
        Generate.cpp
        Generate.h
        Help.cpp
        Help.h
        Import.cpp
        Import.h
        List.cpp
        List.h
        Merge.cpp
        Merge.h
        Move.cpp
        Move.h
        Open.cpp
        Open.h
        Remove.cpp
        Remove.h
        RemoveGroup.cpp
        RemoveGroup.h
        Search.cpp
        Search.h
        Show.cpp
        Show.h
        )

add_library(cli STATIC ${cli_SOURCES})
target_include_directories(cli SYSTEM PRIVATE ${ZXCVBN_INCLUDE_DIR})
target_link_libraries(cli ${ZXCVBN_LIBRARY} Qt5::Core)

find_package(Readline)

if (READLINE_FOUND)
    target_compile_definitions(cli PUBLIC USE_READLINE)
    target_link_libraries(cli ${Readline_LIBRARY})
endif()

add_executable(keepassxc-cli keepassxc-cli.cpp)
target_link_libraries(keepassxc-cli
        ${GPGERROR_LIBRARIES}
        cli
        keepassx_core)

install(TARGETS keepassxc-cli
        BUNDLE DESTINATION . COMPONENT Runtime
        RUNTIME DESTINATION ${CLI_INSTALL_DIR} COMPONENT Runtime)

if(WIN32)
    target_sources(keepassxc-cli
        PRIVATE keepassxc-cli.exe.manifest)

#    install(CODE "include(BundleUtilities)
#                  fixup_bundle(\"\${CMAKE_INSTALL_PREFIX}/keepassxc-cli.exe\" \"\" \"\")"
#            COMPONENT Runtime)
endif()

if(APPLE AND WITH_APP_BUNDLE)
    set(CLI_APP_DIR "${CMAKE_BINARY_DIR}/src/${CLI_INSTALL_DIR}")

    add_custom_command(TARGET keepassxc-cli
            POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_BINARY_DIR}/keepassxc-cli ${CLI_APP_DIR}/keepassxc-cli
            WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/src
            COMMENT "Copying keepassxc-cli into app bundle")
    set_property(GLOBAL APPEND PROPERTY
        _MACDEPLOYQT_EXTRA_BINARIES "${CLI_INSTALL_DIR}/keepassxc-cli")
endif()
