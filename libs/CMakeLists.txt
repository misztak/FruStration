add_subdirectory(spdlog)
add_subdirectory(simpleini)
add_subdirectory(gl3w)
add_subdirectory(imgui)

if (USE_NATIVE_FILE_PICKER)
    add_subdirectory(nfd)
endif()
