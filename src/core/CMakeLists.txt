add_library(core STATIC
        emulator.cpp
        system.cpp
        bus.cpp
        dma.cpp
        gpu.cpp
        cdrom.cpp
        bios.cpp
        interrupt.cpp
        cpu/cpu.cpp
        cpu/cpu_disasm.cpp
        cpu/gte.cpp
        renderer/renderer.h
        renderer/renderer_sw.cpp
        renderer/renderer_opengl.cpp
        renderer/opengl_helper.cpp
        timer/timers.cpp
        timer/timer.cpp
        timer/timer_blank.cpp
        timer/timer_system.cpp
        debugger/debugger.cpp
        debugger/gdb_stub.cpp)

target_include_directories(core PUBLIC .)
target_link_libraries(core PRIVATE common imgui gl3w)

define_file_basename_for_sources(core)
