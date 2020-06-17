#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_GOOGLE_include_directive : require

#include "bridging_header.h"
#include "rayleigh.h"

layout(binding = 1, std140) uniform uniform_fragment_block_t {
  mesh_uniform_t data;
} uniform_block;

layout(location = 0) in vec3 fragment_position;
layout(location = 1) in vec3 fragment_normal;
layout(location = 0) out vec4 out_color;

const vec3 ambient_color = vec3(0.3, 0.3, 0.3);
const vec3 diffuse_attenuation = vec3(0.9, 0.9, 0.9);
const vec3 specular_attenuation = vec3(0.3, 0.3, 0.3);

void main()
{
  vec3 ambient = ambient_color;
  vec3 diffuse = diffuse_attenuation * max(dot(fragment_normal, uniform_block.data.direction_to_sun.xyz), 0.0);
  vec3 diffuseTwoSides = diffuse + diffuse_attenuation * max(dot(fragment_normal, vec3(0, 0, -1.0)), 0.0);
  vec3 eye_to_point = normalize(fragment_position);
  ray_t ray = ray_t(fragment_position + ground, reflect(eye_to_point, fragment_normal));
 /// vec3 specular = specular_attenuation * compute_incident_light(ray, uniform_block.data.direction_to_sun.xyz);
  out_color = vec4(ambient + diffuseTwoSides, 1); //+ specular
}
