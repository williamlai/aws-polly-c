set(APP_NAME "polly_mp3_download")

set(${APP_NAME}_SRC
    ${APP_NAME}.c
)

# build static executable
add_executable(${APP_NAME} ${${APP_NAME}_SRC})
set_target_properties(${APP_NAME} PROPERTIES OUTPUT_NAME ${APP_NAME})
# support usleep
target_compile_definitions(${APP_NAME} PUBLIC -D_XOPEN_SOURCE=600 -D_POSIX_C_SOURCE=200112L)
target_compile_definitions(${APP_NAME} PUBLIC -DBUILD_EXECUTABLE_WITH_STATIC_LIBRARY)

target_link_libraries(${APP_NAME}
    aws-polly
)
