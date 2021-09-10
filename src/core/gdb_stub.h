#pragma once

#include <array>
#include <string>
#include <string_view>

#include "types.h"

class GDBServer {
public:
    GDBServer(u16 port);
    ~GDBServer();

    void HandleClientRequest();
private:
    void Send(std::string_view packet);
    void Receive();

    std::string ReadRegisters();

    std::string CalcChecksum(std::string_view& packet);
    std::string ValueToHex(u32 value);
    std::string ValuesToHex(u8* start, u32 length);

    std::array<s8, 256> rx_buffer = {};

    bool keep_receiving = true;
    s32 gdb_socket = -1;
};