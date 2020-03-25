#include "bus.h"

#include <fstream>

BUS::BUS() : bios(BIOS_FILE_SIZE), ram(RAM_SIZE, 0xCA) {}

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

u32 BUS::MaskRegion(u32 address) { return address & MEM_REGION_MASKS[address >> 29]; }

template <typename ValueType>
ValueType BUS::Load(u32 address) {
    static_assert(std::is_same<ValueType, u32>::value || std::is_same<ValueType, u16>::value ||
                  std::is_same<ValueType, u8>::value);
    u32 masked_address = MaskRegion(address);
    u32 mask = (masked_address & 0xFF000000) >> 24;

    switch (mask) {
        case 0x00:  // RAM
            Assert(masked_address < RAM_SIZE);
            return *reinterpret_cast<ValueType*>(ram.data() + masked_address);
        case 0x1F:
            switch ((masked_address & 0xF00000) >> 20) {
                case 0x0:
                case 0x1:
                case 0x2:
                case 0x3:
                case 0x4:
                case 0x5:
                case 0x6:
                case 0x7:  // Expansion Region 1
                    printf("Tried to load from Expansion Region 1 [0x%08X]\n", address);
                    return 0xFF;
                case 0x8:
                    switch ((masked_address & 0xF000) >> 12) {
                        case 0x0: {  // Scratchpad
                            u32 rel_address = masked_address - 0x1F800000;
                            Assert(rel_address < 1024);
                            Panic("Tried to load from Scratchpad [0x%08X]", address);
                            break;
                        }
                        case 0x1: {  // IO Ports
                            u32 rel_address = masked_address - 0x1F801000;
                            Assert(rel_address < 1024 * 8);
                            if (masked_address == 0x1F801074) return 0x00;
                            Panic("Tried to load from IO Ports [0x%08X]", address);
                            break;
                        }
                        case 0x2: {  // Expansion Region 2
                            u32 rel_address = masked_address - 0x1F802000;
                            Assert(rel_address < 1024 * 8);
                            Panic("Tried to load from Expansion Region 2 [0x%08X]", address);
                            break;
                        }
                        default:
                            Panic("Tried to load from invalid address range [0x%08X]", address);
                    }
                    break;
                case 0xA: {  // Expansion Region 3
                    u32 rel_address = masked_address - 0x1FA00000;
                    Assert(rel_address < 1024 * 2048);
                    Panic("Tried to load from Expansion Region 3 [0x%08X]", address);
                    break;
                }
                case 0xC: {  // BIOS
                    // printf("Load from BIOS at address 0x%08X\n", address);
                    u32 rel_address = masked_address - 0x1FC00000;
                    Assert(rel_address < BIOS_FILE_SIZE);
                    return *reinterpret_cast<ValueType*>(bios.data() + rel_address);
                }
                default:
                    Panic("Tried to load from invalid address range [0x%08X]", address);
            }
        case 0xFF: {  // Cache Control
            u32 rel_address = masked_address - 0xFFFE0000;
            Assert(rel_address < 512);
            Panic("Tried to load from Cache Control [0x%08X]", address);
            break;
        }
        default:
            Panic("Tried to load from invalid address range [0x%08X]", address);
    }

    // unreachable
}

template <typename Value>
void BUS::Store(u32 address, Value value) {
    static_assert(std::is_same<decltype(value), u32>::value || std::is_same<decltype(value), u16>::value ||
                  std::is_same<decltype(value), u8>::value);
    u32 masked_address = MaskRegion(address);
    u32 mask = (masked_address & 0xFF000000) >> 24;

    switch (mask) {
        case 0x00:  // RAM
            Assert(masked_address < RAM_SIZE);
            std::copy(reinterpret_cast<u8*>(&value), reinterpret_cast<u8*>(&value) + sizeof(value),
                      ram.data() + masked_address);
            break;
        case 0x1F:
            switch ((masked_address & 0xF00000) >> 20) {
                case 0x0:
                case 0x1:
                case 0x2:
                case 0x3:
                case 0x4:
                case 0x5:
                case 0x6:
                case 0x7:  // Expansion Region 1
                    Panic("Tried to store in Expansion Region 1 [0x%08X]", address);
                case 0x8:
                    switch ((masked_address & 0xF000) >> 12) {
                        case 0x0: {  // Scratchpad
                            u32 rel_address = masked_address - 0x1F800000;
                            Assert(rel_address < 1024);
                            Panic("Tried to store in Scratchpad [0x%08X]", address);
                            break;
                        }
                        case 0x1: {  // IO Ports
                            u32 rel_address = masked_address - 0x1F801000;
                            Assert(rel_address < 1024 * 8);
                            printf("Store call to IO Ports [0x%X @ 0x%08X] - Ignored\n", value, address);
                            break;
                        }
                        case 0x2: {  // Expansion Region 2
                            u32 rel_address = masked_address - 0x1F802000;
                            Assert(rel_address < 1024 * 8);
                            printf("Tried to store in Expansion Region 2 [0x%X @ 0x%08X] - Ignored\n", value, address);
                            break;
                        }
                        default:
                            Panic("Tried to store in invalid address range [0x%08X]", address);
                    }
                    break;
                case 0xA: {  // Expansion Region 3
                    u32 rel_address = masked_address - 0x1FA00000;
                    Assert(rel_address < 1024 * 2048);
                    Panic("Tried to store in Expansion Region 3 [0x%08X]", address);
                    break;
                }
                case 0xC: {  // BIOS
                    printf("Tried to store value in BIOS address range [0x%08X] - Ignored\n", address);
                    break;
                }
                default:
                    Panic("Tried to store in invalid address range [0x%08X]", address);
            }
            break;
        case 0xFF: {  // Cache Control
            u32 rel_address = masked_address - 0xFFFE0000;
            Assert(rel_address < 512);
            printf("Store call to Cache Control [0x%X @ 0x%08X] - Ignored\n", value, address);
            break;
        }
        default:
            Panic("Tried to store in invalid address range [0x%08X]", address);
    }

    // unreachable
}

template u32 BUS::Load<u32>(u32 address);
template u16 BUS::Load<u16>(u32 address);
template u8 BUS::Load<u8>(u32 address);

template void BUS::Store<u32>(u32 address, u32 value);
template void BUS::Store<u16>(u32 address, u16 value);
template void BUS::Store<u8>(u32 address, u8 value);
