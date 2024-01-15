add_subdirectory(spdlog)
add_subdirectory(simpleini)
add_subdirectory(gl3w)
add_subdirectory(imgui)
add_subdirectory(stb)

if (USE_NATIVE_FILE_PICKER)
    add_subdirectory(nfd)
endif()
