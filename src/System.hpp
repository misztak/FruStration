#pragma once

#include <memory>
#include <vector>
#include <string>

#include "Types.hpp"

class CPU;
class BUS;

class System {
public:
    System();
    ~System();
    void Init();
    bool LoadBIOS(const std::string& bios_path);
    void Run();

private:
    std::unique_ptr<CPU> cpu;
    std::unique_ptr<BUS> bus;
};
