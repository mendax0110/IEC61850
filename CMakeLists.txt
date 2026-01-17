cmake_minimum_required(VERSION 3.16)
project(IEC61850 VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(BUILD_TESTS "Build tests" ON)
option(BUILD_QEMU "Build QEMU simulations" ON)
option(BUILD_DOCS "Build documentation with Doxygen" ON)
option(BUILD_STATIC "Build static binaries for QEMU" OFF)

include_directories(include)
file(GLOB_RECURSE SOURCES "src/**/*.cpp")

add_library(iec61850_sv ${SOURCES})

add_executable(iec61850_demo src/main.cpp)
target_link_libraries(iec61850_demo iec61850_sv)

add_executable(iec61850_client src/client.cpp)
target_link_libraries(iec61850_client iec61850_sv)

if(BUILD_STATIC)
    add_library(iec61850_sv_static STATIC ${SOURCES})
    target_include_directories(iec61850_sv_static PUBLIC include)
    target_compile_options(iec61850_sv_static PRIVATE -Wall -Wextra -Wpedantic)

    add_executable(iec61850_demo_static src/main.cpp)
    target_link_libraries(iec61850_demo_static iec61850_sv_static -static)
    set_target_properties(iec61850_demo_static PROPERTIES OUTPUT_NAME "iec61850_demo_static")

    add_executable(iec61850_client_static src/client.cpp)
    target_link_libraries(iec61850_client_static iec61850_sv_static -static)
    set_target_properties(iec61850_client_static PROPERTIES OUTPUT_NAME "iec61850_client_static")
endif()

if(BUILD_QEMU)
    add_executable(qemu_tool tools/qemu/qemu.cpp)
    target_compile_features(qemu_tool PRIVATE cxx_std_17)
    target_compile_options(qemu_tool PRIVATE -Wall -Wextra -Wpedantic)
endif()

target_include_directories(iec61850_sv PUBLIC include)
target_compile_options(iec61850_sv PRIVATE -Wall -Wextra -Wpedantic)

if(BUILD_DOCS)
    find_package(Doxygen)
    if(DOXYGEN_FOUND)
        add_custom_target(docs
                COMMAND ${DOXYGEN_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/docs/Doxyfile
                WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
                COMMENT "Generating API documentation with Doxygen"
                VERBATIM
        )
    else()
        message(WARNING "Doxygen not found. Documentation will not be built.")
    endif()
endif()

if(BUILD_TESTS)
    include(FetchContent)
    FetchContent_Declare(
            googletest
            URL https://github.com/google/googletest/archive/refs/tags/v1.14.0.zip
            DOWNLOAD_EXTRACT_TIMESTAMP TRUE
    )
    FetchContent_MakeAvailable(googletest)

    enable_testing()
    add_subdirectory(test)
endif()
