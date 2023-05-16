add_executable(frustration
        main.cpp
        display.cpp)

target_link_libraries(frustration PRIVATE common core gl3w imgui nfd ${SDL2_LIBRARIES})

if (WIN32)
    target_link_libraries(frustration PRIVATE ${SDL2MAIN_LIBRARIES})

    set_target_properties(frustration PROPERTIES
        # leave a debug console open until we do proper logging
        #WIN32_EXECUTABLE TRUE
        DEBUG_POSTFIX "-debug")
endif()

define_file_basename_for_sources(frustration)
