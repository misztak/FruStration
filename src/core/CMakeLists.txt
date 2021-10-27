add_library(core
        emulator.cpp
        system.cpp
        cpu/cpu.cpp
        cpu/cpu_disasm.cpp
        bus.cpp
        dma.cpp
        gpu.cpp
        cdrom.cpp
        sw_renderer.cpp
        bios.cpp
        interrupt.cpp
        timer/timer.cpp
        debugger/debugger.cpp
        debugger/gdb_stub.cpp)

target_include_directories(core PUBLIC . cpu debugger timer util ../../libs/nanolog)
target_link_libraries(core PRIVATE fmt nanolog imgui)
