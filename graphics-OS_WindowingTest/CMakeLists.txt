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


    codergirl/sys_font.h
    codergirl/sys_font.cpp
    sbapi_graphics.h

    codergirl/cg_windowex.h
    codergirl/cg_windowex.cpp

    codergirl/cg_gadgets.h
    codergirl/cg_gadgets.cpp

    codergirl/cg_renderer.h
    codergirl/cg_renderer.cpp

    sbapi_graphics.h

    codergirl/cg_input.h
    codergirl/cg_input.cpp

    bitmaps_src.cpp
    fastram.cpp
    fastram.h

    codergirl/cg_itemlist.cpp
    codergirl/cg_itemlist.h
    codergirl/cg_aroot.cpp
    codergirl/cg_aroot.h
    codergirl/cg_resources.cpp
    codergirl/cg_resources.h
    codergirl/cg_gad_button.cpp
    codergirl/cg_gad_button.h
    codergirl/cg_glyphs.h
    codergirl/cg_glyphs.cpp
    codergirl/cg_type.h
    codergirl/cg_gad_checkbox.h
    codergirl/cg_gad_radio.h
    codergirl/cg_gad_bitmapview.h
    codergirl/cg_gad_scrollbar.h
    codergirl/cg_gad_listbox.h

    codergirl/cg_gadgetrender.h
    codergirl/cg_gad_checkbox.cpp
    codergirl/cg_gad_radio.cpp
    codergirl/cg_gad_scrollbar.cpp
    codergirl/cg_gad_bitmapview.cpp
    codergirl/cg_gad_listbox.cpp
    codergirl/cg_wintype.h

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
