add_library(common STATIC
        config.cpp
        log.cpp)

target_link_libraries(common PUBLIC simpleini spdlog)

define_file_basename_for_sources(common)
