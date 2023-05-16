add_library(gl3w STATIC
        include/GL/gl3w.h
        include/GL/glcorearb.h
        include/KHR/khrplatform.h
        src/GL/gl3w.c)

target_include_directories(gl3w PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_include_directories(gl3w INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include)
