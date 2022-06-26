#add_executable(test_http_parse test_http_parse.cpp)
add_executable(getAddr getAddr.cpp)

set(SRCS 
    ../src/buffer/buffer.cpp
    ../src/http/http_conn.cpp
    # ../src/http/http_request.cpp
    # ../src/http/http_response.cpp
    ../src/log/logger.cpp
    ../src/poller/epoller.cpp
    ../src/timer/heaptimer.cpp
    )

include_directories(../include ../src/http/ ../src/buffer ../src/log ../src/poller ../src/pool ../src/timer)
add_executable(server3 server3.cpp ${SRCS})
target_link_libraries(server3 pthread)