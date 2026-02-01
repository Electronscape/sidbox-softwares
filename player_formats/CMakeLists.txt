cmake_minimum_required(VERSION 3.16)

project(playerformats-test LANGUAGES C)

set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED ON)

add_executable(playerformats-test main.c
    main.h

    sidbox/players/sidplay/cpu6502.c
    sidbox/players/sidplay/cpu6502.h
    sidbox/players/sidplay/sid8579.c
    sidbox/players/sidplay/sid8579.h

    sidbox/players/sidplay/cia.h
    sidbox/players/sidplay/cia.c
    sidbox/players/sidplay/vic.h
    sidbox/players/sidplay/vic.c
    sidbox/players/sidplay/bus.h
    sidbox/players/sidplay/bus.c

    sidbox/players/sidplay/playsid.c
    sidbox/players/sidplay/playsid.h
)

find_package(ALSA REQUIRED)

target_link_libraries(playerformats-test PRIVATE ALSA::ALSA)

include(GNUInstallDirs)
install(TARGETS playerformats-test
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
