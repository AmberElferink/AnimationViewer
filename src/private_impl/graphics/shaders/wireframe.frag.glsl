#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_GOOGLE_include_directive : require
#extension GL_OES_standard_derivatives : enable

#include "bridging_header.h"

layout(location = 0) in float edge;
layout(location = 0) out vec4 out_color;

const vec3 color = vec3(0.5, 0.5, 0.5);

void main() {
    out_color.rgb = color;
    out_color.a = smoothstep(1.0f - fwidth(edge) * 2, 1.0f, edge);
}
