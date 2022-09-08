cmake_minimum_required(VERSION 3.2.0)

set(LIB_NAME "aws-polly")

# needed for gettimeofday()
set(CMAKE_C_FLAGS "-D_XOPEN_SOURCE=600 -D_POSIX_C_SOURCE=200112L ${CMAKE_C_FLAGS}")

set(LIB_DIR ${CMAKE_CURRENT_SOURCE_DIR})
set(LIB_SRC
    ${LIB_DIR}/include/polly/polly.h
    ${LIB_DIR}/source/http_parser.c
    ${LIB_DIR}/source/http_parser.h
    ${LIB_DIR}/source/netio.c
    ${LIB_DIR}/source/netio.h
    ${LIB_DIR}/source/polly.c
    ${LIB_DIR}/source/sigv4.c
    ${LIB_DIR}/source/sigv4.h
)

set(LIB_PUB_INC
    ${LIB_DIR}/include
)

set(LIB_PRV_INC
    ${LIB_DIR}/source
)

set(LINK_LIBS
    mbedtls
    mbedcrypto
    mbedx509
    llhttp
)

# setup static library
add_library(${LIB_NAME} STATIC ${LIB_SRC})
set_target_properties(${LIB_NAME} PROPERTIES POSITION_INDEPENDENT_CODE 1)
target_include_directories(${LIB_NAME} PUBLIC ${LIB_PUB_INC})
target_include_directories(${LIB_NAME} PRIVATE ${LIB_PRV_INC})

target_link_libraries(${LIB_NAME} PUBLIC
    ${LINK_LIBS}
)

include(GNUInstallDirs)

install(TARGETS ${LIB_NAME}
        LIBRARY DESTINATION ${CMAKE_INSTALL_FULL_LIBDIR}
        ARCHIVE DESTINATION ${CMAKE_INSTALL_FULL_LIBDIR}
)

install(DIRECTORY ${LIB_PUB_INC}/
        DESTINATION ${CMAKE_INSTALL_FULL_INCLUDEDIR}
)
