#include "gdb_stub.h"

#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sstream>

#include "macros.h"
#include "fmt/format.h"

LOG_CHANNEL(GDB);

GDBServer::GDBServer(u16 port) {
    LOG_INFO << "Initializing GDB server on port " << port;

    s32 init_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (init_socket == -1) Panic("Failed to open socket");

    sockaddr_in init_sockaddr = {};
    init_sockaddr.sin_family = AF_INET;
    init_sockaddr.sin_addr.s_addr = INADDR_ANY;
    init_sockaddr.sin_port = htons(port);

    // make sure we can bind to the same port without waiting
    const s32 reuse_port = 1;
    if (setsockopt(init_socket, SOL_SOCKET, SO_REUSEPORT, (const char *) &reuse_port, sizeof(reuse_port)) < 0) {
        LOG_WARN << "Failed to set socket option REUSEPORT";
    }

    if (bind(init_socket, (sockaddr *) &init_sockaddr, sizeof(init_sockaddr)) == -1) {
        Panic("Failed to bind socket to port %d", port);
    }

    if (listen(init_socket, 20) == -1) {
        Panic("Failed to listen with server socket");
    }
    LOG_INFO << "Waiting for GDB client...";

    sockaddr_in gdb_sockaddr = {};
    socklen_t socklen = sizeof(gdb_sockaddr);
    gdb_socket = accept(init_socket, (sockaddr *) &gdb_sockaddr, &socklen);
    if (gdb_socket < 0) {
        Panic("Failed to connect to client");
    }

    LOG_INFO << "Connection established";

    while (keep_receiving) HandleClientRequest();

    if (init_socket != -1) {
        shutdown(init_socket, SHUT_RDWR);
    }
}

void GDBServer::HandleClientRequest() {
    // get the next command
    Receive();
    
    // handle ACK and ERR
    std::string_view request((const char *) rx_buffer.data());
    if (request.size() < 4) {
        LOG_WARN << fmt::format("Invalid packet, too small: '{}'", request);
        return;
    }
    if (request[0] == '+') request = request.substr(1);
    if (request[0] == '-') {
        LOG_WARN << "Client error";
        return;
    }
    
    // check if it's a valid packet
    if (request[0] != '$' || request[request.size() - 3] != '#') {
        LOG_WARN << fmt::format("Received invalid packet '{}'", request);
        return;
    }
    // TODO: verify checksum

    request = request.substr(1, request.size() - 4);
    LOG_DEBUG << fmt::format("Received packet '{}'", request);

    keep_receiving = true;
    switch (request[0]) {
        case '?':
            // send signal, not sure about which one
            Send("S00");
            break;
        case 'g':
            Send(ReadRegisters());
            keep_receiving = false;
            break;
        default:
            //LOG_DEBUG << fmt::format("Unsupported request '{}', sending empty reply", request);
            Send("");
    }
}

std::string GDBServer::ReadRegisters() {
    // number of registers in GDB remote protocol
    // starts with GP registers followed by special and (some) cop0 registers
    // and finally 35 unused FP registers
    constexpr u32 GDB_REGISTER_COUNT = 73;
    constexpr u32 GDB_USED_REGISTERS = 38;

    // registers are stored same way as everything else
    // 1 32-bit register == 8 hex chars
    // little endian
    std::stringstream ss;

    return ss.str();
}

void GDBServer::Send(std::string_view packet) {
    std::stringstream ss;
    ss << '+' << '$' << packet << '#' << CalcChecksum(packet);
    LOG_INFO << fmt::format("Sending packet '{}'", ss.str());

    if (send(gdb_socket, ss.str().c_str(), ss.str().size(), 0) == -1) {
        LOG_WARN << "Failed to send message to GDB client";
    }
}

void GDBServer::Receive() {
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

std::string GDBServer::CalcChecksum(std::string_view& packet) {
    u8 csum = 0;
    for (auto& c : packet) {
        csum += (u8) c;
    }
    return ValueToHex(csum);
}

std::string GDBServer::ValueToHex(u32 value) {
    constexpr u32 BUFFER_SIZE = 64;
    char buffer[BUFFER_SIZE];

    int result = snprintf(buffer, BUFFER_SIZE, "%02x", value);
    if (result < 0 || result > (int) BUFFER_SIZE) {
        LOG_WARN << "Failed to convert value to hex string";
    }

    return std::string(buffer);
}

std::string GDBServer::ValuesToHex(u8* start, u32 length) {
    constexpr u32 BUFFER_SIZE = 3;
    char buffer[BUFFER_SIZE] = {};
    std::stringstream ss;

    for (u8* ptr = start; ptr < start + length; ptr++) {
        int result = snprintf(buffer, BUFFER_SIZE, "%02x", *ptr);
        DebugAssert(result >= 0 && result < (int) BUFFER_SIZE);
        ss << buffer;
    }

    return ss.str();
}

GDBServer::~GDBServer() {
    if (gdb_socket != -1) {
        shutdown(gdb_socket, SHUT_RDWR);
        gdb_socket = -1;
    }
    LOG_INFO << "Shutting down GDB server";
}
