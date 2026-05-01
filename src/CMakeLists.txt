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

include_directories(${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR})

set(keepassx_SOURCES
	core/Alloc.cpp
	core/Base32.cpp
	core/Bootstrap.cpp
	core/Clock.cpp
	core/Config.cpp
	core/CustomData.cpp
	core/Database.cpp
	core/DatabaseStats.cpp
	core/EntryAttachments.cpp
	core/EntryAttributes.cpp
	core/Entry.cpp
	core/EntrySearcher.cpp
	core/FileWatcher.cpp
	core/Group.cpp
	core/InactivityTimer.cpp
	core/Merger.cpp
	core/Metadata.cpp
	core/ModifiableObject.cpp
	core/PassphraseGenerator.cpp
	core/PasswordGenerator.cpp
	core/PasswordHealth.cpp
	core/Resources.cpp
	core/SignalMultiplexer.cpp
	core/TimeDelta.cpp
	core/TimeInfo.cpp
	core/Tools.cpp
	core/Totp.cpp
	core/Translator.cpp
	crypto/Crypto.cpp
	crypto/CryptoHash.cpp
	crypto/kdf/AesKdf.cpp
	crypto/kdf/Argon2Kdf.cpp
	crypto/kdf/Kdf.cpp
	crypto/Random.cpp
	crypto/SymmetricCipher.cpp
	format/BitwardenReader.cpp
	format/CsvExporter.cpp
	format/CsvParser.cpp
	format/HtmlExporter.cpp
	format/Kdbx3Reader.cpp
	format/Kdbx3Writer.cpp
	format/Kdbx4Reader.cpp
	format/Kdbx4Writer.cpp
	format/KdbxReader.cpp
	format/KdbxWriter.cpp
	format/KdbxXmlReader.cpp
	format/KdbxXmlWriter.cpp
	format/KeePass2.cpp
	format/KeePass2RandomStream.cpp
	format/KeePass2Reader.cpp
	format/KeePass2Writer.cpp
	format/OpData01.cpp
	format/OPUXReader.cpp
	format/OpVaultReaderAttachments.cpp
	format/OpVaultReaderBandEntry.cpp
	format/OpVaultReader.cpp
	format/OpVaultReaderSections.cpp
	format/ProtonPassReader.cpp
	gui/AboutDialog.cpp
	gui/Application.cpp
	gui/ApplicationSettingsWidget.cpp
	gui/CategoryListWidget.cpp
	gui/Clipboard.cpp
	gui/CloneDialog.cpp
	gui/csvImport/CsvImportWidget.cpp
	gui/csvImport/CsvParserModel.cpp
	gui/DatabaseIcons.cpp
	gui/databasekey/KeyComponentWidget.cpp
	gui/databasekey/KeyFileEditWidget.cpp
	gui/databasekey/PasswordEditWidget.cpp
	gui/DatabaseOpenDialog.cpp
	gui/DatabaseOpenWidget.cpp
	gui/DatabaseTabWidget.cpp
	gui/DatabaseWidget.cpp
	gui/DatabaseWidgetStateSync.cpp
	gui/dbsettings/DatabaseSettingsDialog.cpp
	gui/dbsettings/DatabaseSettingsWidget.cpp
	gui/dbsettings/DatabaseSettingsWidgetDatabaseKey.cpp
	gui/dbsettings/DatabaseSettingsWidgetEncryption.cpp
	gui/dbsettings/DatabaseSettingsWidgetGeneral.cpp
	gui/dbsettings/DatabaseSettingsWidgetMaintenance.cpp
	gui/dbsettings/DatabaseSettingsWidgetMetaDataSimple.cpp
	gui/DialogyWidget.cpp
	gui/EditWidget.cpp
	gui/EditWidgetIcons.cpp
	gui/EditWidgetProperties.cpp
	gui/entry/attachments/AttachmentWidget.cpp
	gui/entry/attachments/ImageAttachmentsView.cpp
	gui/entry/attachments/ImageAttachmentsWidget.cpp
	gui/entry/attachments/TextAttachmentsEditWidget.cpp
	gui/entry/attachments/TextAttachmentsPreviewWidget.cpp
	gui/entry/attachments/TextAttachmentsWidget.cpp
	gui/entry/EditEntryAttachmentsDialog.cpp
	gui/entry/EditEntryWidget.cpp
	gui/entry/EntryAttachmentsModel.cpp
	gui/entry/EntryAttachmentsWidget.cpp
	gui/entry/EntryAttributesModel.cpp
	gui/entry/EntryHistoryModel.cpp
	gui/entry/EntryModel.cpp
	gui/entry/EntryView.cpp
	gui/entry/PreviewEntryAttachmentsDialog.cpp
	gui/EntryPreviewWidget.cpp
	gui/export/ExportDialog.cpp
	gui/FileDialog.cpp
	gui/Font.cpp
	gui/group/EditGroupWidget.cpp
	gui/group/GroupModel.cpp
	gui/group/GroupView.cpp
	gui/GuiTools.cpp
	gui/HtmlGuiExporter.cpp
	gui/IconModels.cpp
	gui/Icons.cpp
	gui/KMessageWidget.cpp
	gui/MainWindow.cpp
	gui/MergeDialog.cpp
	gui/MessageBox.cpp
	gui/MessageWidget.cpp
	gui/osutils/ScreenLockListener.cpp
	gui/osutils/ScreenLockListenerPrivate.cpp
	gui/PasswordGeneratorWidget.cpp
	gui/PasswordWidget.cpp
	gui/reports/ReportsDialog.cpp
	gui/reports/ReportsPageHealthcheck.cpp
	gui/reports/ReportsPageStatistics.cpp
	gui/reports/ReportsWidget.cpp
	gui/reports/ReportsWidgetHealthcheck.cpp
	gui/reports/ReportsWidgetStatistics.cpp
	gui/SearchWidget.cpp
	gui/SettingsWidget.cpp
	gui/SortFilterHideProxyModel.cpp
	gui/SquareSvgWidget.cpp
	gui/tag/TagModel.cpp
	gui/tag/TagsEdit.cpp
	gui/tag/TagView.cpp
	gui/TotpDialog.cpp
	gui/TotpExportSettingsDialog.cpp
	gui/TotpSetupDialog.cpp
	gui/URLEdit.cpp
	gui/WelcomeWidget.cpp
	gui/widgets/ElidedLabel.cpp
	gui/widgets/KPToolBar.cpp
	gui/widgets/PopupHelpWidget.cpp
	gui/wizard/ImportWizard.cpp
	gui/wizard/ImportWizardPageReview.cpp
	gui/wizard/ImportWizardPageSelect.cpp
	gui/wizard/NewDatabaseWizard.cpp
	gui/wizard/NewDatabaseWizardPage.cpp
	gui/wizard/NewDatabaseWizardPageDatabaseKey.cpp
	gui/wizard/NewDatabaseWizardPageEncryption.cpp
	gui/wizard/NewDatabaseWizardPageMetaData.cpp
	keys/CompositeKey.cpp
	keys/FileKey.cpp
	keys/PasswordKey.cpp
	streams/HashedBlockStream.cpp
	streams/HashingStream.cpp
	streams/HmacBlockStream.cpp
	streams/LayeredStream.cpp
	streams/qtiocompressor.cpp
	streams/StoreDataStream.cpp
	streams/SymmetricCipherStream.cpp
	)

