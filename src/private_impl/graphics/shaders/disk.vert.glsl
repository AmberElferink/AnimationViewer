#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_GOOGLE_include_directive : require

#include "bridging_header.h"

layout(binding = 0, std140) uniform uniform_vertex_block_t {
    joint_uniform_t data;
} uniform_block;

layout(location = 0) in vec3 vertex_position;
layout(location = 0) out float edge;

void main() {
    gl_Position = uniform_block.data.vp * uniform_block.data.model * vec4(vertex_position * uniform_block.data.node_size, 1.0);
    edge = float(dot(vertex_position, vertex_position) > 0.0f);
}
