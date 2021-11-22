include_directories(timer http)
#add_executable(server server.cpp timer/lst_timer.cpp http/http_conn.cpp)
add_executable(server3 server3.cpp http/http_conn.cpp)
add_subdirectory(tests)