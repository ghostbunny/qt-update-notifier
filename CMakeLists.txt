
cmake_minimum_required(VERSION 2.6)
cmake_policy(SET CMP0017 OLD)

find_package( Qt4 COMPONENTS REQUIRED )

find_package( KDE4 REQUIRED )

add_definitions( -I${KDE4_INCLUDE_DIR} )
add_definitions( -I${Qt4_INCLUDE_DIR} )

INCLUDE( ${QT_USE_FILE} )

INCLUDE_DIRECTORIES( ${CMAKE_BINARY_DIR} )

QT4_WRAP_CPP( MOC check_updates.h qtUpdateNotifier.h )

add_executable( qt-update-notifier check_updates.cpp main.cpp qtUpdateNotifier.cpp ${MOC} )

find_library ( kdeui_lib libkdeui.so )
TARGET_LINK_LIBRARIES( qt-update-notifier ${QT_LIBRARIES} ${kdeui_lib} ) 

set_target_properties( qt-update-notifier PROPERTIES COMPILE_FLAGS "-D_FILE_OFFSET_BITS=64 -Wextra -Wall -s -fPIE -pthread  -pedantic " )
set_target_properties( qt-update-notifier PROPERTIES LINK_FLAGS "-pie" )

install ( FILES qt-update-notifier.png DESTINATION share/icons )
install ( FILES qt-update-notifier-updating.png DESTINATION share/icons )
install ( FILES qt-update-notifier-updates-are-available.png DESTINATION share/icons )

install( TARGETS qt-update-notifier RUNTIME DESTINATION bin )

install ( FILES ${PROJECT_BINARY_DIR}/qt-update-notifier.desktop DESTINATION share/applications 
	PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE 
)

install ( SCRIPT "${CMAKE_SOURCE_DIR}/PostInstall.cmake" )

# uninstall target
configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake_uninstall.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake"
    IMMEDIATE @ONLY )

add_custom_target( uninstall
    COMMAND ${CMAKE_COMMAND} -P ${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake )

# desktop file section
file( WRITE "${PROJECT_BINARY_DIR}/qt-update-notifier.desktop"
"#!/usr/bin/env xdg-open
[Desktop Entry]
Comment[en_US]=
Comment=
Exec=${CMAKE_INSTALL_PREFIX}/bin/qt-update-notifier
GenericName[en_US]=update notifier
GenericName=update notifier
Icon=${CMAKE_INSTALL_PREFIX}/share/icons/qt-update-notifier.png
MimeType=
Name[en_US]=qt-update-notifier
Name=qt-update-notifier
NoDisplay=false
Path=
StartupNotify=true
Terminal=false
TerminalOptions=
Type=Application
X-DBUS-ServiceName=
X-DBUS-StartupType=
X-KDE-SubstituteUID=false
X-KDE-Username=
Categories=Utility;Qt;X-MandrivaLinux-System-FileTools;\n")