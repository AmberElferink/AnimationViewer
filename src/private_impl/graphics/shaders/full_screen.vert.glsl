#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec2 screen_coordinates;
layout(location = 0) out vec2 texture_coordinates;

void main() {
  gl_Position = vec4(
    mix(1.0f, -1.0f, screen_coordinates.x),
    mix(1.0f, -1.0f, screen_coordinates.y),
    0.0,
    1.0);
  texture_coordinates = screen_coordinates;
}
