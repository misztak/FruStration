add_library(common
        log.cpp)

target_include_directories(common PUBLIC . "${PROJECT_SOURCE_DIR}/libs/spdlog/include")
target_link_libraries(common PUBLIC spdlog)
