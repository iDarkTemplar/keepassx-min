if(WITH_XC_FDOSECRETS)
    include_directories(${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR})

	set(fdo_UI
		widgets/AccessControlDialog.ui
		widgets/DatabaseSettingsWidgetFdoSecrets.ui
		widgets/SettingsWidgetFdoSecrets.ui
		)

	qt_wrap_ui(fdo_UI_CPP ${fdo_UI})

    add_library(fdosecrets STATIC
        # app settings page
        FdoSecretsPlugin.cpp
        FdoSecretsPlugin.h
        widgets/SettingsModels.cpp
        widgets/SettingsModels.h
        widgets/SettingsWidgetFdoSecrets.cpp
        widgets/SettingsWidgetFdoSecrets.h
        widgets/RowButtonHelper.cpp
        widgets/RowButtonHelper.h

        # per database settings page
        widgets/DatabaseSettingsWidgetFdoSecrets.cpp
        widgets/DatabaseSettingsWidgetFdoSecrets.h

        # prompt dialog
        widgets/AccessControlDialog.cpp
        widgets/AccessControlDialog.h

        # setting storage
        FdoSecretsSettings.cpp
        FdoSecretsSettings.h

        # dbus objects
        dbus/DBusClient.cpp
        dbus/DBusClient.h
        dbus/DBusMgr.cpp
        dbus/DBusMgr.h
        dbus/DBusDispatch.cpp
        dbus/DBusObject.cpp
        dbus/DBusObject.h
        objects/Service.cpp
        objects/Service.h
        objects/Session.cpp
        objects/Session.h
        objects/SessionCipher.cpp
        objects/SessionCipher.h
        objects/Collection.cpp
        objects/Collection.h
        objects/Item.cpp
        objects/Item.h
        objects/Prompt.cpp
        objects/Prompt.h
        dbus/DBusTypes.cpp
        dbus/DBusTypes.h
		${fdo_UI_CPP}
    )
    target_link_libraries(fdosecrets Qt5::Core Qt5::Widgets Qt5::DBus ${BOTAN_LIBRARIES})
endif()