set(keepassx_HEADERS
	core/AsyncTask.h
	core/Base32.h
	core/Bootstrap.h
	core/Clock.h
	core/Compare.h
	core/DatabaseStats.h
	core/Endian.h
	core/EntrySearcher.h
	core/Exporter.h
	core/Global.h
	core/ListDeleter.h
	core/PassphraseGenerator.h
	core/PasswordGenerator.h
	core/PasswordHealth.h
	core/Resources.h
	core/SignalMultiplexer.h
	core/TimeDelta.h
	core/TimeInfo.h
	core/Tools.h
	core/Totp.h
	core/Translator.h
	crypto/Crypto.h
	crypto/CryptoHash.h
	crypto/kdf/AesKdf.h
	crypto/kdf/Argon2Kdf.h
	crypto/kdf/Kdf.h
	crypto/Random.h
	crypto/SymmetricCipher.h
	format/BitwardenReader.h
	format/CsvExporter.h
	format/CsvParser.h
	format/HtmlExporter.h
	format/Kdbx3Reader.h
	format/Kdbx3Writer.h
	format/Kdbx4Reader.h
	format/Kdbx4Writer.h
	format/KdbxReader.h
	format/KdbxWriter.h
	format/KdbxXmlReader.h
	format/KdbxXmlWriter.h
	format/KeePass2.h
	format/KeePass2RandomStream.h
	format/KeePass2Reader.h
	format/KeePass2Writer.h
	format/OPUXReader.h
	format/ProtonPassReader.h
	gui/ColorPalette.h
	gui/DatabaseIcons.h
	gui/entry/attachments/AttachmentTypes.h
	gui/entry/EditEntryWidget_p.h
	gui/FileDialog.h
	gui/Font.h
	gui/GuiTools.h
	gui/HtmlGuiExporter.h
	gui/Icons.h
	gui/MessageBox.h
	gui/reports/ReportsPageHealthcheck.h
	gui/reports/ReportsPageStatistics.h
	gui/SquareSvgWidget.h
	keys/CompositeKey.h
	keys/FileKey.h
	keys/PasswordKey.h
	)

