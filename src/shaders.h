const char *basic_fragment_shader = R"FOO(
#version 330 core

uniform sampler2D in_texture_0;
uniform sampler3D in_texture_1;

out vec4 color;

void main() {
    color = vec4(0.4, 0.1, 0.3, 1.0);
}
)FOO";

const char *basic_vertex_shader = R"FOO(
#version 330 core

layout(location = 0) in vec3 in_vertex_position;

uniform mat4 model;
uniform mat4 view_projection;

void main(){
    gl_Position = view_projection * model * vec4(in_vertex_position, 1.0f);
}
)FOO";