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

set(CMAKE_INCLUDE_CURRENT_DIR on)

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

# .deb package generation
include(InstallRequiredSystemLibraries)
#set(CPACK_SET_DESTDIR "on")
#set(CPACK_PACKAGING_INSTALL_PREFIX "/usr/local")
set(CPACK_GENERATOR "DEB")
set(CPACK_DEBIAN_PACKAGE_MAINTAINER "Gerhard Gappmeier")
set(CPACK_PACKAGE_DESCRIPTION "GCode Simulator")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Can be used to simulate Gcode exported from pcb-gcode before trying to use it on a real CNC mill")
#set(CPACK_PACKAGE_VENDOR "package maintainer")
#set(CPACK_PACKAGE_CONTACT "package maintainer email")
set(CPACK_PACKAGE_VERSION_MAJOR "${MAJOR_VERSION}")
set(CPACK_PACKAGE_VERSION_MINOR "${MINOR_VERSION}")
set(CPACK_PACKAGE_VERSION_PATCH "${PATCH_VERSION}")
#SET(CPACK_PACKAGE_FILE_NAME "${CMAKE_PROJECT_NAME}_${MAJOR_VERSION}.${MINOR_VERSION}.${CPACK_PACKAGE_VERSION_PATCH}")
#SET(CPACK_SOURCE_PACKAGE_FILE_NAME "${CMAKE_PROJECT_NAME}_${MAJOR_VERSION}.${MINOR_VERSION}.${CPACK_PACKAGE_VERSION_PATCH}")
#demo dependencies
#SET(CPACK_DEBIAN_PACKAGE_DEPENDS " kate , plasma-desktop, libc6 (>= 2.1.3) ")
SET(CPACK_DEBIAN_PACKAGE_PRIORITY "optional")
SET(CPACK_DEBIAN_PACKAGE_SECTION "devel")
SET(CPACK_DEBIAN_ARCHITECTURE ${CMAKE_SYSTEM_PROCESSOR})
include(CPack)

