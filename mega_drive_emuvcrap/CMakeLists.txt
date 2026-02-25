cmake_minimum_required(VERSION 3.19)
project(graphics-crap_md LANGUAGES C CXX)

find_package(Qt6 6.5 REQUIRED COMPONENTS Core Widgets)

qt_standard_project_setup()

qt_add_executable(graphics-crap_md
    WIN32 MACOSX_BUNDLE
    main.cpp
    dialog.cpp
    dialog.h
    dialog.ui
    graphics.c
    sbapi_graphics.h
    font.h
    font.cpp
    megadriveem/megadrive.c
    megadriveem/megadrive.h
    megadriveem/bus.c
    megadriveem/bus.h
    megadriveem/z80_stub.c
    megadriveem/z80_stub.h
    megadriveem/m68k_core.c
    megadriveem/m68k_core.h
    megadriveem/renderer.c
    megadriveem/renderer.h
    megadriveem/rom.c
    megadriveem/rom.h
    megadriveem/input.c
    megadriveem/input.h
    megadriveem/common.h
    megadriveem/vdp.c
    megadriveem/vdp.h
)


find_package(
    ALSA REQUIRED
    Threads REQUIRED
)

target_link_libraries(graphics-crap_md
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
add_compile_options(graphics-crap_md PRIVATE -fpermissive)



include(GNUInstallDirs)

install(TARGETS graphics-crap_md
    BUNDLE  DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

qt_generate_deploy_app_script(
    TARGET graphics-crap_md
    OUTPUT_SCRIPT deploy_script
    NO_UNSUPPORTED_PLATFORM_ERROR
)
install(SCRIPT ${deploy_script})
