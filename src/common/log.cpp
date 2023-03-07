#include "log.h"

#include <array>

#include "spdlog/pattern_formatter.h"
#include "spdlog/spdlog.h"
#include "spdlog/sinks/stdout_color_sinks.h"

namespace Log {
namespace {

std::chrono::high_resolution_clock::time_point program_start = std::chrono::high_resolution_clock::now();

class time_since_launch_formatter : public spdlog::custom_flag_formatter {
public:
    void format(const spdlog::details::log_msg &, const std::tm &, spdlog::memory_buf_t &dest) override {
        auto time_since_launch = std::chrono::high_resolution_clock::now() - program_start;
        auto s = std::chrono::duration_cast<std::chrono::seconds>(time_since_launch);
        auto ms = std::chrono::duration_cast<std::chrono::microseconds>(time_since_launch - s);

        char buffer[32];
        int size = snprintf(buffer, 32, "%05ld.%06ld", s.count(), ms.count());
        dest.append(buffer, buffer + size);
    }

    std::unique_ptr<custom_flag_formatter> clone() const override {
        return spdlog::details::make_unique<time_since_launch_formatter>();
    }
};

}    //namespace

void Init() {
    auto logger = spdlog::stdout_color_mt("stdout_logger");
    spdlog::set_default_logger(logger);

    spdlog::set_level(spdlog::level::trace);

    auto formatter = std::make_unique<spdlog::pattern_formatter>();
    formatter->add_flag<time_since_launch_formatter>('*');

    formatter->set_pattern("%^[%*][%l][%!] %v%$");
    spdlog::set_formatter(std::move(formatter));
}

void Shutdown() {
    spdlog::shutdown();
}

}    //namespace Log