set(keepassx_QT_HEADERS
	core/Config.h
	core/CustomData.h
	core/Database.h
	core/EntryAttachments.h
	core/EntryAttributes.h
	core/Entry.h
	core/FileWatcher.h
	core/Group.h
	core/InactivityTimer.h
	core/Merger.h
	core/Metadata.h
	core/ModifiableObject.h
	format/OpData01.h
	format/OpVaultReader.h
	gui/AboutDialog.h
	gui/Application.h
	gui/ApplicationSettingsWidget.h
	gui/CategoryListWidget.h
	gui/Clipboard.h
	gui/CloneDialog.h
	gui/csvImport/CsvImportWidget.h
	gui/csvImport/CsvParserModel.h
	gui/databasekey/KeyComponentWidget.h
	gui/databasekey/KeyFileEditWidget.h
	gui/databasekey/PasswordEditWidget.h
	gui/DatabaseOpenDialog.h
	gui/DatabaseOpenWidget.h
	gui/DatabaseTabWidget.h
	gui/DatabaseWidget.h
	gui/DatabaseWidgetStateSync.h
	gui/dbsettings/DatabaseSettingsDialog.h
	gui/dbsettings/DatabaseSettingsWidgetDatabaseKey.h
	gui/dbsettings/DatabaseSettingsWidgetEncryption.h
	gui/dbsettings/DatabaseSettingsWidgetGeneral.h
	gui/dbsettings/DatabaseSettingsWidget.h
	gui/dbsettings/DatabaseSettingsWidgetMaintenance.h
	gui/dbsettings/DatabaseSettingsWidgetMetaDataSimple.h
	gui/DialogyWidget.h
	gui/EditWidget.h
	gui/EditWidgetIcons.h
	gui/EditWidgetProperties.h
	gui/entry/attachments/AttachmentWidget.h
	gui/entry/attachments/ImageAttachmentsView.h
	gui/entry/attachments/ImageAttachmentsWidget.h
	gui/entry/attachments/TextAttachmentsEditWidget.h
	gui/entry/attachments/TextAttachmentsPreviewWidget.h
	gui/entry/attachments/TextAttachmentsWidget.h
	gui/entry/EditEntryAttachmentsDialog.h
	gui/entry/EditEntryWidget.h
	gui/entry/EntryAttachmentsModel.h
	gui/entry/EntryAttachmentsWidget.h
	gui/entry/EntryAttributesModel.h
	gui/entry/EntryHistoryModel.h
	gui/entry/EntryModel.h
	gui/entry/EntryView.h
	gui/entry/PreviewEntryAttachmentsDialog.h
	gui/EntryPreviewWidget.h
	gui/export/ExportDialog.h
	gui/group/EditGroupWidget.h
	gui/group/GroupModel.h
	gui/group/GroupView.h
	gui/IconModels.h
	gui/KMessageWidget.h
	gui/KMessageWidget_p.h
	gui/MainWindow.h
	gui/MergeDialog.h
	gui/MessageWidget.h
	gui/osutils/ScreenLockListener.h
	gui/osutils/ScreenLockListenerPrivate.h
	gui/PasswordGeneratorWidget.h
	gui/PasswordWidget.h
	gui/reports/ReportsDialog.h
	gui/reports/ReportsWidget.h
	gui/reports/ReportsWidgetHealthcheck.h
	gui/reports/ReportsWidgetStatistics.h
	gui/SearchWidget.h
	gui/SettingsWidget.h
	gui/SortFilterHideProxyModel.h
	gui/tag/TagModel.h
	gui/tag/TagsEdit.h
	gui/tag/TagView.h
	gui/TotpDialog.h
	gui/TotpExportSettingsDialog.h
	gui/TotpSetupDialog.h
	gui/URLEdit.h
	gui/WelcomeWidget.h
	gui/widgets/ElidedLabel.h
	gui/widgets/KPToolBar.h
	gui/widgets/PopupHelpWidget.h
	gui/wizard/ImportWizard.h
	gui/wizard/ImportWizardPageReview.h
	gui/wizard/ImportWizardPageSelect.h
	gui/wizard/NewDatabaseWizard.h
	gui/wizard/NewDatabaseWizardPageDatabaseKey.h
	gui/wizard/NewDatabaseWizardPageEncryption.h
	gui/wizard/NewDatabaseWizardPage.h
	gui/wizard/NewDatabaseWizardPageMetaData.h
	streams/HashedBlockStream.h
	streams/HashingStream.h
	streams/HmacBlockStream.h
	streams/LayeredStream.h
	streams/qtiocompressor.h
	streams/StoreDataStream.h
	streams/SymmetricCipherStream.h
	)

