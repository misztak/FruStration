add_library(stb STATIC src/stb_image_write.c)

target_include_directories(stb PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_include_directories(stb INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include)
