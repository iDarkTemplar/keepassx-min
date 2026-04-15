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

include_directories(${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR})

set(keepassx_SOURCES
        core/Alloc.cpp
        core/AsyncTask.h
        core/AutoTypeAssociations.cpp
        core/AutoTypeAssociations.h
        core/Base32.cpp
        core/Base32.h
        core/Bootstrap.cpp
        core/Bootstrap.h
        core/Clock.cpp
        core/Clock.h
        core/Compare.h
        core/Config.cpp
        core/Config.h
        core/CustomData.cpp
        core/CustomData.h
        core/Database.cpp
        core/Database.h
        core/DatabaseStats.cpp
        core/DatabaseStats.h
        core/Endian.h
        core/Entry.cpp
        core/Entry.h
        core/EntryAttachments.cpp
        core/EntryAttachments.h
        core/EntryAttributes.cpp
        core/EntryAttributes.h
        core/EntrySearcher.cpp
        core/EntrySearcher.h
        core/Exporter.h
        core/FileWatcher.cpp
        core/FileWatcher.h
        core/Global.h
        core/Group.cpp
        core/Group.h
        core/HibpOffline.cpp
        core/HibpOffline.h
        core/InactivityTimer.cpp
        core/InactivityTimer.h
        core/ListDeleter.h
        core/Merger.cpp
        core/Merger.h
        core/Metadata.cpp
        core/Metadata.h
        core/ModifiableObject.cpp
        core/ModifiableObject.h
        core/PasswordGenerator.cpp
        core/PasswordGenerator.h
        core/PasswordHealth.cpp
        core/PasswordHealth.h
        core/PassphraseGenerator.cpp
        core/PassphraseGenerator.h
        core/Resources.cpp
        core/Resources.h
        core/SignalMultiplexer.cpp
        core/SignalMultiplexer.h
        core/TimeDelta.cpp
        core/TimeDelta.h
        core/TimeInfo.cpp
        core/TimeInfo.h
        core/Tools.cpp
        core/Tools.h
        core/Totp.cpp
        core/Totp.h
        core/Translator.cpp
        core/Translator.h
        core/UrlTools.cpp
        core/UrlTools.h
        cli/Utils.cpp
        cli/Utils.h
        cli/TextStream.cpp
        cli/TextStream.h
        crypto/Crypto.cpp
        crypto/Crypto.h
        crypto/CryptoHash.cpp
        crypto/CryptoHash.h
        crypto/Random.cpp
        crypto/Random.h
        crypto/SymmetricCipher.cpp
        crypto/SymmetricCipher.h
        crypto/kdf/Kdf.cpp
        crypto/kdf/Kdf.h
        crypto/kdf/AesKdf.cpp
        crypto/kdf/AesKdf.h
        crypto/kdf/Argon2Kdf.cpp
        crypto/kdf/Argon2Kdf.h
        format/BitwardenReader.cpp
        format/BitwardenReader.h
        format/CsvExporter.cpp
        format/CsvExporter.h
        format/CsvParser.cpp
        format/CsvParser.h
        format/HtmlExporter.cpp
        format/HtmlExporter.h
        format/KeePass1.h
        format/KeePass1Reader.cpp
        format/KeePass1Reader.h
        format/KeePass2.cpp
        format/KeePass2.h
        format/KeePass2RandomStream.cpp
        format/KeePass2RandomStream.h
        format/KdbxReader.cpp
        format/KdbxReader.h
        format/KdbxWriter.cpp
        format/KdbxWriter.h
        format/KdbxXmlReader.cpp
        format/KdbxXmlReader.h
        format/KeePass2Reader.cpp
        format/KeePass2Reader.h
        format/KeePass2Writer.cpp
        format/KeePass2Writer.h
        format/Kdbx3Reader.cpp
        format/Kdbx3Reader.h
        format/Kdbx3Writer.cpp
        format/Kdbx3Writer.h
        format/Kdbx4Reader.cpp
        format/Kdbx4Reader.h
        format/Kdbx4Writer.cpp
        format/Kdbx4Writer.h
        format/KdbxXmlWriter.cpp
        format/KdbxXmlWriter.h
        format/OpData01.cpp
        format/OpData01.h
        format/OPUXReader.cpp
        format/OPUXReader.h
        format/OpVaultReader.cpp
        format/OpVaultReader.h
        format/OpVaultReaderAttachments.cpp
        format/OpVaultReaderBandEntry.cpp
        format/OpVaultReaderSections.cpp
        format/ProtonPassReader.cpp
        format/ProtonPassReader.h
        gui/styles/styles.qrc
        gui/styles/StateColorPalette.cpp
        gui/styles/StateColorPalette.h
        gui/styles/base/phantomcolor.cpp
        gui/styles/base/phantomcolor.h
        gui/styles/base/BaseStyle.cpp
        gui/styles/base/BaseStyle.h
        gui/styles/dark/DarkStyle.cpp
        gui/styles/dark/DarkStyle.h
        gui/styles/light/LightStyle.cpp
        gui/styles/light/LightStyle.h
        gui/AboutDialog.cpp
        gui/AboutDialog.h
        gui/Application.cpp
        gui/Application.h
        gui/CategoryListWidget.cpp
        gui/CategoryListWidget.h
        gui/Clipboard.cpp
        gui/Clipboard.h
        gui/CloneDialog.cpp
        gui/CloneDialog.h
        gui/DatabaseIcons.cpp
        gui/DatabaseIcons.h
        gui/DatabaseOpenWidget.cpp
        gui/DatabaseOpenWidget.h
        gui/DatabaseTabWidget.cpp
        gui/DatabaseTabWidget.h
        gui/DatabaseWidget.cpp
        gui/DatabaseWidget.h
        gui/DatabaseWidgetStateSync.cpp
        gui/DatabaseWidgetStateSync.h
        gui/EntryPreviewWidget.cpp
        gui/EntryPreviewWidget.h
        gui/DialogyWidget.cpp
        gui/DialogyWidget.h
        gui/EditWidget.cpp
        gui/EditWidget.h
        gui/EditWidgetIcons.cpp
        gui/EditWidgetIcons.h
        gui/EditWidgetProperties.cpp
        gui/EditWidgetProperties.h
        gui/FileDialog.cpp
        gui/FileDialog.h
        gui/Font.cpp
        gui/Font.h
        gui/GuiTools.cpp
        gui/GuiTools.h
        gui/HtmlGuiExporter.cpp
        gui/HtmlGuiExporter.h
        gui/IconModels.cpp
        gui/IconModels.h
        gui/KMessageWidget.cpp
        gui/KMessageWidget.h
        gui/MainWindow.cpp
        gui/MainWindow.h
        gui/MergeDialog.cpp
        gui/MergeDialog.h
        gui/MessageBox.cpp
        gui/MessageBox.h
        gui/MessageWidget.cpp
        gui/MessageWidget.h
        gui/PasswordWidget.cpp
        gui/PasswordWidget.h
        gui/PasswordGeneratorWidget.cpp
        gui/PasswordGeneratorWidget.h
        gui/ApplicationSettingsWidget.cpp
        gui/ApplicationSettingsWidget.h
        gui/Icons.cpp
        gui/Icons.h
        gui/SearchWidget.cpp
        gui/SearchWidget.h
        gui/SettingsWidget.cpp
        gui/SettingsWidget.h
        gui/SortFilterHideProxyModel.cpp
        gui/SortFilterHideProxyModel.h
        gui/SquareSvgWidget.cpp
        gui/SquareSvgWidget.h
        gui/TotpSetupDialog.cpp
        gui/TotpSetupDialog.h
        gui/TotpDialog.cpp
        gui/TotpDialog.h
        gui/TotpExportSettingsDialog.cpp
        gui/TotpExportSettingsDialog.h
        gui/DatabaseOpenDialog.cpp
        gui/DatabaseOpenDialog.h
        gui/URLEdit.cpp
        gui/URLEdit.h
        gui/WelcomeWidget.cpp
        gui/WelcomeWidget.h
        gui/csvImport/CsvImportWidget.cpp
        gui/csvImport/CsvImportWidget.h
        gui/csvImport/CsvParserModel.cpp
        gui/csvImport/CsvParserModel.h
        gui/entry/AutoTypeAssociationsModel.cpp
        gui/entry/AutoTypeAssociationsModel.h
        gui/entry/EditEntryWidget.cpp
        gui/entry/EditEntryWidget.h
        gui/entry/EntryAttachmentsModel.cpp
        gui/entry/EntryAttachmentsModel.h
        gui/entry/EntryAttachmentsWidget.cpp
        gui/entry/EntryAttachmentsWidget.h
        gui/entry/EntryAttributesModel.cpp
        gui/entry/EntryAttributesModel.h
        gui/entry/EditEntryAttachmentsDialog.cpp
        gui/entry/EditEntryAttachmentsDialog.h
        gui/entry/PreviewEntryAttachmentsDialog.cpp
        gui/entry/PreviewEntryAttachmentsDialog.h
        gui/entry/attachments/TextAttachmentsWidget.cpp
        gui/entry/attachments/TextAttachmentsWidget.h
        gui/entry/attachments/ImageAttachmentsWidget.cpp
        gui/entry/attachments/ImageAttachmentsWidget.h
        gui/entry/attachments/ImageAttachmentsView.cpp
        gui/entry/attachments/ImageAttachmentsView.h
        gui/entry/attachments/TextAttachmentsPreviewWidget.cpp
        gui/entry/attachments/TextAttachmentsPreviewWidget.h
        gui/entry/attachments/TextAttachmentsEditWidget.cpp
        gui/entry/attachments/TextAttachmentsEditWidget.h
        gui/entry/attachments/AttachmentWidget.cpp
        gui/entry/attachments/AttachmentWidget.h
        gui/entry/EntryHistoryModel.cpp
        gui/entry/EntryHistoryModel.h
        gui/entry/EntryModel.cpp
        gui/entry/EntryModel.h
        gui/entry/EntryView.cpp
        gui/entry/EntryView.h
        gui/export/ExportDialog.cpp
        gui/export/ExportDialog.h
        gui/group/EditGroupWidget.cpp
        gui/group/EditGroupWidget.h
        gui/group/GroupModel.cpp
        gui/group/GroupModel.h
        gui/group/GroupView.cpp
        gui/group/GroupView.h
        gui/tag/TagModel.cpp
        gui/tag/TagModel.h
        gui/tag/TagView.cpp
        gui/tag/TagView.h
        gui/tag/TagsEdit.cpp
        gui/tag/TagsEdit.h
        gui/databasekey/KeyComponentWidget.cpp
        gui/databasekey/KeyComponentWidget.h
        gui/databasekey/PasswordEditWidget.cpp
        gui/databasekey/PasswordEditWidget.h
        gui/databasekey/YubiKeyEditWidget.cpp
        gui/databasekey/YubiKeyEditWidget.h
        gui/databasekey/KeyFileEditWidget.cpp
        gui/databasekey/KeyFileEditWidget.h
        gui/dbsettings/DatabaseSettingsWidget.cpp
        gui/dbsettings/DatabaseSettingsWidget.h
        gui/dbsettings/DatabaseSettingsDialog.cpp
        gui/dbsettings/DatabaseSettingsDialog.h
        gui/dbsettings/DatabaseSettingsWidgetGeneral.cpp
        gui/dbsettings/DatabaseSettingsWidgetGeneral.h
        gui/dbsettings/DatabaseSettingsWidgetMaintenance.cpp
        gui/dbsettings/DatabaseSettingsWidgetMaintenance.h
        gui/dbsettings/DatabaseSettingsWidgetMetaDataSimple.cpp
        gui/dbsettings/DatabaseSettingsWidgetMetaDataSimple.h
        gui/dbsettings/DatabaseSettingsWidgetEncryption.cpp
        gui/dbsettings/DatabaseSettingsWidgetEncryption.h
        gui/dbsettings/DatabaseSettingsWidgetDatabaseKey.cpp
        gui/dbsettings/DatabaseSettingsWidgetDatabaseKey.h
        gui/reports/ReportsWidget.cpp
        gui/reports/ReportsWidget.h
        gui/reports/ReportsDialog.cpp
        gui/reports/ReportsDialog.h
        gui/reports/ReportsWidgetHealthcheck.cpp
        gui/reports/ReportsWidgetHealthcheck.h
        gui/reports/ReportsPageHealthcheck.cpp
        gui/reports/ReportsPageHealthcheck.h
        gui/reports/ReportsWidgetHibp.cpp
        gui/reports/ReportsWidgetHibp.h
        gui/reports/ReportsPageHibp.cpp
        gui/reports/ReportsPageHibp.h
        gui/reports/ReportsWidgetStatistics.cpp
        gui/reports/ReportsWidgetStatistics.h
        gui/reports/ReportsPageStatistics.cpp
        gui/reports/ReportsPageStatistics.h
        gui/osutils/OSUtilsBase.cpp
        gui/osutils/OSUtilsBase.h
        gui/osutils/ScreenLockListener.cpp
        gui/osutils/ScreenLockListener.h
        gui/osutils/ScreenLockListenerPrivate.cpp
        gui/widgets/ElidedLabel.cpp
        gui/widgets/ElidedLabel.h
        gui/widgets/KPToolBar.cpp
        gui/widgets/KPToolBar.h
        gui/widgets/PopupHelpWidget.cpp
        gui/widgets/PopupHelpWidget.h
        gui/wizard/ImportWizard.cpp
        gui/wizard/ImportWizard.h
        gui/wizard/ImportWizardPageReview.cpp
        gui/wizard/ImportWizardPageReview.h
        gui/wizard/ImportWizardPageSelect.cpp
        gui/wizard/ImportWizardPageSelect.h
        gui/wizard/NewDatabaseWizard.cpp
        gui/wizard/NewDatabaseWizard.h
        gui/wizard/NewDatabaseWizardPage.cpp
        gui/wizard/NewDatabaseWizardPage.h
        gui/wizard/NewDatabaseWizardPageMetaData.cpp
        gui/wizard/NewDatabaseWizardPageMetaData.h
        gui/wizard/NewDatabaseWizardPageEncryption.cpp
        gui/wizard/NewDatabaseWizardPageEncryption.h
        gui/wizard/NewDatabaseWizardPageDatabaseKey.cpp
        gui/wizard/NewDatabaseWizardPageDatabaseKey.h
        keys/CompositeKey.cpp
        keys/CompositeKey.h
        keys/FileKey.cpp
        keys/FileKey.h
        keys/PasswordKey.cpp
        keys/PasswordKey.h
        keys/ChallengeResponseKey.cpp
        keys/ChallengeResponseKey.h
        streams/HashedBlockStream.cpp
        streams/HashedBlockStream.h
        streams/HashingStream.cpp
        streams/HashingStream.h
        streams/HmacBlockStream.cpp
        streams/HmacBlockStream.h
        streams/LayeredStream.cpp
        streams/LayeredStream.h
        streams/qtiocompressor.cpp
        streams/qtiocompressor.h
        streams/StoreDataStream.cpp
        streams/StoreDataStream.h
        streams/SymmetricCipherStream.cpp
        streams/SymmetricCipherStream.h
        )

