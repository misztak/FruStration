add_executable(frog
        main.cpp
        system.cpp
        cpu.cpp
        bus.cpp
        dma.cpp
        gpu.cpp)

target_include_directories(frog PUBLIC .)