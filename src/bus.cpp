#include "bus.h"

#include <fstream>

BUS::BUS() : bios(BIOS_FILE_SIZE) {}

// use once BUS needs CPU member
void BUS::Init() {}

bool BUS::LoadBIOS(const std::string& path) {
    printf("Loading BIOS...\n");

    std::ifstream file(path);
    if (!file || !file.good()) {
        printf("Failed to open BIOS file %s\n", path.c_str());
        return false;
    }
    file.seekg(0, std::ifstream::end);
    long length = file.tellg();
    file.seekg(0, std::ifstream::beg);

    if (length != BIOS_FILE_SIZE) {
        printf("BIOS file %s has invalid length %ld\n", path.c_str(), length);
        return false;
    }

    file.read(reinterpret_cast<char*>(bios.data()), BIOS_FILE_SIZE);
    printf("BIOS loaded\n");
    return true;
}

u32 BUS::Load32(u32 address) {
    Assert((address & 0x3) == 0);
    u32 mask = (address & 0xFFF00000) >> 20;

    switch (mask) {
        case 0xBFC: {
            //printf("Load from BIOS at address 0x%08X\n", address);
            u32 rel_address = address - 0xBFC00000;
            Assert(rel_address < 0x80000);
            return *reinterpret_cast<u32*>(bios.data() + rel_address);
        }
        default:
            Panic("Tried to load from unimplemented address range [0x%08X]", address);
    }

    // unreachable
}

void BUS::Store32(u32 address, u32 value) {
    Assert((address & 0x3) == 0);
    u32 mask = (address & 0xFFF00000) >> 20;

    switch (mask) {
        case 0x1F8:
            switch ((address & 0xF000) >> 12) {
                case 0: {
                    Panic("Scratchpad not implemented [0x%08X]", address);
                    break;
                }
                case 1: {
                    switch (address) {
                        case 0x1F801060:
                            printf("Store call to IO Port RAM_SIZE [0x%08X] - Ignored\n", address);
                            break;
                        default:
                            printf("Store call to IO Ports [0x%08X] - Ignored\n", address);
                    }
                    break;
                }
                case 2: {
                    Panic("Expansion Region 2 (IO Ports) not implemented [0x%08X]", address);
                    break;
                }
            }
            break;
        case 0xBFC:
            printf("Tried to store value in BIOS address range [0x%08X]\n", address);
            break;
        default:
            Panic("Tried to store into unimplemented address range [0x%08X]", address);
    }

    // unreachable
}