set(keepassx_SOURCES
            ${keepassx_SOURCES}
            gui/osutils/nixutils/ScreenLockListenerDBus.cpp
            gui/osutils/nixutils/ScreenLockListenerDBus.h
            gui/osutils/nixutils/NixUtils.cpp
            gui/osutils/nixutils/NixUtils.h
            )
    if(WITH_XC_X11)
        list(APPEND keepassx_SOURCES
            gui/osutils/nixutils/X11Funcs.cpp
            gui/osutils/nixutils/X11Funcs.h
            )
    endif()
qt5_add_dbus_adaptor(keepassx_SOURCES
        gui/org.keepassxc.KeePassXC.MainWindow.xml
        gui/MainWindow.h
        MainWindow)

set(keepassx_UI
	gui/AboutDialog.ui
	gui/ApplicationSettingsWidgetGeneral.ui
	gui/ApplicationSettingsWidgetSecurity.ui
	gui/CategoryListWidget.ui
	gui/CloneDialog.ui
	gui/DatabaseOpenWidget.ui
	gui/EditWidget.ui
	gui/EditWidgetIcons.ui
	gui/EditWidgetProperties.ui
	gui/EntryPreviewWidget.ui
	gui/IconDownloaderDialog.ui
	gui/MainWindow.ui
	gui/MergeDialog.ui
	gui/PasswordGeneratorWidget.ui
	gui/PasswordWidget.ui
	gui/SearchHelpWidget.ui
	gui/SearchWidget.ui
	gui/TotpDialog.ui
	gui/TotpSetupDialog.ui
	gui/UpdateCheckDialog.ui
	gui/WelcomeWidget.ui
	gui/csvImport/CsvImportWidget.ui
	gui/databasekey/KeyComponentWidget.ui
	gui/databasekey/KeyFileEditWidget.ui
	gui/databasekey/PasswordEditWidget.ui
	gui/databasekey/YubiKeyEditWidget.ui
	gui/dbsettings/DatabaseSettingsWidgetBrowser.ui
	gui/dbsettings/DatabaseSettingsWidgetEncryption.ui
	gui/dbsettings/DatabaseSettingsWidgetGeneral.ui
	gui/dbsettings/DatabaseSettingsWidgetMaintenance.ui
	gui/dbsettings/DatabaseSettingsWidgetMetaDataSimple.ui
	gui/entry/EditEntryAttachmentsDialog.ui
	gui/entry/EditEntryWidgetAdvanced.ui
	gui/entry/EditEntryWidgetAutoType.ui
	gui/entry/EditEntryWidgetBrowser.ui
	gui/entry/EditEntryWidgetHistory.ui
	gui/entry/EditEntryWidgetMain.ui
	gui/entry/EditEntryWidgetSSHAgent.ui
	gui/entry/EntryAttachmentsWidget.ui
	gui/entry/PreviewEntryAttachmentsDialog.ui
	gui/entry/attachments/ImageAttachmentsWidget.ui
	gui/entry/attachments/TextAttachmentsEditWidget.ui
	gui/entry/attachments/TextAttachmentsPreviewWidget.ui
	gui/export/ExportDialog.ui
	gui/group/EditGroupWidgetBrowser.ui
	gui/group/EditGroupWidgetMain.ui
	gui/passkeys/PasskeyExportDialog.ui
	gui/passkeys/PasskeyImportDialog.ui
	gui/reports/ReportsDialog.ui
	gui/reports/ReportsWidgetBrowserStatistics.ui
	gui/reports/ReportsWidgetHealthcheck.ui
	gui/reports/ReportsWidgetHibp.ui
	gui/reports/ReportsWidgetPasskeys.ui
	gui/reports/ReportsWidgetStatistics.ui
	gui/wizard/ImportWizardPageReview.ui
	gui/wizard/ImportWizardPageSelect.ui
	gui/wizard/NewDatabaseWizardPage.ui
	)

