project(gcodesim C)
cmake_minimum_required(VERSION 3.2)

option(ENABLE_POVRAY "Enables POVray output for rendering simulation." OFF)
if (ENABLE_POVRAY)
    add_definitions(-DPOVRAY_ANIM_OUTPUT)
endif()

set(SOURCES main.c voxelspace.c gcode.c)

add_executable(${PROJECT_NAME} ${SOURCES})
target_link_libraries(${PROJECT_NAME} m)

install(TARGETS ${PROJECT_NAME} DESTINATION bin)
install(FILES custom_bottom.gcode custom_top.gcode DESTINATION bin)
