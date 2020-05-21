#include "renderer.h"

#include <cstring>

#include "shader.h"

Renderer::Renderer() {}

Renderer::~Renderer() {
    glDeleteVertexArrays(1, &VAO);
    glDeleteProgram(program);
}

void Renderer::Init() {
    // compile shaders
    vertex_shader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertex_shader, 1, &vertex_shader_string, nullptr);
    glCompileShader(vertex_shader);
    CheckCompileErrors(vertex_shader, "Vertex");

    frag_shader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(frag_shader, 1, &fragment_shader_string, nullptr);
    glCompileShader(frag_shader);
    CheckCompileErrors(frag_shader, "Fragment");

    program = glCreateProgram();
    glAttachShader(program, vertex_shader);
    glAttachShader(program, frag_shader);
    glLinkProgram(program);
    CheckCompileErrors(program, "Program");

    glDeleteShader(vertex_shader);
    glDeleteShader(frag_shader);

    glUseProgram(program);

    glGenVertexArrays(1, &VAO);
    glBindVertexArray(VAO);

    positions.Init();
    GLuint pos_index = GetAttributeIndex("vertex_position");
    glEnableVertexAttribArray(pos_index);
    glVertexAttribIPointer(pos_index, 2, GL_SHORT, 0, nullptr);

    colors.Init();
    GLuint clr_index = GetAttributeIndex("vertex_color");
    glEnableVertexAttribArray(clr_index);
    glVertexAttribIPointer(clr_index, 3, GL_UNSIGNED_BYTE, 0, nullptr);
}

void Renderer::PushTriangle(std::array<Position, 3> vertices, std::array<Color, 3> clrs) {
    if (curr_vertices + 3 > BUFFER_SIZE) {
        printf("Buffer full, forcing redraw\n");
        Draw();
    }

    for (u32 i=0; i<3; i++) {
        positions.Set(curr_vertices, vertices[i]);
        colors.Set(curr_vertices, clrs[i]);
        curr_vertices++;
    }
}

void Renderer::Draw() {
    glMemoryBarrier(GL_CLIENT_MAPPED_BUFFER_BARRIER_BIT);
    glDrawArrays(GL_TRIANGLES, 0, (GLsizei)curr_vertices);

    GLsync sync = glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0);
    for (;;) {
        auto result = glClientWaitSync(sync, GL_SYNC_FLUSH_COMMANDS_BIT, 10000000);
        if (result == GL_ALREADY_SIGNALED || result == GL_CONDITION_SATISFIED) break;
    }

    curr_vertices = 0;
}

GLuint Renderer::GetAttributeIndex(const char* attribute) {
    GLint index = glGetAttribLocation(program, attribute);
    Assert(index >= 0);

    return static_cast<GLuint>(index);
}

void Renderer::CheckCompileErrors(GLuint shader, const char* type) {
    int success;
    char infoLog[1024];
    if (std::strcmp(type, "Program")) {
        glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
        if (!success) {
            glGetShaderInfoLog(shader, 1024, nullptr, infoLog);
            printf("%s", infoLog);
            Panic("%s shader error!", type);
        }
    } else {
        glGetProgramiv(shader, GL_LINK_STATUS, &success);
        if (!success) {
            glGetProgramInfoLog(shader, 1024, nullptr, infoLog);
            printf("%s", infoLog);
            Panic("Program error!");
        }
    }
}