if(WITH_XC_YUBIKEY)
    set(keepassx_SOURCES ${keepassx_SOURCES} gui/osutils/DeviceListener.cpp)
    if(APPLE)
        set(keepassx_SOURCES ${keepassx_SOURCES} gui/osutils/macutils/DeviceListenerMac.cpp)
    elseif(UNIX)
        set(keepassx_SOURCES ${keepassx_SOURCES} gui/osutils/nixutils/DeviceListenerLibUsb.cpp)
    elseif(WIN32)
        set(keepassx_SOURCES ${keepassx_SOURCES} gui/osutils/winutils/DeviceListenerWin.cpp)
    endif()
endif()

set(keepassx_SOURCES ${keepassx_SOURCES}
        ../share/icons/icons.qrc
        ../share/wizard/wizard.qrc)

set(keepassx_SOURCES_MAINEXE main.cpp)

add_feature_info(FdoSecrets WITH_XC_FDOSECRETS "Implement freedesktop.org Secret Storage Spec server side API.")

add_subdirectory(browser)
add_subdirectory(proxy)
if(WITH_XC_BROWSER)
    set(keepassxcbrowser_LIB keepassxcbrowser)
    set(keepassx_SOURCES ${keepassx_SOURCES}
            gui/dbsettings/DatabaseSettingsWidgetBrowser.cpp
            gui/entry/EntryURLModel.cpp
            gui/reports/ReportsWidgetBrowserStatistics.cpp
            gui/reports/ReportsPageBrowserStatistics.cpp)
