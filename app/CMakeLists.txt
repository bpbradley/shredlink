# Build script for shredlink application firmware
#
# Copyright (c) 2021 Brian Bradley

cmake_minimum_required(VERSION 3.20.0)

list (APPEND SYSCALL_INCLUDE_DIRS 
    ${CMAKE_SOURCE_DIR}/../extras/drivers/wii
)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})

project(shredlink LANGUAGES C VERSION 0.1.0)

configure_file(app_version.h.in ${CMAKE_BINARY_DIR}/app/include/app_version.h)
target_include_directories(app PRIVATE ${CMAKE_BINARY_DIR}/app/include src)

target_sources(app PRIVATE src/main.c)
