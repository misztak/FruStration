#!/usr/bin/env python3

import os

path = os.path.join(os.getcwd(), "src/core/shaders/")
arrays = ""

for filename in os.listdir(path):
    file_path = os.path.join(path, filename)
    if os.path.isdir(file_path):
        continue
    with open(file_path, 'r') as file:
        c_array = "static const char* " + filename.split('.')[0] + "_shader_string = \n"

        for line in file:
            c_line = '"' + line.replace('\n', '\\n') + '"\n'
            c_array += c_line

        c_array += ";\n\n"
        arrays += c_array

print(arrays)

res_path = os.path.join(os.getcwd(), "src/core/")
f = open(res_path + "shader.h", "w")
f.write(arrays)
f.close()
