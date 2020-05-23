cmake_minimum_required(VERSION 3.15)
project(FrogStation)

set(CMAKE_CXX_STANDARD 17)

if(CMAKE_COMPILER_IS_GNUCC OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -DDEBUG -Wall -O2")
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -DDEBUG -Wall -O2 -D_GLIBCXX_DEBUG -D_GLIBCXX_DEBUG_PEDANTIC")

    set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -Wall")
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -Wall")
endif()

#message("CXX debug flags: ${CMAKE_CXX_FLAGS_DEBUG}")
#message("CXX release flags: ${CMAKE_CXX_FLAGS_RELEASE}")

set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,--start-group -lGL -ldl")

add_subdirectory(libs)
add_subdirectory(src)