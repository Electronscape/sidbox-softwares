cmake_minimum_required(VERSION 3.16)

project(asound-test LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(asound-test main.cpp)

find_package(ALSA REQUIRED)

target_link_libraries(asound-test PRIVATE ALSA::ALSA)

include(GNUInstallDirs)
install(TARGETS asound-test
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
