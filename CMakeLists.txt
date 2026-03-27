cmake_minimum_required(VERSION 3.16)
project(JinglePalettePro VERSION 0.1 LANGUAGES CXX)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets Multimedia MultimediaWidgets Network)


# Fontes principais
file(GLOB SOURCES src/*.cpp)
file(GLOB HEADERS src/*.h)
file(GLOB RESOURCES resources/*.qrc)

# Recursos extras para o bundle .app
set(EXTRA_RESOURCES resources/logo.icns)
file(GLOB WAV_FILES resources/*.wav)
list(APPEND EXTRA_RESOURCES ${WAV_FILES})

add_executable(JinglePalettePro
    ${SOURCES}
    ${HEADERS}
    ${RESOURCES}
    ${EXTRA_RESOURCES}
)

target_link_libraries(JinglePalettePro PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    Qt6::Multimedia
    Qt6::MultimediaWidgets
    Qt6::Network
)

# Para builds no Windows, copie DLLs do Qt se necessário
if(WIN32)
    set_target_properties(JinglePalettePro PROPERTIES WIN32_EXECUTABLE TRUE)
endif()

# Para gerar bundle .app no macOS
if(APPLE)
    set_target_properties(JinglePalettePro PROPERTIES
        MACOSX_BUNDLE TRUE
        MACOSX_BUNDLE_BUNDLE_NAME "JinglePalettePro"
        MACOSX_BUNDLE_GUI_IDENTIFIER "com.seuprojeto.jinglepalettepro"
        MACOSX_BUNDLE_BUNDLE_VERSION "${PROJECT_VERSION}"
        MACOSX_BUNDLE_SHORT_VERSION_STRING "${PROJECT_VERSION}"
        MACOSX_BUNDLE_COPYRIGHT "Copyright 2026"
        MACOSX_BUNDLE_ICON_FILE logo.icns
    )
    set_source_files_properties(resources/logo.icns PROPERTIES MACOSX_PACKAGE_LOCATION "Resources")

    # Copiar todos os arquivos .wav para Resources
    file(GLOB WAV_FILES resources/*.wav)
    foreach(wav ${WAV_FILES})
        set_source_files_properties(${wav} PROPERTIES MACOSX_PACKAGE_LOCATION "Resources")
    endforeach()
endif()
