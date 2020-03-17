#include "BUS.hpp"

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
    // BIOS start 0xBFC00000
    u32 mask = (address & 0xFFF00000) >> 20;

    switch (mask) {
        case 0xBFC: {
            printf("Load from BIOS at address 0x%08X\n", address);
            u32 rel_address = address - 0xBFC00000;
            Assert(rel_address < 0x80000);
            return *reinterpret_cast<u32 *>(bios.data() + rel_address);
        }
        default:
            Panic("Tried to load from unimplemented address 0x%08X", address);
    }

    // unreachable
}

void BUS::Store32(u32 address, u32 value) { Panic("Not implemented"); }
