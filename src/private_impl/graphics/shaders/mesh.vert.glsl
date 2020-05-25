#version 450 core
#extension GL_ARB_separate_shader_objects : enable
#extension GL_GOOGLE_include_directive : require

#include "bridging_header.h"

layout(binding = 0, std140) uniform uniform_vertex_block_t {
  mesh_uniform_t data;
} uniform_block;

layout(location = 0) in vec3 vertex_position;
layout(location = 1) in vec3 vertex_normal;
layout(location = 2) in float vertex_bone_id; //uint won't upload correctly

layout(location = 0) out vec3 fragment_position;
layout(location = 1) out vec3 fragment_normal;

void main() {
    mat4 view_projection_matrix = uniform_block.data.projection_matrix * uniform_block.data.view_matrix;
  // Augment vertex_position with 1 in the w parameter indicating that it is a
  // point and can be translated.
  // Transform the position of the vertex with the inverse camera view to move
  // the object relative to the camera if it were at the origin.
  // Combine with its projection matrix, which will transform a point in
  // infinite R3 to Normalized Device coordinates. A box from -1 to 1 in three
  // axis where the xy coordinates are perpective projected (parallel lines
  // converge at a point) and the z gets fed into the depth-buffer.
  vec4 trans_rot_vertex_pos = uniform_block.data.bone_trans_rots[uint(vertex_bone_id)] * vec4(vertex_position, 1);
  gl_Position = view_projection_matrix * trans_rot_vertex_pos;
  gl_Position.x = vertex_bone_id;
  // We also store the position unaffected by perpective to do lighting calculations
  fragment_position = (uniform_block.data.view_matrix * trans_rot_vertex_pos).xyz;
  // Simularly, the normal which is a point, gets augmented with 0 in the w
  // parameter indicating that it cannot be translated.
  // Normals aren't perspective transformed which is why only the inverse
  // view matrix is used.
  fragment_normal = normalize((uniform_block.data.view_matrix * vec4(vertex_normal, 0)).xyz);
}
