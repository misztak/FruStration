#include "config.h"

#include <filesystem>

#include "SimpleIni.h"

#include "asserts.h"
#include "log.h"

LOG_CHANNEL(CONFIG);

// Global emulator configuration
namespace Config {
namespace {
constexpr char CONFIG_FILE[] = "frustration.ini";

// ini section names
constexpr char SEC_GENERAL[] = "General";
constexpr char SEC_GDB[] = "GDB";
}


// ### SAVED TO FILE ###

// General
ConfigEntry<std::string> bios_path {"SCPH1001.BIN"};

// GDB
ConfigEntry<bool> gdb_server_enabled {false};
ConfigEntry<u16> gdb_server_port {45678};

// ### NOT SAVED TO FILE ###

std::string psexe_file_path;
std::string ps_bin_file_path;

// ImGUI
bool draw_mem_viewer = true;
bool draw_cpu_state = true;
bool draw_gpu_state = true;
bool draw_renderer_state = true;
bool draw_debugger = true;
bool draw_timer_state = true;

void SaveConfig() {
    CSimpleIniA ini;

    ini.SetValue(SEC_GENERAL, "BiosFilePath", bios_path.Get().c_str());
    ini.SetValue(SEC_GDB, "ServerEnabled", std::to_string(gdb_server_enabled.Get()).c_str());
    ini.SetValue(SEC_GDB, "ServerPort", std::to_string(gdb_server_port.Get()).c_str());

    // if the config file exists this will overwrite the above set values
    // otherwise it will create a new config file
    auto rc = ini.SaveFile(CONFIG_FILE);
    if (rc != SI_OK) LogErr("Failed to save config file");
}

void LoadConfig() {
    // if no config exists first create a new one with compile-time values
    if (!std::filesystem::exists(CONFIG_FILE)) {
        LogInfo("No config file found. Creating new frustration.ini");
        SaveConfig();
    }

    CSimpleIniA ini;
    auto rc = ini.LoadFile(CONFIG_FILE);
    if (rc != SI_OK) {
        LogErr("Failed to open config frustration.ini");
        return;
    }

    bios_path.Set(ini.GetValue(SEC_GENERAL, "BiosFilePath", ""));
    gdb_server_enabled.Set(ini.GetBoolValue(SEC_GDB, "ServerEnabled", false));
    gdb_server_port.Set((u16) ini.GetLongValue(SEC_GDB, "ServerPort", 0));
}

}