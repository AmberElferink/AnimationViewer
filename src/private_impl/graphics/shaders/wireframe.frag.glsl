#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_GOOGLE_include_directive : require
#extension GL_OES_standard_derivatives : enable

#include "bridging_header.h"

layout(binding = 0, std140) uniform uniform_fragment_block_t {
    joint_uniform_t data;
} uniform_block;

layout(location = 0) in float edge;
layout(location = 0) out vec4 out_color;

void main() {
    out_color.rgb = vec3(1.0f, 1.0f, 1.0f);
    out_color.a = smoothstep(1.0f - fwidth(edge) * 2, 1.0f, edge);
    out_color *= uniform_block.data.color;
}
