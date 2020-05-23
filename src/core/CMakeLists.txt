add_library(core
        system.cpp
        cpu.cpp
        cpu_disasm.cpp
        bus.cpp
        dma.cpp
        gpu.cpp
        sw_renderer.cpp)

target_link_libraries(core PRIVATE fmt)