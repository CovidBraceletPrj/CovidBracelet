# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.13.1)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(bluecovid)

target_sources(app PRIVATE src/main.c src/contacts.c src/exposure-notification.c src/gatt-service.c src/covid.c) 

zephyr_include_directories(${APPLICATION_SOURCE_DIR}/src/tls_config)

#use for testing only, will update keys every X seconds, default is 600
#add_definitions(-DEN_INTERVAL_LENGTH=11)
add_definitions(-DEN_INTERVAL_LENGTH=1)
