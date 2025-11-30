cmake_minimum_required(VERSION 3.16)
project(SidboxTerminal LANGUAGES CXX)

find_package(Qt6 6.5 REQUIRED COMPONENTS Core Widgets)

qt_standard_project_setup()

qt_add_executable(SidboxTerminal
    WIN32 MACOSX_BUNDLE
    main.cpp
    mainwindow.cpp
    mainwindow.h
    mainwindow.ui
    directoryboss.cpp directoryboss.h directoryboss.ui resources.qrc serialhandler.cpp serialhandler.h
    frmftp.h frmftp.cpp frmftp.ui
    kfmparser.h
    kfmparser.cpp






)

find_package(Qt6 REQUIRED COMPONENTS SerialPort)
qt_add_resources(RESOURCES_RCC resources.qrc)


target_link_libraries(SidboxTerminal
    PRIVATE
        Qt::Core
        Qt::Widgets
        Qt6::SerialPort
        ${RESOURCES_RCC}
)

include(GNUInstallDirs)

install(TARGETS SidboxTerminal
    BUNDLE  DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

qt_generate_deploy_app_script(
    TARGET SidboxTerminal
    OUTPUT_SCRIPT deploy_script
    NO_UNSUPPORTED_PLATFORM_ERROR
)
install(SCRIPT ${deploy_script})
