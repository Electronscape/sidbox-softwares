cmake_minimum_required(VERSION 3.19)
project(graphics-test LANGUAGES CXX)

find_package(Qt6 6.5 REQUIRED COMPONENTS Core Widgets)

qt_standard_project_setup()

qt_add_executable(graphics-test
    WIN32 MACOSX_BUNDLE
    main.cpp
    dialog.cpp
    dialog.h
    dialog.ui
    graphics.cpp
    sbapi_graphics.h
    font.h
    font.cpp


    hardware/audiosys.h
    hardware/audiosys.cpp
    hardware/stm32ram.h
    hardware/stm32ram.cpp

    smsemu/bus.cpp
    smsemu/types.h
    smsemu/internal.h
    smsemu/sms.h
    smsemu/cpu.cpp
    smsemu/sms_main.cpp
    smsemu/vdp.cpp
    smsemu/sms.cpp
    smsemu/sn76489.cpp
    smsemu/joypad.cpp

)


find_package(
    ALSA REQUIRED
    Threads REQUIRED
)

target_link_libraries(graphics-test
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
add_compile_options(graphics-test PRIVATE -fpermissive)



include(GNUInstallDirs)

install(TARGETS graphics-test
    BUNDLE  DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

qt_generate_deploy_app_script(
    TARGET graphics-test
    OUTPUT_SCRIPT deploy_script
    NO_UNSUPPORTED_PLATFORM_ERROR
)
install(SCRIPT ${deploy_script})
