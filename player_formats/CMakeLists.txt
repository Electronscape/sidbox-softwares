cmake_minimum_required(VERSION 3.16)

project(playerformats-test-poky LANGUAGES C)

set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED ON)

add_executable(playerformats-test-poky main.c
    main.h

    sidbox/host.h
    sidbox/host.c
)

find_package(ALSA REQUIRED)

target_link_libraries(playerformats-test-poky PRIVATE
    ALSA::ALSA
    m

)

target_compile_options(playerformats-test-poky PRIVATE
  -Wall -Wextra
  -Wshadow
  -Wconversion -Wsign-conversion
  -Wformat=2
  -Wno-unused-parameter
  -Wno-unused-variable
  -Wno-unused-function
)


include(GNUInstallDirs)
install(TARGETS playerformats-test-poky
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
