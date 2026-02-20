cmake_minimum_required(VERSION 3.16)

project(vgm-packerv1 LANGUAGES C)

set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED ON)

add_executable(vgm-packerv1 main.c
    main.h
    sidbox/host.h
    sidbox/host.c
    sidbox/vgzpacker.c
    sidbox/vgzpacker.h
)

find_package(ALSA REQUIRED)

target_link_libraries(vgm-packerv1 PRIVATE
    #ALSA::ALSA
    #m
)

target_compile_options(vgm-packerv1 PRIVATE
  -Wall -Wextra
  -Wshadow
  -Wconversion -Wsign-conversion
  -Wformat=2
  -Wno-unused-parameter
  -Wno-unused-variable
  -Wno-unused-function
)


include(GNUInstallDirs)
install(TARGETS vgm-packerv1
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
