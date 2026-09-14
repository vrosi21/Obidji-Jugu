cmake_minimum_required(VERSION 3.18)

set(SOLUTION_NAME The_Travelling_Salesman)	#Unijeti naziv solution-a

#BEGIN dio koji ide u sve solutions
project(${SOLUTION_NAME})
set(SOURCE_ROOT ${CMAKE_CURRENT_LIST_DIR})
set(HOME_ROOT $ENV{HOME})
if(WIN32)
    set(HOME_ROOT "$ENV{USERPROFILE}")
endif()
if (WIN32)
	string(REPLACE "\\" "/" HOME_ROOT "${HOME_ROOT}")
endif()
set(WORK_ROOT ${HOME_ROOT}/Work)
set(NATID_SDK_ROOT "${WORK_ROOT}" CACHE PATH "natID SDK root (matching headers and binaries)")
include(${NATID_SDK_ROOT}/DevEnv/Common.cmake)
#END dio koji ide u sve solutions

include(${NATID_SDK_ROOT}/DevEnv/natGUI.cmake)

include(TTS.cmake)


