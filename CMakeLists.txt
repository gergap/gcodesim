project(gcodesim C)
cmake_minimum_required(VERSION 3.2)

# version info
set(MAJOR_VERSION 0)
set(MINOR_VERSION 0)
set(PATCH_VERSION 1)
configure_file(version.h.in version.h)

option(ENABLE_POVRAY "Enables POVray output for rendering simulation." OFF)
if (ENABLE_POVRAY)
    add_definitions(-DPOVRAY_ANIM_OUTPUT)
endif()

set(SOURCES main.c voxelspace.c gcode.c)

add_executable(${PROJECT_NAME} ${SOURCES})
target_link_libraries(${PROJECT_NAME} m)

install(TARGETS ${PROJECT_NAME} DESTINATION bin)
install(FILES gcode/demo.gcode gcode/custom_bottom.gcode gcode/custom_top.gcode DESTINATION bin)

# enable Unit Testing
include(CTest)
if (BUILD_TESTING)
    add_subdirectory(test)
    #############
    # GOOD Cases:
    #############
    # only show help
    add_test(NAME showhelp
        COMMAND ${TESTDRIVER} $<TARGET_FILE:gcodesim> -h
        WORKING_DIRECTORY ${CMAKE_INSTALL_PREFIX}/bin)
    add_test(NAME simpletest
        COMMAND ${TESTDRIVER} $<TARGET_FILE:gcodesim> -W30 -H30 demo.gcode
        WORKING_DIRECTORY ${CMAKE_INSTALL_PREFIX}/bin)
    #############
    # BAD Cases:
    #############
    # no file name given
    add_test(NAME nofile
        COMMAND ${TESTDRIVER} $<TARGET_FILE:gcodesim>
        WORKING_DIRECTORY ${CMAKE_INSTALL_PREFIX}/bin)
    set_tests_properties(nofile PROPERTIES WILL_FAIL TRUE)
    # invalid file name given
    add_test(NAME invalidfile
        COMMAND ${TESTDRIVER} $<TARGET_FILE:gcodesim> invalid_xyz.gcode
        WORKING_DIRECTORY ${CMAKE_INSTALL_PREFIX}/bin)
    set_tests_properties(invalidfile PROPERTIES WILL_FAIL TRUE)
endif()

