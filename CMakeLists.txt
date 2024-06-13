project(Snake)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED True)

add_executable(Snake main.cpp)

include_directories(./include)

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)
target_link_libraries(Snake ${CMAKE_THREAD_LIBS_INIT})

target_link_libraries(Snake PUBLIC ncurses)


