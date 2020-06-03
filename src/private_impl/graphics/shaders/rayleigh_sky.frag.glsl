#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_GOOGLE_include_directive : require

#include "bridging_header.h"
#include "rayleigh.h"

layout(binding = 0, std140) uniform uniform_block_t
{
  sky_uniform_t data;
}
uniform_block;

layout(location = 0) in vec2 texture_coordinates;
layout(location = 0) out vec4 out_color;

void
main()
{
  // Cheat the alorithm by steching the sky down
  vec2 screen_coordinates = vec2(texture_coordinates.x, texture_coordinates.y);

  const vec2 aspect_ratio = vec2(float(uniform_block.data.width) / uniform_block.data.height, 1);
  vec4 point_cam = vec4((2.0 * screen_coordinates - 1.0) * aspect_ratio * 2.0f *
                          tan(uniform_block.data.camera_fov_y * 0.5f),
                        -1.0,
                        0.0f);

  vec4 direction = transpose(uniform_block.data.camera_rotation_matrix) * point_cam;
  // Grab translation from matrix by using augmented vector
  vec4 origin = uniform_block.data.camera_rotation_matrix * vec4(0, 0, 0, -1);
  ray_t ray = ray_t(origin.xyz + ground, normalize(direction.xyz));

  out_color = vec4(compute_incident_light(ray, uniform_block.data.direction_to_sun), 1);
}
