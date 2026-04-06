cmake_minimum_required(VERSION 3.16)
project(JinglePalettePro VERSION 0.3.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt6 REQUIRED COMPONENTS
    Core
    Gui
    Widgets
    Multimedia
    MultimediaWidgets
    Network
)

set(SOURCES
    src/main.cpp
    src/mainwindow.cpp
    src/settingsdialog.cpp
    src/aboutdialog.cpp
    src/audioengine.cpp
    src/palettemanager.cpp
    src/settingsmanager.cpp
    src/languagemanager.cpp
    src/vumeter.cpp
    src/jinglebutton.cpp
    src/errorlogger.cpp
    src/remotecontrolserver.cpp
)

set(HEADERS
    src/mainwindow.h
    src/settingsdialog.h
    src/aboutdialog.h
    src/audioengine.h
    src/palettemanager.h
    src/settingsmanager.h
    src/languagemanager.h
    src/vumeter.h
    src/jinglebutton.h
    src/errorlogger.h
    src/globals.h
    src/remotecontrolserver.h
)

set(RESOURCES
    resources/resources.qrc
)

set(WIN32_APP_ICON)
if(WIN32)
    set(WIN32_APP_ICON appicon.rc)
endif()

add_executable(JinglePalettePro
    ${SOURCES}
    ${HEADERS}
    ${RESOURCES}
    ${WIN32_APP_ICON}
)

set_target_properties(JinglePalettePro PROPERTIES
    OUTPUT_NAME "Jingle Palette Pro"
)

target_link_libraries(JinglePalettePro PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    Qt6::Multimedia
    Qt6::MultimediaWidgets
    Qt6::Network
)

# Copy wav files to build directory
configure_file(${CMAKE_SOURCE_DIR}/resources/Time_Announce.wav
               ${CMAKE_BINARY_DIR}/Time_Announce.wav COPYONLY)

if(WIN32)
    set_target_properties(JinglePalettePro PROPERTIES WIN32_EXECUTABLE TRUE)
endif()
