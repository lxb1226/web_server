cmake_minimum_required(VERSION 2.8)

project(web_server CXX)

if (NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Debug")
endif ()

set(CMAKE_CXX_COMPILER "g++")
set(CMAKE_CXX_FLAGS_DEBUG "-O0")
set(CMAKE_CXX_FLAGS_RELEASE "-O2 -finline-limit=1000 -DNDEBUG")
set(EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR}/bin)
set(LIBRARY_OUTPUT_PATH ${PROJECT_BINARY_DIR}/lib)

set(SOURCES 
    ${PROJECT_SOURCE_DIR}/src/buffer/buffer.cpp
    ${PROJECT_SOURCE_DIR}/src/http/httpconn.cpp
    ${PROJECT_SOURCE_DIR}/src/http/httprequest.cpp
    ${PROJECT_SOURCE_DIR}/src/http/httpresponse.cpp
    ${PROJECT_SOURCE_DIR}/src/log/logger.cpp
    ${PROJECT_SOURCE_DIR}/src/poller/epoller.cpp
    ${PROJECT_SOURCE_DIR}/src/server/server.cpp
    ${PROJECT_SOURCE_DIR}/src/timer/heaptimer.cpp)

    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++14 -g -ggdb")
include_directories(include src/buffer src/http src/log src/poller src/pool src/server src/timer)
add_executable(server main.cpp ${SOURCES})
target_link_libraries(server pthread)

add_subdirectory(tests)
