# use cmake for nativefiledialog
set(NFD_SOURCES nativefiledialog/src/nfd_common.c)
if (WIN32)
    list(APPEND NFD_SOURCES nativefiledialog/src/nfd_win.cpp)
    list(APPEND NFD_DEFS UNICODE _UNICODE)
elseif (APPLE)
    list(APPEND NFD_SOURCES nativefiledialog/src/nfd_cocoa.m)
elseif (UNIX AND NOT APPLE AND NOT ANDROID)
    list(APPEND NFD_SOURCES nativefiledialog/src/nfd_gtk.c)
else()
    message(FATAL_ERROR "Unsupported system")
endif()
add_library(nativefiledialog STATIC ${NFD_SOURCES})
target_include_directories(nativefiledialog PUBLIC nativefiledialog/src/include)
target_compile_definitions(nativefiledialog PUBLIC ${NFD_DEFS})
if(UNIX AND NOT APPLE AND NOT ANDROID)
    find_package(PkgConfig)
    if (PKG_CONFIG_FOUND)
        pkg_check_modules(GTK "gtk+-3.0")
        if (GTK_FOUND)
            target_link_libraries(nativefiledialog ${GTK_LIBRARIES})
            add_definitions(${GTK_CFLAGS} ${GTK_CFLAGS_OTHER})
        else()
            message(FATAL_ERROR "Could not find GTK3 libraries")
        endif()
    endif()
endif()

add_subdirectory(spdlog)

add_definitions(-DIMGUI_IMPL_OPENGL_LOADER_GL3W)

add_library(gl3w STATIC gl3w/gl3w.c)
target_include_directories(gl3w PUBLIC gl3w)

add_library(imgui STATIC
        imgui/imgui.cpp
        imgui/imgui_demo.cpp
        imgui/imgui_draw.cpp
        imgui/imgui_impl_opengl3.cpp
        imgui/imgui_impl_sdl.cpp
        imgui/imgui_widgets.cpp
        imgui/imgui_memory_editor.h)
target_include_directories(imgui PUBLIC imgui gl3w ${SDL2_INCLUDE_DIRS})
target_link_libraries(imgui PRIVATE gl3w ${SDL2_LIBRARIES})
