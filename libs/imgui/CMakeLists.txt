add_library(imgui STATIC
        include/imconfig.h
        include/imgui.h
        include/imgui_impl_opengl3.h
        include/imgui_impl_opengl3_loader.h
        include/imgui_impl_sdl2.h
        include/imgui_internal.h
        include/imstb_rectpack.h
        include/imstb_textedit.h
        include/imstb_truetype.h
        include/imgui_memory_editor.h
        src/imgui.cpp
        src/imgui_demo.cpp
        src/imgui_draw.cpp
        src/imgui_impl_opengl3.cpp
        src/imgui_impl_sdl2.cpp
        src/imgui_tables.cpp
        src/imgui_widgets.cpp)

target_include_directories(imgui PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include ${CMAKE_CURRENT_SOURCE_DIR}/src ${SDL2_INCLUDE_DIRS})
target_include_directories(imgui INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(imgui PRIVATE gl3w ${SDL2_LIBRARIES})
