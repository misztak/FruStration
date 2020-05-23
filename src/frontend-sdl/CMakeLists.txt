find_package(SDL2 CONFIG REQUIRED)

add_executable(frog
        main.cpp
        display.cpp)

target_include_directories(frog PUBLIC ../core ../../libs/gl3w ../../libs/imgui ${SDL2_INCLUDE_DIRS})
target_link_libraries(frog PRIVATE core gl3w imgui ${SDL2_LIBRARIES})

