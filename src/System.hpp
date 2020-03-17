#pragma once

#include <memory>
#include <string>
#include <vector>

#include "Types.hpp"

namespace CPU {
class CPU;
}

class BUS;

class System {
public:
    System();
    ~System();
    void Init();
    bool LoadBIOS(const std::string& bios_path);
    void Run();

private:
    std::unique_ptr<CPU::CPU> cpu;
    std::unique_ptr<BUS> bus;
};
