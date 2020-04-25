add_executable(frog
        main.cpp
        system.cpp
        cpu.cpp
        cpu_disasm.cpp
        bus.cpp
        dma.cpp
        gpu.cpp)

target_include_directories(frog PUBLIC .)
target_link_libraries(frog PRIVATE fmt)