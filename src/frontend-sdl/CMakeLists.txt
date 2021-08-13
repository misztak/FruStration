add_executable(frustration
        main.cpp
        display.cpp)

target_include_directories(frustration PUBLIC ../core ../../libs/gl3w ../../libs/imgui ../../libs/nativefiledialog/src/include ${SDL2_INCLUDE_DIRS})
target_link_libraries(frustration PRIVATE core gl3w imgui nativefiledialog ${SDL2_LIBRARIES})

if (WIN32)
    target_link_libraries(frustration PRIVATE ${SDL2MAIN_LIBRARIES})

    set_target_properties(frustration PROPERTIES
        # leave a debug console open until we do proper logging
        #WIN32_EXECUTABLE TRUE
        DEBUG_POSTFIX "-debug")
endif()