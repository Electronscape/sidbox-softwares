cmake_minimum_required(VERSION 3.16)

project(playerformats-test LANGUAGES C)

set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED ON)

add_executable(playerformats-test main.c

    sidbox/players/sidplay/cpu6502.c
    sidbox/players/sidplay/cpu6502.h
    sidbox/players/sidplay/cpu_test.c
    sidbox/players/sidplay/sid8579.c
    sidbox/players/sidplay/sid8579.h
    sidbox/players/sidplay/sidplaybus.h
    sidbox/players/sidplay/sidplaybus.c
    song.c
    sidbox/players/sidplay/playsid.c
    sidbox/players/sidplay/playsid.h
    main.h
    song.h
    sidbox/players/sidplay/ciairq.h
    sidbox/players/sidplay/ciairq.c



)

find_package(ALSA REQUIRED)

target_link_libraries(playerformats-test PRIVATE ALSA::ALSA)

include(GNUInstallDirs)
install(TARGETS playerformats-test
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
