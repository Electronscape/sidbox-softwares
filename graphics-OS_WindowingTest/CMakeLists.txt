cmake_minimum_required(VERSION 3.19)
project(graphics-gui_testing LANGUAGES CXX)

find_package(Qt6 6.5 REQUIRED COMPONENTS Core Widgets)

qt_standard_project_setup()

qt_add_executable(graphics-gui_testing
    WIN32 MACOSX_BUNDLE
    main.cpp
    dialog.cpp
    dialog.h
    dialog.ui
    graphics.cpp
    font.h
    font.cpp
    sbapi_graphics.h

    sbx_windowex.h
    sbx_windowex.cpp




    sbx_gadgets.h

    sbx_render.h
    sbx_render.cpp
    sbx_gadgets.cpp

    sbx_input.h
    sbx_input.cpp

)

find_package(
    ALSA REQUIRED
    Threads REQUIRED
)

target_link_libraries(graphics-gui_testing
    PRIVATE
        Qt::Core
        Qt::Widgets
        Threads::Threads
)

#allows for static declaration of functions, bizaaar
#add them to the project CMAKE options to
# CMAKE_CXX_FLAGS eg, -fpermissive -anotheroption -then another
# CMAKE_C_FLAGS eg, -fpermissive -anotheroption -then another
add_compile_options(graphics-gui_testing PRIVATE -fpermissive)



include(GNUInstallDirs)

install(TARGETS graphics-gui_testing
    BUNDLE  DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

qt_generate_deploy_app_script(
    TARGET graphics-gui_testing
    OUTPUT_SCRIPT deploy_script
    NO_UNSUPPORTED_PLATFORM_ERROR
)
install(SCRIPT ${deploy_script})
