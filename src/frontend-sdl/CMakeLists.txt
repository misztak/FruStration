add_executable(pain
        main.cpp
        display.cpp)

target_include_directories(pain PUBLIC ../core ../../libs/gl3w ../../libs/imgui ${SDL2_INCLUDE_DIRS})
target_link_libraries(pain PRIVATE core gl3w imgui ${SDL2_LIBRARIES})

if (WIN32)
    target_link_libraries(pain PRIVATE ${SDL2MAIN_LIBRARIES})

    set_target_properties(pain PROPERTIES
        # leave a debug console open until we do proper logging
        #WIN32_EXECUTABLE TRUE
        DEBUG_POSTFIX "-debug")
endif()