endif()

if(WITH_XC_BROWSER_PASSKEYS)
    set(keepassx_SOURCES ${keepassx_SOURCES}
            gui/reports/ReportsWidgetPasskeys.cpp
            gui/reports/ReportsPagePasskeys.cpp
            gui/passkeys/PasskeyExporter.cpp
            gui/passkeys/PasskeyExportDialog.cpp
            gui/passkeys/PasskeyImporter.cpp
            gui/passkeys/PasskeyImportDialog.cpp)
endif()

add_subdirectory(autotype)
add_subdirectory(cli)
add_subdirectory(qrcode)
set(qrcode_LIB qrcode)

add_subdirectory(fdosecrets)
if(WITH_XC_FDOSECRETS)
    set(fdosecrets_LIB fdosecrets)
endif()

set(autotype_UI
	autotype/AutoTypeSelectDialog.ui
	autotype/PickcharsDialog.ui
	)

set(autotype_SOURCES
        core/Tools.cpp
        autotype/AutoType.cpp
        autotype/AutoTypeAction.cpp
        autotype/AutoTypeMatchModel.cpp
        autotype/AutoTypeMatchView.cpp
        autotype/AutoTypeSelectDialog.cpp
        autotype/PickcharsDialog.cpp
        autotype/ShortcutWidget.cpp
        autotype/WindowSelectComboBox.cpp)

