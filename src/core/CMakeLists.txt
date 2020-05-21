add_library(core
        system.cpp
        cpu.cpp
        cpu_disasm.cpp
        bus.cpp
        dma.cpp
        gpu.cpp
        renderer.cpp)

target_include_directories(core PUBLIC ../../libs/gl3w)
target_link_libraries(core PRIVATE fmt gl3w)