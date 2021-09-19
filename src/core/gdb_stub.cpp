#include "gdb_stub.h"

#ifdef _WIN32
#include <winsock2.h>
#include <io.h>
#include <iphlpapi.h>
#include <ws2tcpip.h>
#define SHUT_RDWR 2
using ssize_t = long long;
#else
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#endif

#include <array>
#include <sstream>
#include <string>
#include <string_view>
#include <charconv>

#include "macros.h"
#include "fmt/format.h"
#include "cpu.h"

LOG_CHANNEL(GDB);

namespace GDB {
namespace {
constexpr char HEX_CHARS[] = "0123456789abcdef";

bool server_enabled = false;
bool keep_receiving = false;

std::array<s8, 256> rx_buffer = {};

s32 gdb_socket = -1;

Debugger* debugger = nullptr;

#ifdef _WIN32
WSADATA InitData;
#endif
}

static std::string ValuesToHex(u8* start, u32 length) {
    std::stringstream ss;

    for (u8* ptr = start; ptr < start + length; ptr++) {
        char snd = HEX_CHARS[*ptr & 0xF];
        char fst = HEX_CHARS[(*ptr >> 4) & 0xF];
        ss << fst << snd;
    }

    return ss.str();
}

static std::string CalcChecksum(std::string_view& packet) {
    u8 csum = 0;
    for (auto& c : packet) {
        csum += (u8) c;
    }
    return ValuesToHex(&csum, 1);
}

static std::string ReadRegisters() {
    // all registers supported by GDB remote protocol
    // starts with GP registers followed by special and (some) cop0 registers
    // and finally 35 unused FP registers
    constexpr u32 GDB_UNUSED_REGISTERS = 35;

    // registers are stored same way as everything else
    // 1 32-bit register == 8 hex chars
    // little endian
    std::stringstream ss;

    CPU::CPU* cpu = debugger->GetCPU();

    ss << ValuesToHex((u8*) cpu->gp.r, 32 * sizeof(u32));
    ss << ValuesToHex((u8*) &cpu->cp.sr, sizeof(u32));
    ss << ValuesToHex((u8*) &cpu->sp.lo, sizeof(u32));
    ss << ValuesToHex((u8*) &cpu->sp.hi, sizeof(u32));
    ss << ValuesToHex((u8*) &cpu->cp.bad_vaddr, sizeof(u32));
    ss << ValuesToHex((u8*) &cpu->cp.cause, sizeof(u32));
    ss << ValuesToHex((u8*) &cpu->sp.pc, sizeof(u32));

    // unused FP registers
    for (u32 i = 0; i < GDB_UNUSED_REGISTERS; i++) {
        ss << "00000000";
    }

    return ss.str();
}

static std::string ReadRegister(u32 index) {
    constexpr u32 GDB_REGISTER_COUNT = 73;
    constexpr u32 GDB_USED_REGISTERS = 38;

    if (index >= GDB_REGISTER_COUNT) {
        LOG_WARN << fmt::format("Invalid index {} for register", index);
    }

    CPU::CPU* cpu = debugger->GetCPU();
    std::stringstream ss;

    if (index >= GDB_USED_REGISTERS) {
        // FP register or invalid register
        ss << "00000000";
    } else if (index < 32) {
        // GP register
        ss << ValuesToHex((u8*) &cpu->gp.r[index], sizeof(u32));
    } else {
        // everything else
        u32 rel_index = index - 32;
        u32 reg = 0;

        switch (rel_index) {
            case 0: reg = cpu->cp.sr.value; break;
            case 1: reg = cpu->sp.lo; break;
            case 2: reg = cpu->sp.hi; break;
            case 3: reg = cpu->cp.bad_vaddr; break;
            case 4: reg = cpu->cp.cause.value; break;
            case 5: reg = cpu->sp.pc; break;
        }

        ss << ValuesToHex((u8*) &reg, sizeof(u32));
    }

    return ss.str();
}

static void Send(std::string_view packet) {
    std::stringstream ss;
    ss << '+' << '$' << packet << '#' << CalcChecksum(packet);
    LOG_INFO << fmt::format("Sending packet '{}'", ss.str());

    if (send(gdb_socket, ss.str().c_str(), ss.str().size(), 0) == -1) {
        LOG_WARN << "Failed to send message to GDB client";
    }
}

static void Receive() {
    std::fill(rx_buffer.begin(), rx_buffer.end(), 0);
    ssize_t rx_len;

    do {
        rx_len = read(gdb_socket, rx_buffer.data(), rx_buffer.size());
        if (rx_len < 0) {
            LOG_WARN << "Failed to receive package";
            break;
        }

        if (rx_len == 0 || rx_buffer[0] == '-') {
            LOG_WARN << "Client error";
            break;
        }

    } while (rx_len == 1 && rx_buffer[0] == '+');
}

bool HandleClientRequest() {
    if (!server_enabled) return false;

    // get the next command
    Receive();

    // handle ACK and ERR
    std::string_view request((const char *) rx_buffer.data());
    if (request.size() < 4) {
        LOG_WARN << fmt::format("Invalid packet, too small: '{}'", request);
        return false;
    }
    if (request[0] == '+') request = request.substr(1);
    if (request[0] == '-') {
        LOG_WARN << "Client error";
        return false;
    }

    // check if it's a valid packet
    if (request[0] != '$' || request[request.size() - 3] != '#') {
        LOG_WARN << fmt::format("Received invalid packet '{}'", request);
        return false;
    }

    // remove packet header and checksum
    std::string_view csum = request.substr(request.size() - 2, 2);
    request = request.substr(1, request.size() - 4);

    // verify checksum
    if (csum != CalcChecksum(request)) {
        LOG_WARN << fmt::format("Received packet with incorrect checksum '{}', expected {}", csum,
                                CalcChecksum(request));
        return false;
    }

    LOG_INFO << fmt::format("Received packet '{}'", request);
    std::string_view params = request.substr(1);

    keep_receiving = true;
    switch (request[0]) {
        case '?':
            // send signal, not sure about which one
            Send("S00");
            break;
        case 'g':
            // read all registers
            Send(ReadRegisters());
            break;
        case 'p': {
            // read single register, index is in hex
            u32 index;
            auto result = std::from_chars(params.data(), params.data() + params.size(), index, 16);
            if (result.ec == std::errc()) {
                Send(ReadRegister(index));
            } else {
                LOG_WARN << "Failed to parse register index";
                Send("E00");
            }
            break; }
        case 'm':
            // read memory
            Send("");
            keep_receiving = false;
            break;
        default:
            //LOG_DEBUG << fmt::format("Unsupported request '{}', sending empty reply", request);
            Send("");
    }

    return true;
}

void Init(u16 port, Debugger* dbg) {
    if (server_enabled) return;
    debugger = dbg;

    LOG_INFO << "Initializing GDB server on port " << port;
    // set to false in case init fails
    server_enabled = false;

    s32 init_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (init_socket == -1) {
        LOG_WARN << "Failed to open socket";
        return;
    }

    sockaddr_in init_sockaddr = {};
    init_sockaddr.sin_family = AF_INET;
    init_sockaddr.sin_addr.s_addr = INADDR_ANY;
    init_sockaddr.sin_port = htons(port);

#ifdef _WIN32
    WSAStartup(MAKEWORD(2, 2), &InitData);
#endif

    // make sure we can bind to the same port without waiting
    const s32 reuse_port = 1;
    if (setsockopt(init_socket, SOL_SOCKET, SO_REUSEADDR, (const char *) &reuse_port, sizeof(reuse_port)) < 0) {
        LOG_WARN << "Failed to set socket option REUSEPORT";
        return;
    }

    if (bind(init_socket, (sockaddr *) &init_sockaddr, sizeof(init_sockaddr)) == -1) {
        LOG_WARN << fmt::format("Failed to bind socket to port {}", port);
        return;
    }

    if (listen(init_socket, 20) == -1) {
        LOG_WARN << "Failed to listen with server socket";
        return;
    }
    LOG_INFO << "Waiting for GDB client...";

    sockaddr_in gdb_sockaddr = {};
    socklen_t socklen = sizeof(gdb_sockaddr);
    gdb_socket = accept(init_socket, (sockaddr *) &gdb_sockaddr, &socklen);
    if (gdb_socket < 0) {
        LOG_WARN << "Failed to connect to client";
        return;
    }

    // we first need to handle multiple initialization packets
    keep_receiving = true;

    server_enabled = true;
    LOG_INFO << "Connection established";

    shutdown(init_socket, SHUT_RDWR);
}

bool ServerRunning() {
    return server_enabled;
}

bool KeepReceiving() {
    return keep_receiving;
}

void Shutdown() {
    if (gdb_socket != -1) {
        shutdown(gdb_socket, SHUT_RDWR);
        gdb_socket = -1;
    }

    keep_receiving = false;
    server_enabled = false;

#ifdef _WIN32
    WSACleanup();
#endif

    LOG_INFO << "Shutting down GDB server";
}

}

