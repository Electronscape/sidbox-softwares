cmake_minimum_required(VERSION 3.16)

project(playerformats-test LANGUAGES C)

set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED ON)

add_executable(playerformats-test main.c
    main.h

    sidbox/host.h
    sidbox/host.c
    sidbox/player/playmed.c
    sidbox/player/playmed.h








)

find_package(ALSA REQUIRED)

target_link_libraries(playerformats-test PRIVATE
    ALSA::ALSA
    m

)

target_compile_options(playerformats-test PRIVATE
  -Wall -Wextra
  -Wshadow
  -Wconversion -Wsign-conversion
  -Wformat=2
  -Wno-unused-parameter
  -Wno-unused-variable
  -Wno-unused-function
)


include(GNUInstallDirs)
install(TARGETS playerformats-test
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
