set(PROJECT_NAME The_Travelling_Salesman)				#Naziv prvog projekta u solution-u

file(GLOB PROJECT_SOURCES  ${CMAKE_CURRENT_LIST_DIR}/src/*.cpp)
file(GLOB PROJECT_INCS  ${CMAKE_CURRENT_LIST_DIR}/include/*.h)
set(PROJECT_PLIST  ${CMAKE_CURRENT_LIST_DIR}/src/Info.plist)
file(GLOB PROJECT_INC_TD  ${MY_INC}/td/*.h)
file(GLOB PROJECT_INC_GUI ${MY_INC}/gui/*.h)

# add executable
add_executable(${PROJECT_NAME} ${PROJECT_INCS} ${PROJECT_SOURCES} ${PROJECT_INC_TD}  ${PROJECT_INC_GUI})

# Add the public include directory so source files can #include headers from include/ and
# so we can keep header files out of src/
target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_LIST_DIR}/include)

source_group("inc"            FILES ${PROJECT_INCS})
source_group("inc\\td"        FILES ${PROJECT_INC_TD})
source_group("inc\\gui"        FILES ${PROJECT_INC_GUI})
source_group("src"            FILES ${PROJECT_SOURCES})



target_link_libraries(${PROJECT_NAME} debug ${MU_LIB_DEBUG} debug ${NATGUI_LIB_DEBUG} 
										optimized ${MU_LIB_RELEASE} optimized ${NATGUI_LIB_RELEASE})

setTargetPropertiesForGUIApp(${PROJECT_NAME} ${PROJECT_PLIST})

setIDEPropertiesForGUIExecutable(${PROJECT_NAME} ${CMAKE_CURRENT_LIST_DIR})

setPlatformDLLPath(${PROJECT_NAME})