if(WITH_XC_YUBIKEY)
    list(APPEND keepassx_SOURCES
        keys/drivers/YubiKey.h
        keys/drivers/YubiKey.cpp
        keys/drivers/YubiKeyInterface.cpp
        keys/drivers/YubiKeyInterfaceUSB.cpp
        keys/drivers/YubiKeyInterfacePCSC.cpp)
else()
    list(APPEND keepassx_SOURCES
        keys/drivers/YubiKey.h
        keys/drivers/YubiKeyStub.cpp)
endif()

if(WITH_XC_NETWORKING)
    list(APPEND keepassx_SOURCES
            core/HibpDownloader.cpp
            core/NetworkManager.cpp
            gui/UpdateCheckDialog.cpp
            gui/IconDownloader.cpp
            gui/IconDownloaderDialog.cpp
            updatecheck/UpdateChecker.cpp)
endif()

configure_file(config-keepassx.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/config-keepassx.h)
configure_file(git-info.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/git-info.h)

qt_wrap_ui(keepassx_UI_CPP ${keepassx_UI})
qt_wrap_ui(autotype_UI_CPP ${autotype_UI})

add_library(autotype STATIC ${autotype_SOURCES} ${autotype_UI_CPP})
target_link_libraries(autotype Qt5::Core Qt5::Widgets)

