cmake_minimum_required(VERSION 3.12)

project(nstool
    VERSION 1.0.0
    DESCRIPTION "Nintendo Switch Tool"
    LANGUAGES CXX C
)

# Set C++ standard
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Platform detection
if(WIN32)
    set(PLATFORM_NAME "win32")
elseif(APPLE)
    set(PLATFORM_NAME "macos")
else()
    set(PLATFORM_NAME "linux")
endif()

# Output directories - use PROJECT_SOURCE_DIR/bin instead of CMAKE_BINARY_DIR
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/bin/${PLATFORM_NAME})
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/bin/${PLATFORM_NAME})
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/bin/${PLATFORM_NAME})

# Set output directory for all configurations (Debug, Release, etc.)
foreach(CONFIG ${CMAKE_CONFIGURATION_TYPES})
    string(TOUPPER ${CONFIG} CONFIG_UPPER)
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_${CONFIG_UPPER} ${PROJECT_SOURCE_DIR}/bin/${PLATFORM_NAME})
    set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_${CONFIG_UPPER} ${PROJECT_SOURCE_DIR}/bin/${PLATFORM_NAME})
    set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_${CONFIG_UPPER} ${PROJECT_SOURCE_DIR}/bin/${PLATFORM_NAME})
endforeach()

# Compiler flags
if(MSVC)
    add_compile_options(/W4)
    add_definitions(-D_CRT_SECURE_NO_WARNINGS)
else()
    add_compile_options(-Wall -Wno-unused-value -Wno-unused-but-set-variable)
    if(APPLE)
        add_compile_options(-Wno-unused-private-field)
    endif()
    add_compile_options(-fPIC)
endif()

# Windows-specific settings
if(WIN32 AND NOT MSVC)
    # MinGW settings
    add_compile_definitions(UNICODE _UNICODE)
    add_link_options(-municode -static-libgcc -static-libstdc++ -Wl,--subsystem,console:5.02)
endif()

# Add dependencies as subdirectories
add_subdirectory(deps/libfmt)
add_subdirectory(deps/liblz4)
add_subdirectory(deps/libmbedtls)
add_subdirectory(deps/libtoolchain)
add_subdirectory(deps/libpietendo)

# Collect all source files
file(GLOB_RECURSE NSTOOL_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/src/*.c
)

# Create executable
add_executable(nstool ${NSTOOL_SOURCES})

# Link dependencies
target_link_libraries(nstool
    PRIVATE
        pietendo
        toolchain
        fmt
        lz4
        mbedtls
)

# Include directories for nstool
target_include_directories(nstool
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

# Windows-specific libraries
if(WIN32 AND NOT MSVC)
    target_link_libraries(nstool PRIVATE ssp winpthread)
endif()

# Installation rules
install(TARGETS nstool
    RUNTIME DESTINATION bin
)
