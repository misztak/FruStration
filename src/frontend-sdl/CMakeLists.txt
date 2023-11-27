add_executable(frustration
        main.cpp
        display.cpp)

target_include_directories(frustration PRIVATE ${SDL2_INCLUDE_DIRS})
target_link_libraries(frustration PRIVATE common core gl3w imgui ${SDL2_LIBRARIES})

if (USE_NATIVE_FILE_PICKER)
    target_link_libraries(frustration PRIVATE nfd)
    target_compile_definitions(frustration PRIVATE USE_NFD)
endif()

if (WIN32)
    target_link_libraries(frustration PRIVATE ${SDL2MAIN_LIBRARIES})

    set_target_properties(frustration PROPERTIES
        # leave a debug console open until we do proper logging
        #WIN32_EXECUTABLE TRUE
        DEBUG_POSTFIX "-debug")
endif()

define_file_basename_for_sources(frustration)
