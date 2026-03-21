cmake_minimum_required(VERSION 3.16)

project(tse VERSION 1.4.1 LANGUAGES CXX)

include(GNUInstallDirs)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

file(GLOB SRC_FILES CONFIGURE_DEPENDS
    "${PROJECT_SOURCE_DIR}/src/*.cpp"
)

add_executable(tse ${SRC_FILES})

target_include_directories(tse PRIVATE
    ${PROJECT_SOURCE_DIR}/src
)

if(MSVC)
    target_compile_options(tse PRIVATE /W4 /permissive-)
else()
    target_compile_options(tse PRIVATE -Wall -Wextra -Wpedantic)
endif()

install(TARGETS tse
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)