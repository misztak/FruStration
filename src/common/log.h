#pragma once

#include "spdlog/spdlog.h"

namespace Log {

void Init(spdlog::level::level_enum log_level);
void Shutdown();

}    //namespace Log

#define LOG_CHANNEL(Name) static const char* __Channel__ = #Name

// HACK: we misuse spdlog::source_loc to show our custom channel names as source function names
#define LogWithChannel(Level, Fmt, ...)                                                                                \
    spdlog::default_logger_raw()->log(spdlog::source_loc {__FILE__, __LINE__, __Channel__}, spdlog::level::Level, Fmt, \
                                      ##__VA_ARGS__)

#define LogTrace(Fmt, ...) LogWithChannel(trace, Fmt, ##__VA_ARGS__)
#define LogDebug(Fmt, ...) LogWithChannel(debug, Fmt, ##__VA_ARGS__)
#define LogInfo(Fmt, ...)  LogWithChannel(info, Fmt, ##__VA_ARGS__)
#define LogWarn(Fmt, ...)  LogWithChannel(warn, Fmt, ##__VA_ARGS__)
#define LogErr(Fmt, ...)   LogWithChannel(err, Fmt, ##__VA_ARGS__)
#define LogCrit(Fmt, ...)  LogWithChannel(critical, Fmt, ##__VA_ARGS__)
