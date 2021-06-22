cmake_minimum_required(VERSION 3.13.1)
include($ENV{ZEPHYR_BASE}/cmake/app/boilerplate.cmake NO_POLICY_SCOPE)
project(test)

if(DEFINED NATIVE_POSIX)
    include(FetchContent)
    FetchContent_Declare(
        exposure-notification
        GIT_REPOSITORY  https://github.com/prathje/exposure-notification
        GIT_TAG         origin/main
    )
    FetchContent_GetProperties(exposure-notification)
    if (NOT exposure-notification_POPULATED)
        FetchContent_Populate(exposure-notification)
    endif ()
    target_include_directories(app PUBLIC ${exposure-notification_SOURCE_DIR}/include)

    FILE(GLOB app_sources ../src/*.c* ${exposure-notification_SOURCE_DIR}/src/*.c*)
else()
    FILE(GLOB app_sources ../src/*.c*)
endif()

target_sources(app PRIVATE ${app_sources})