add_library(keepassx_core STATIC ${keepassx_SOURCES} ${keepassx_UI_CPP})

target_include_directories(keepassx_core SYSTEM PRIVATE ${ZXCVBN_INCLUDE_DIR})
set_target_properties(keepassx_core PROPERTIES COMPILE_DEFINITIONS KEEPASSX_BUILDING_CORE)
target_link_libraries(keepassx_core
        autotype
        ${qrcode_LIB}
        ${fdosecrets_LIB}
        Qt5::Core
        Qt5::Concurrent
        Qt5::Network
        Qt5::Widgets
        ${BOTAN_LIBRARIES}
        ${ZXCVBN_LIBRARY}
        ${ZLIB_LIBRARIES}
        ${MINIZIP_LIBRARIES}
        ${ARGON2_LIBRARIES}
	)

target_link_libraries(keepassx_core Qt5::DBus ${LIBUSB_LIBRARIES})
if(WITH_XC_X11)
    target_link_libraries(keepassx_core Qt5::X11Extras X11)
endif()
include_directories(${Qt5Gui_PRIVATE_INCLUDE_DIRS})

# Main Executable Definition
add_executable(${PROGNAME} main.cpp)
target_link_libraries(${PROGNAME} keepassx_core)
set_target_properties(${PROGNAME} PROPERTIES ENABLE_EXPORTS ON)

install(TARGETS ${PROGNAME}
        BUNDLE DESTINATION . COMPONENT Runtime
        RUNTIME DESTINATION ${BIN_INSTALL_DIR} COMPONENT Runtime)
