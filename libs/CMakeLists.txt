add_library(fmt STATIC fmt/src/format.cc)
target_include_directories(fmt PUBLIC fmt)

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
