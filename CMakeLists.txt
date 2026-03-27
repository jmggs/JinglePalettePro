cmake_minimum_required(VERSION 3.16)
project(JinglePalette VERSION 4.4.5 LANGUAGES CXX)

# Forçar /Zc:__cplusplus e /permissive- no MSVC
if (MSVC)
    add_compile_options(/Zc:__cplusplus /permissive-)
endif()

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
)

set(RESOURCES
    resources/resources.qrc
)

add_executable(JinglePalette
    ${SOURCES}
    ${HEADERS}
    ${RESOURCES}
)

target_link_libraries(JinglePalette PRIVATE
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
    set_target_properties(JinglePalette PROPERTIES WIN32_EXECUTABLE TRUE)
endif()
