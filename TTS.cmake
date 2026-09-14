set(PROJECT_NAME obidji-jugu)

file(GLOB PROJECT_SOURCES CONFIGURE_DEPENDS ${CMAKE_CURRENT_LIST_DIR}/src/*.cpp)
file(GLOB PROJECT_INCS CONFIGURE_DEPENDS ${CMAKE_CURRENT_LIST_DIR}/include/*.h)
set(PROJECT_PLIST  ${CMAKE_CURRENT_LIST_DIR}/src/Info.plist)
file(GLOB PROJECT_INC_TD  ${MY_INC}/td/*.h)
file(GLOB PROJECT_INC_GUI ${MY_INC}/gui/*.h)

# add executable
add_executable(${PROJECT_NAME} ${PROJECT_INCS} ${PROJECT_SOURCES} ${PROJECT_INC_TD}  ${PROJECT_INC_GUI})
find_package(Threads REQUIRED)
target_link_libraries(${PROJECT_NAME} PRIVATE Threads::Threads)
target_compile_features(${PROJECT_NAME} PRIVATE cxx_std_20)
if(MSVC)
    target_compile_options(${PROJECT_NAME} PRIVATE /utf-8)
endif()
if(WIN32)
    target_sources(${PROJECT_NAME} PRIVATE res/appIcon/winAppIcon.rc)
endif()
setAppIcon(${PROJECT_NAME} "${CMAKE_CURRENT_LIST_DIR}")

# Add the public include directory so source files can #include headers from include/ and
# so we can keep header files out of src/
target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_LIST_DIR}/include)

source_group("inc"            FILES ${PROJECT_INCS})
source_group("inc\\td"        FILES ${PROJECT_INC_TD})
source_group("inc\\gui"        FILES ${PROJECT_INC_GUI})
source_group("src"            FILES ${PROJECT_SOURCES})



target_link_libraries(${PROJECT_NAME} PRIVATE debug ${MU_LIB_DEBUG} debug ${NATGUI_LIB_DEBUG} 
										optimized ${MU_LIB_RELEASE} optimized ${NATGUI_LIB_RELEASE})

setTargetPropertiesForGUIApp(${PROJECT_NAME} ${PROJECT_PLIST})

setIDEPropertiesForGUIExecutable(${PROJECT_NAME} ${CMAKE_CURRENT_LIST_DIR})

setPlatformDLLPath(${PROJECT_NAME})