list(APPEND keepassx_SOURCES
	gui/osutils/nixutils/ScreenLockListenerDBus.cpp
	)

list(APPEND keepassx_QT_HEADERS
	gui/osutils/nixutils/ScreenLockListenerDBus.h
	)

list(APPEND keepassx_HEADERS ${keepassx_QT_HEADERS})

set(keepassx_UI
	gui/AboutDialog.ui
	gui/ApplicationSettingsWidgetGeneral.ui
	gui/ApplicationSettingsWidgetSecurity.ui
	gui/CategoryListWidget.ui
	gui/CloneDialog.ui
	gui/csvImport/CsvImportWidget.ui
	gui/databasekey/KeyComponentWidget.ui
	gui/databasekey/KeyFileEditWidget.ui
	gui/databasekey/PasswordEditWidget.ui
	gui/DatabaseOpenWidget.ui
	gui/dbsettings/DatabaseSettingsWidgetEncryption.ui
	gui/dbsettings/DatabaseSettingsWidgetGeneral.ui
	gui/dbsettings/DatabaseSettingsWidgetMaintenance.ui
	gui/dbsettings/DatabaseSettingsWidgetMetaDataSimple.ui
	gui/EditWidgetIcons.ui
	gui/EditWidgetProperties.ui
	gui/EditWidget.ui
	gui/entry/attachments/ImageAttachmentsWidget.ui
	gui/entry/attachments/TextAttachmentsEditWidget.ui
	gui/entry/attachments/TextAttachmentsPreviewWidget.ui
	gui/entry/EditEntryAttachmentsDialog.ui
	gui/entry/EditEntryWidgetAdvanced.ui
	gui/entry/EditEntryWidgetHistory.ui
	gui/entry/EditEntryWidgetMain.ui
	gui/entry/EntryAttachmentsWidget.ui
	gui/entry/PreviewEntryAttachmentsDialog.ui
	gui/EntryPreviewWidget.ui
	gui/export/ExportDialog.ui
	gui/group/EditGroupWidgetMain.ui
	gui/MainWindow.ui
	gui/MergeDialog.ui
	gui/PasswordGeneratorWidget.ui
	gui/PasswordWidget.ui
	gui/reports/ReportsDialog.ui
	gui/reports/ReportsWidgetHealthcheck.ui
	gui/reports/ReportsWidgetStatistics.ui
	gui/SearchHelpWidget.ui
	gui/SearchWidget.ui
	gui/TotpDialog.ui
	gui/TotpSetupDialog.ui
	gui/WelcomeWidget.ui
	gui/wizard/ImportWizardPageReview.ui
	gui/wizard/ImportWizardPageSelect.ui
	gui/wizard/NewDatabaseWizardPage.ui
	)

set(keepassx_RESOURCES
	../share/icons/icons.qrc
	)

qt_wrap_cpp(MOC_keepassx_HEADERS ${keepassx_QT_HEADERS})
qt_add_resources(COMPILED_keepassx_RESOURCES ${keepassx_RESOURCES})

set(keepassx_SOURCES_MAINEXE main.cpp)

set(qrcode_SOURCES
	qrcode/QrCode.cpp
	)

set(qrcode_HEADERS
	qrcode/QrCode.h
	qrcode/QrCode_p.h
	)

add_library(qrcode STATIC ${qrcode_SOURCES} ${qrcode_HEADERS})
target_include_directories(qrcode PRIVATE ${QRENCODE_INCLUDE_DIR})
target_link_libraries(qrcode PUBLIC Qt6::Core Qt6::Svg ${QRENCODE_LIBRARY})

set(qrcode_LIB qrcode)

add_feature_info(FdoSecrets WITH_XC_FDOSECRETS "Implement freedesktop.org Secret Storage Spec server side API.")

