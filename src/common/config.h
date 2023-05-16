#pragma once

#include <string>

#include "core/util/types.h"

// Global emulator configuration
namespace Config {

void SaveConfig();
void LoadConfig();

// TODO: flesh out ConfigEntry template
template<typename T>
struct ConfigEntry {
public:
    ConfigEntry() = delete;
    explicit ConfigEntry(T entry) : entry(entry) {};

    void Set(T value) {
        entry = value;
        Config::SaveConfig();
    }

    T Get() {
        return entry;
    }

private:
    T entry;
};

// General
extern ConfigEntry<std::string> bios_path;

// GDB
extern ConfigEntry<bool> gdb_server_enabled;
extern ConfigEntry<u16> gdb_server_port;


extern std::string psexe_file_path;
extern std::string ps_bin_file_path;

// ImGUI
extern bool draw_mem_viewer;
extern bool draw_cpu_state;
extern bool draw_gpu_state;
extern bool draw_debugger;
extern bool draw_timer_state;

}
