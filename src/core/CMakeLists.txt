add_library(core
        system.cpp
        cpu.cpp
        cpu_disasm.cpp
        bus.cpp
        dma.cpp
        gpu.cpp
        sw_renderer.cpp
        bios.cpp)

target_include_directories(core PUBLIC ../../libs/imgui)
target_link_libraries(core PRIVATE fmt imgui)