if (WITH_XC_FDOSECRETS)
	set(fdo_SOURCES
		# app settings page
		fdosecrets/FdoSecretsPlugin.cpp
		fdosecrets/widgets/SettingsModels.cpp
		fdosecrets/widgets/SettingsWidgetFdoSecrets.cpp
		fdosecrets/widgets/RowButtonHelper.cpp

		# per database settings page
		fdosecrets/widgets/DatabaseSettingsWidgetFdoSecrets.cpp

		# prompt dialog
		fdosecrets/widgets/AccessControlDialog.cpp

		# setting storage
		fdosecrets/FdoSecretsSettings.cpp

		# dbus objects
		fdosecrets/dbus/DBusClient.cpp
		fdosecrets/dbus/DBusMgr.cpp
		fdosecrets/dbus/DBusDispatch.cpp
		fdosecrets/dbus/DBusObject.cpp
		fdosecrets/objects/Service.cpp
		fdosecrets/objects/Session.cpp
		fdosecrets/objects/SessionCipher.cpp
		fdosecrets/objects/Collection.cpp
		fdosecrets/objects/Item.cpp
		fdosecrets/objects/Prompt.cpp
		fdosecrets/dbus/DBusTypes.cpp
		)

	set(fdo_HEADERS
		# app settings page
		fdosecrets/widgets/RowButtonHelper.h

		# setting storage
		fdosecrets/FdoSecretsSettings.h

		# dbus objects
		fdosecrets/dbus/DBusConstants.h
		fdosecrets/dbus/DBusClient.h
		fdosecrets/dbus/DBusTypes.h
		fdosecrets/objects/SessionCipher.h
		)

	set(fdo_QT_HEADERS
		# app settings page
		fdosecrets/FdoSecretsPlugin.h
		fdosecrets/widgets/SettingsModels.h
		fdosecrets/widgets/SettingsWidgetFdoSecrets.h
		fdosecrets/widgets/SettingsWidgetFdoSecrets_p.h

		# per database settings page
		fdosecrets/widgets/DatabaseSettingsWidgetFdoSecrets.h
		fdosecrets/widgets/DatabaseSettingsWidgetFdoSecrets_p.h

		# prompt dialog
		fdosecrets/widgets/AccessControlDialog.h

		# dbus objects
		fdosecrets/dbus/DBusMgr.h
		fdosecrets/dbus/DBusObject.h
		fdosecrets/objects/Service.h
		fdosecrets/objects/Session.h
		fdosecrets/objects/Collection.h
		fdosecrets/objects/Item.h
		fdosecrets/objects/Prompt.h
		)

	list(APPEND fdo_HEADERS ${fdo_QT_HEADERS})

	set(fdo_UI
		fdosecrets/widgets/AccessControlDialog.ui
		fdosecrets/widgets/DatabaseSettingsWidgetFdoSecrets.ui
		fdosecrets/widgets/SettingsWidgetFdoSecrets.ui
		)

	qt_wrap_cpp(MOC_fdo_HEADERS ${fdo_QT_HEADERS})
	qt_wrap_ui(fdo_UI_CPP ${fdo_UI})

	add_library(fdosecrets STATIC ${fdo_SOURCES} ${fdo_HEADERS} ${MOC_fdo_HEADERS} ${fdo_UI_CPP})
	target_link_libraries(fdosecrets Qt6::Core Qt6::Widgets Qt6::DBus ${BOTAN_LIBRARIES})

	set(fdosecrets_LIB fdosecrets)
endif()

configure_file(config-keepassx.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/config-keepassx.h)

qt_wrap_ui(keepassx_UI_CPP ${keepassx_UI})

add_library(keepassx_core STATIC ${keepassx_SOURCES} ${keepassx_HEADERS} ${MOC_keepassx_HEADERS} ${COMPILED_keepassx_RESOURCES} ${keepassx_UI_CPP})

target_include_directories(keepassx_core SYSTEM PRIVATE ${ZXCVBN_INCLUDE_DIR})
set_target_properties(keepassx_core PROPERTIES COMPILE_DEFINITIONS KEEPASSX_BUILDING_CORE)
target_link_libraries(keepassx_core
	${qrcode_LIB}
	${fdosecrets_LIB}
	Qt6::Core
	Qt6::Concurrent
	Qt6::Network
	Qt6::Widgets
	Qt6::SvgWidgets
	${BOTAN_LIBRARIES}
	${ZXCVBN_LIBRARY}
	${ZLIB_LIBRARIES}
	${MINIZIP_LIBRARIES}
	${ARGON2_LIBRARIES}
	)

target_link_libraries(keepassx_core Qt6::DBus)
include_directories(${Qt6Gui_PRIVATE_INCLUDE_DIRS})

# Main Executable Definition
add_executable(keepassx-min main.cpp)
target_link_libraries(keepassx-min keepassx_core)
set_target_properties(keepassx-min PROPERTIES ENABLE_EXPORTS ON RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

install(TARGETS keepassx-min
	BUNDLE DESTINATION . COMPONENT Runtime
	RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR} COMPONENT Runtime)
