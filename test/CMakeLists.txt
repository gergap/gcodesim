add_executable(arctest arctest.c)
target_link_libraries(arctest m)

add_test(NAME arctest
    COMMAND $<TARGET_FILE:arctest>
    )

