#include "System.hpp"

int main(int, char**) {
    const std::string bios_path = "../../bios/SCPH1001.BIN";

    System system;
    system.Init();
    if (!system.LoadBIOS(bios_path)) return 1;

    system.Run();

    return 0;
}
