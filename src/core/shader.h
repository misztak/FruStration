static const char* fragment_shader_string = 
"#version 330 core\n"
"\n"
"in vec3 color;\n"
"out vec4 frag_color;\n"
"\n"
"void main() {\n"
"    frag_color = vec4(color, 1.0);\n"
"}\n"
;

static const char* vertex_shader_string = 
"#version 330 core\n"
"\n"
"in ivec2 vertex_position;\n"
"in uvec3 vertex_color;\n"
"\n"
"out vec3 color;\n"
"\n"
"void main() {\n"
"    // convert from 0;1024 to -1;1\n"
"    float xpos = (float(vertex_position.x) / 512) - 1.0;\n"
"    // convert from 0;512 to -1;1 and flip the origin\n"
"    float ypos = 1.0 - (float(vertex_position.y) / 256);\n"
"\n"
"    gl_Position.xyzw = vec4(xpos, ypos, 0.0, 1.0);\n"
"\n"
"    // convert from 0;256 to 0;1\n"
"    color = vec3(float(vertex_color.r) / 255, float(vertex_color.g) / 255, float(vertex_color.b) / 255);\n"
"}"
;

