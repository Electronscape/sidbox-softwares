cmake_minimum_required(VERSION 3.19)
project(graphics-3d LANGUAGES CXX C)

find_package(Qt6 6.5 REQUIRED COMPONENTS Core Widgets)

qt_standard_project_setup()

qt_add_executable(graphics-3d
    WIN32 MACOSX_BUNDLE
    main.cpp
    dialog.cpp
    dialog.h
    dialog.ui
    graphics.cpp
    sbapi_graphics.h
    font.h
    font.cpp



)


find_package(
    ALSA REQUIRED
    Threads REQUIRED
)

target_link_libraries(graphics-3d
    PRIVATE
        ALSA::ALSA
        Qt::Core
        Qt::Widgets
        Threads::Threads
)

#allows for static declaration of functions, bizaaar
#add them to the project CMAKE options to
# CMAKE_CXX_FLAGS eg, -fpermissive -anotheroption -then another
# CMAKE_C_FLAGS eg, -fpermissive -anotheroption -then another
add_compile_options(graphics-3d PRIVATE -fpermissive)



include(GNUInstallDirs)

install(TARGETS graphics-3d
    BUNDLE  DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

qt_generate_deploy_app_script(
    TARGET graphics-3d
    OUTPUT_SCRIPT deploy_script
    NO_UNSUPPORTED_PLATFORM_ERROR
)
install(SCRIPT ${deploy_script})
