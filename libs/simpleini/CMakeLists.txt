add_library(simpleini STATIC
        include/ConvertUTF.h
        include/SimpleIni.h
        src/ConvertUTF.c)

target_include_directories(simpleini PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/include" "${CMAKE_CURRENT_SOURCE_DIR}/src")
target_include_directories(simpleini INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/include")
