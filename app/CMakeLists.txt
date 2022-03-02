# Build script for shredlink application firmware
#
# Copyright (c) 2021 Brian Bradley

cmake_minimum_required(VERSION 3.20.0)

list (APPEND SYSCALL_INCLUDE_DIRS 
    ${CMAKE_SOURCE_DIR}/../extras/drivers/wii
)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})

project(shredlink LANGUAGES C VERSION 0.2.0)

configure_file(app_version.h.in ${CMAKE_BINARY_DIR}/app/include/app_version.h)
target_include_directories(app PRIVATE ${CMAKE_BINARY_DIR}/app/include src)

set(SHREDLINK_SOURCES
        src/main.c
        src/hid.c
)

if (CONFIG_GAMEPAD_DAQ_POLL_MODE)
    list(APPEND SHREDLINK_SOURCES src/polling.c)
else()
    message(FATAL_ERROR "Polling mode is disabled but is currently required, as interrupt mode is not yet supported.")
endif()

if (CONFIG_TILT_SENSOR)
    list(APPEND SHREDLINK_SOURCES src/tilt.c)
endif()

include_directories(include)
target_sources(app PRIVATE ${SHREDLINK_SOURCES})
