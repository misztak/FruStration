add_library(imgui STATIC
        include/imconfig.h
        include/imgui.h
        include/imgui_impl_opengl3.h
        include/imgui_impl_sdl.h
        include/imgui_internal.h
        include/imgui_memory_editor.h
        include/imstb_rectpack.h
        include/imstb_textedit.h
        include/imstb_truetype.h
        src/imgui.cpp
        src/imgui_demo.cpp
        src/imgui_draw.cpp
        src/imgui_impl_opengl3.cpp
        src/imgui_impl_sdl.cpp
        src/imgui_widgets.cpp)

target_compile_definitions(imgui PRIVATE -DIMGUI_IMPL_OPENGL_LOADER_GL3W)
target_include_directories(imgui PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_include_directories(imgui INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(imgui PRIVATE gl3w ${SDL2_LIBRARIES})
