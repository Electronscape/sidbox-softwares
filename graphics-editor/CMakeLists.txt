cmake_minimum_required(VERSION 3.19)
project(graphics-editor LANGUAGES CXX)

find_package(Qt6 6.5 REQUIRED COMPONENTS Core Widgets)

qt_standard_project_setup()

qt_add_executable(graphics-editor
    WIN32 MACOSX_BUNDLE
    font.cpp
    main.cpp
    mainwindow.cpp
    mainwindow.h
    mainwindow.ui
    resources.qrc
    fonteditor.h
    fonteditor.cpp
)


qt_add_resources(RESOURCES_RCC resources.qrc)


target_link_libraries(graphics-editor
    PRIVATE
        Qt::Core
        Qt::Widgets
        ${RESOURCES_RCC}
)

include(GNUInstallDirs)

install(TARGETS graphics-editor
    BUNDLE  DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

qt_generate_deploy_app_script(
    TARGET graphics-editor
    OUTPUT_SCRIPT deploy_script
    NO_UNSUPPORTED_PLATFORM_ERROR
)
install(SCRIPT ${deploy_script})
