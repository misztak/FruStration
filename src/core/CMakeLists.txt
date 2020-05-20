add_library(core
        system.cpp
        cpu.cpp
        cpu_disasm.cpp
        bus.cpp
        dma.cpp
        gpu.cpp)

target_include_directories(core PUBLIC .)
target_link_libraries(core PRIVATE fmt)