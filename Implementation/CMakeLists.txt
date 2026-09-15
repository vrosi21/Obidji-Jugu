cmake_minimum_required(VERSION 3.18)

set(SOLUTION_NAME The_Travelling_Salesman)
project(${SOLUTION_NAME})

set(SOURCE_ROOT "${CMAKE_CURRENT_LIST_DIR}")

if(WIN32)
    file(TO_CMAKE_PATH "$ENV{USERPROFILE}" HOME_ROOT)
else()
    file(TO_CMAKE_PATH "$ENV{HOME}" HOME_ROOT)
endif()

if(NOT DEFINED NATID_SDK_ROOT)
    if(DEFINED ENV{NATID_SDK_ROOT})
        file(TO_CMAKE_PATH "$ENV{NATID_SDK_ROOT}" NATID_SDK_ROOT)
    else()
        set(NATID_SDK_ROOT "${HOME_ROOT}/natID.SDK")
    endif()
endif()

set(NATID_SDK_ROOT "${NATID_SDK_ROOT}"
    CACHE PATH "Path to the natID.SDK folder"
)

if(NOT EXISTS "${NATID_SDK_ROOT}/DevEnv/Common.cmake")
    message(FATAL_ERROR
        "natID SDK was not found at '${NATID_SDK_ROOT}'. "
        "Set -DNATID_SDK_ROOT=/path/to/natID.SDK."
    )
endif()

include("${NATID_SDK_ROOT}/DevEnv/Common.cmake")
include("${NATID_SDK_ROOT}/DevEnv/natGUI.cmake")

include("${SOURCE_ROOT}/TTS.cmake")