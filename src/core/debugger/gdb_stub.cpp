#include "gdb_stub.h"

#ifdef _WIN32
// clang-format off
#include <winsock2.h>
#include <io.h>
#include <iphlpapi.h>
#include <ws2tcpip.h>
// clang-format on

#define SHUT_RDWR 2
using ssize_t = long long;
#else
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#endif

#include <array>
#include <charconv>
#include <optional>
#include <tuple>
#include <sstream>
#include <string>
#include <string_view>

#include "bus.h"
#include "common/asserts.h"
#include "common/log.h"
#include "cpu/cpu.h"
#include "debugger.h"
#include "gdb_target_info.h"
#include "system.h"

LOG_CHANNEL(GDB);

namespace GDB {
namespace {
constexpr char HEX_CHARS[] = "0123456789abcdef";

constexpr u32 RX_BUFFER_SIZE = 1024;

constexpr u32 GDB_REGISTER_COUNT = 73;
constexpr u32 GDB_USED_REGISTERS = 38;
constexpr u32 GDB_UNUSED_REGISTERS = 35;
constexpr u32 GP_REGISTER_COUNT = 32;

constexpr u8 CTRL_C = 3;

bool server_enabled = false;

// changes to true after receiving continue or step command from client
// resets once emulation stopped again
bool received_step_or_continue_cmd = false;

std::array<s8, RX_BUFFER_SIZE> rx_buffer = {};

s32 gdb_socket = -1;

Debugger* debugger = nullptr;

#ifdef _WIN32
WSADATA InitData;
#endif
}    //namespace

static std::string ValuesToHex(u8* start, u32 length) {
    std::stringstream ss;

    for (u8* ptr = start; ptr < start + length; ptr++) {
        char snd = HEX_CHARS[*ptr & 0xF];
        char fst = HEX_CHARS[(*ptr >> 4) & 0xF];
        ss << fst << snd;
    }

    return ss.str();
}

static std::optional<u32> FromHexChars(std::string_view string_view) {
    constexpr s32 BASE_16 = 16;

    u32 value;
    auto result = std::from_chars(string_view.data(), string_view.data() + string_view.size(), value, BASE_16);

    return (result.ec == std::errc()) ? std::optional(value) : std::nullopt;
}

static std::string CalcChecksum(std::string_view packet) {
    u8 csum = 0;
    for (auto& c : packet) {
        csum += (u8)c;
    }
    return ValuesToHex(&csum, 1);
}

static std::tuple<u32, u32> ParseStartAndOffset(std::string_view packet) {
    u32 start_delim = packet.find_last_of(':');
    u32 offset_delim = packet.find_last_of(',');

    u32 start = 0, offset = 0;
    auto result = std::from_chars(packet.data() + start_delim + 1, packet.data() + offset_delim, start);
    if (result.ec != std::errc()) LogWarn("Failed to parse start index of partial send command");

    result = std::from_chars(packet.data() + offset_delim + 1, packet.data() + packet.size(), offset);
    if (result.ec != std::errc()) LogWarn("Failed to parse offset index of partial send command");

    return std::make_tuple(start, offset);
}

static std::string ReadRegisters() {
    // all registers supported by GDB remote protocol
    // starts with GP registers followed by special and (some) cop0 registers
    // and finally 35 unused FP registers

    // registers are stored same way as everything else
    // 1 32-bit register == 8 hex chars
    // little endian
    std::stringstream ss;

    auto& cpu = debugger->GetContext()->cpu;

    ss << ValuesToHex((u8*)cpu->gp.r, GP_REGISTER_COUNT * sizeof(u32));
    ss << ValuesToHex((u8*)&cpu->cp.sr, sizeof(u32));
    ss << ValuesToHex((u8*)&cpu->sp.lo, sizeof(u32));
    ss << ValuesToHex((u8*)&cpu->sp.hi, sizeof(u32));
    ss << ValuesToHex((u8*)&cpu->cp.bad_vaddr, sizeof(u32));
    ss << ValuesToHex((u8*)&cpu->cp.cause, sizeof(u32));
    ss << ValuesToHex((u8*)&cpu->sp.pc, sizeof(u32));

    // unused FP registers
    for (u32 i = 0; i < GDB_UNUSED_REGISTERS; i++) {
        ss << "00000000";
    }

    return ss.str();
}

static std::string ReadRegister(u32 index) {
    if (index >= GDB_REGISTER_COUNT) {
        LogWarn("Invalid register index {}", index);
    }

    auto& cpu = debugger->GetContext()->cpu;
    std::stringstream ss;

    if (index >= GDB_USED_REGISTERS) {
        // FP register or invalid register
        ss << "00000000";
    } else if (index < GP_REGISTER_COUNT) {
        // GP register
        ss << ValuesToHex((u8*)&cpu->gp.r[index], sizeof(u32));
    } else {
        // everything else
        u32 rel_index = index - GP_REGISTER_COUNT;
        u32 reg = 0;

        switch (rel_index) {
            case 0: reg = cpu->cp.sr.value; break;
            case 1: reg = cpu->sp.lo; break;
            case 2: reg = cpu->sp.hi; break;
            case 3: reg = cpu->cp.bad_vaddr; break;
            case 4: reg = cpu->cp.cause.value; break;
            case 5: reg = cpu->sp.pc; break;
        }

        ss << ValuesToHex((u8*)&reg, sizeof(u32));
    }

    return ss.str();
}

static std::string ReadMemory(u32 start, u32 length) {
    auto& bus = debugger->GetContext()->bus;
    std::stringstream ss;

    for (u32 i = 0; i < length; i++) {
        u8 value = bus->Peek(start + i);
        ss << ValuesToHex(&value, sizeof(u8));
    }

    return ss.str();
}

static void Send(std::string_view packet) {
    std::stringstream ss;
    ss << '+' << '$' << packet << '#' << CalcChecksum(packet);
    LogInfo("Sending packet '{}'", ss.str());

    if (send(gdb_socket, ss.str().c_str(), ss.str().size(), 0) == -1) {
        LogWarn("Failed to send message to GDB client");
    }
}

static void Receive() {
    std::fill(rx_buffer.begin(), rx_buffer.end(), 0);
    ssize_t rx_len;

    rx_len = read(gdb_socket, rx_buffer.data(), rx_buffer.size());

    if (rx_len < 1) {
        LogWarn("Failed to receive package: {}", rx_len);
    }
}

static bool ReceivedNewPackage() {
    fd_set read_fds;
    FD_ZERO(&read_fds);
    FD_SET(gdb_socket, &read_fds);

    // timeout will be set to zero, select will exit immediately (polling)
    struct timeval tv = {};

    if (select(gdb_socket + 1, &read_fds, nullptr, nullptr, &tv) < 0) {
        LogWarn("Call to select failed");
        return false;
    }

    return FD_ISSET(gdb_socket, &read_fds);
}

void HandleClientRequest() {
    if (!server_enabled) return;

    // respond to finished continue or step command
    if (received_step_or_continue_cmd) {
        // make sure we aren't responding before the emulator stopped (should never happen)
        DebugAssert(debugger->GetContext()->cpu->halt);

        // reset to normal command mode
        received_step_or_continue_cmd = false;

        // send the pending ACK and a SIGTRAP
        Send("S05");
        // exit early to give the client some time to send new requests
        return;
    }

    // poll for new request
    if (!ReceivedNewPackage()) return;

    // get the new request
    Receive();

    // handle interrupt from client, treat it like a SIGINT
    if (rx_buffer[0] == CTRL_C) {
        LogInfo("Received interrupt from client");
        Send("S02");
        debugger->SetPausedState(true, false);
        return;
    }

    std::string_view request((const char*)rx_buffer.data());

    // handle ACK and ERR
    if (request[0] == '+') {
        // nothing else to do
        if (request.size() == 1) return;
        // consume ACK
        request = request.substr(1);
    }
    if (request[0] == '-') {
        LogWarn("Client error");
        return;
    }

    if (request.size() < 4) {
        LogWarn("Invalid packet, too small: '{}'", request);
        return;
    }

    // check if it's a valid packet
    if (request[0] != '$' || request[request.size() - 3] != '#') {
        LogWarn("Received invalid packet '{}'", request);
        return;
    }

    // remove packet header and checksum
    std::string_view csum = request.substr(request.size() - 2, 2);
    request = request.substr(1, request.size() - 4);

    // verify checksum
    if (csum != CalcChecksum(request)) {
        LogWarn("Received packet with incorrect checksum '{}', expected {}", csum, CalcChecksum(request));
        return;
    }

    LogInfo("Received packet '{}'", request);
    std::string_view params = request.substr(1);

    switch (request[0]) {
        case '?':
            // send SIGINT at start of session
            Send("S02");
            break;
        case 'q':
        {
            if (request.compare(0, 10, "qSupported") == 0) {
                // we have to provide target information, especially the memory map
                //Send("PacketSize=1024;qXfer:features:read+;qXfer:memory-map:read+");
                Send("PacketSize=1024;qXfer:memory-map:read+");
            } else if (request.compare(0, 31, "qXfer:features:read:target.xml:") == 0) {
                std::string_view target_xml(MIPS_TARGET_CONFIG, sizeof(MIPS_TARGET_CONFIG));

                auto [start, offset] = ParseStartAndOffset(request);
                LogDebug("Start: {}, Offset: {}", start, offset);

                Send(target_xml.substr(start, offset));
            } else if (request.compare(0, 23, "qXfer:memory-map:read::") == 0) {
                std::string_view mem_map_xml(PSX_MEMORY_MAP, sizeof(PSX_MEMORY_MAP));

                auto [start, offset] = ParseStartAndOffset(request);
                LogDebug("Start: {}, Offset: {}", start, offset);

                Send(mem_map_xml);
            } else {
                Send("");
            }
            break;
        }
        case 'c':
            // continue
            LogDebug("Received continue command, resuming emulator...");
            debugger->SetPausedState(false, false);
            received_step_or_continue_cmd = true;
            break;
        case 's':
            // single step
            LogDebug("Received single step command, resuming emulator...");
            debugger->SetPausedState(false, true);
            received_step_or_continue_cmd = true;
            break;
        case 'g':
            // read all registers
            Send(ReadRegisters());
            break;
        case 'G':
            // TODO: write all registers
            Send("OK");
            break;
        case 'k':
            // client disconnected, kill server
            LogInfo("GDB client closed connection");
            Shutdown();
            break;
        case 'p':
        {
            // read single register, index is in hex
            auto index = FromHexChars(params);
            if (index) {
                Send(ReadRegister(index.value()));
            } else {
                LogWarn("Failed to parse register index");
                Send("E00");
            }
            break;
        }
        case 'm':
        {
            // read memory
            u32 delim_pos = params.find(',');
            std::string_view addr_str = params.substr(0, delim_pos);
            std::string_view len_str = params.substr(delim_pos + 1);

            auto address = FromHexChars(addr_str);
            auto length = FromHexChars(len_str);
            if (!address || !length) {
                LogWarn("Failed to parse memory address and/or length");
                Send("E00");
            } else {
                u32 end_address = address.value() + length.value();
                LogDebug("Reading memory from 0x{:08x} to 0x{:08x}", address.value(), end_address);
                Send(ReadMemory(address.value(), length.value()));
            }
            break;
        }
        case 'M':
            // TODO: write memory
            Send("OK");
            break;
        case 'z':
        case 'Z':
        {
            // add/remove breakpoint
            char type = params[0];
            if (type != '0' && type != '1') {
                LogWarn("Unsupported z/Z command type: {}", type);
                Send("");
                break;
            }
            char bp_size = params[params.size() - 1];
            if (bp_size != '4') {
                LogWarn("Unsupported z/Z command breakpoint size: {}", bp_size);
                Send("");
                break;
            }

            // 32-bit sw/hw breakpoint
            DebugAssert(params.size() >= 5);
            std::string_view bp_address_str = params.substr(2, params.size() - 4);

            auto bp_address = FromHexChars(bp_address_str);
            if (bp_address) {
                bool add_bp = request[0] == 'Z';
                LogDebug("{} breakpoint at address 0x{:08x}", add_bp ? "Added" : "Removed",
                                         bp_address.value());
                if (add_bp) {
                    debugger->AddBreakpoint(bp_address.value());
                } else {
                    debugger->RemoveBreakpoint(bp_address.value());
                }
                Send("OK");
            } else {
                LogWarn("Failed to parse breakpoint address");
                Send("E00");
            }

            break;
        }
        default:
            //LOG_DEBUG << fmt::format("Unsupported request '{}', sending empty reply", request);
            Send("");
    }
}

void Init(u16 port, Debugger* _debugger) {
    if (server_enabled) return;
    debugger = _debugger;

    LogInfo("Initializing GDB server on port {}", port);
    // set to false in case init fails
    server_enabled = false;

    s32 init_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (init_socket == -1) {
        LogWarn("Failed to open socket");
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
    // can't use SO_REUSEPORT because windows does not support it
    const s32 reuse_port = 1;
    if (setsockopt(init_socket, SOL_SOCKET, SO_REUSEADDR, (const char*)&reuse_port, sizeof(reuse_port)) < 0) {
        LogWarn("Failed to set socket option REUSEPORT");
        return;
    }

    if (bind(init_socket, (sockaddr*)&init_sockaddr, sizeof(init_sockaddr)) == -1) {
        LogWarn("Failed to bind socket to port {}", port);
        return;
    }

    if (listen(init_socket, 20) == -1) {
        LogWarn("Failed to listen with server socket");
        return;
    }
    LogInfo("Waiting for GDB client...");

    sockaddr_in gdb_sockaddr = {};
    socklen_t socklen = sizeof(gdb_sockaddr);

    gdb_socket = accept(init_socket, (sockaddr*)&gdb_sockaddr, &socklen);
    if (gdb_socket < 0) {
        LogWarn("Failed to connect to client");
        return;
    }

    server_enabled = true;
    LogInfo("Connection established");

    shutdown(init_socket, SHUT_RDWR);
}

bool ServerRunning() {
    return server_enabled;
}

void Shutdown() {
    if (gdb_socket != -1) {
        shutdown(gdb_socket, SHUT_RDWR);
        gdb_socket = -1;

        LogInfo("Shutting down GDB server");
    }

    server_enabled = false;

#ifdef _WIN32
    WSACleanup();
#endif
}

}    //namespace GDB
