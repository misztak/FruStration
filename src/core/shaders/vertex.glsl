#version 330 core

in ivec2 vertex_position;
in uvec3 vertex_color;

out vec3 color;

void main() {
    // convert from 0;1024 to -1;1
    float xpos = (float(vertex_position.x) / 512) - 1.0;
    // convert from 0;512 to -1;1 and flip the origin
    float ypos = 1.0 - (float(vertex_position.y) / 256);

    gl_Position.xyzw = vec4(xpos, ypos, 0.0, 1.0);

    // convert from 0;256 to 0;1
    color = vec3(float(vertex_color.r) / 255, float(vertex_color.g) / 255, float(vertex_color.b) / 255);
}