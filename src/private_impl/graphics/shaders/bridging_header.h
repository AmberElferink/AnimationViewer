#ifndef BRIDGING_HEADER_H
#define BRIDGING_HEADER_H

/// This is a header which is read by both the gpu shader code and the cpu
/// c++ code.
/// The purpose of this header is to avoid double declarations.

#if __cplusplus
#define out
#define inout
#define REF &
using glm::vec4;
using glm::vec3;
using glm::mat4;

#else
#define uint32_t uint
#define alignas(x)
#define M_PI 3.14159265358979323846       /* pi */
#define M_PI_2 1.57079632679489661923     /* pi/2 */
#define M_PI_4 0.78539816339744830962     /* pi/4 */
#define M_1_PI 0.31830988618379067154     /* 1/pi */
#define M_1_2PI 0.15915494309189533576    /* 1/2pi */
#define M_2_PI 0.63661977236758134308     /* 2/pi */
#define M_2_SQRTPI 1.12837916709551257390 /* 2/sqrt(pi) */
#define M_SQRT2 1.41421356237309504880    /* sqrt(2) */
#define M_SQRT1_2 0.70710678118654752440  /* 1/sqrt(2) */

// From cmath
#ifndef _HUGE_ENUF
#define _HUGE_ENUF 1e+300 // _HUGE_ENUF*_HUGE_ENUF must overflow
#endif
#define INFINITY (float(_HUGE_ENUF * _HUGE_ENUF))
#define HUGE_VAL (double(INFINITY))
#define HUGE_VALF (float(INFINITY))
#define NAN (float(INFINITY * 0.0F))
#define FLT_EPSILON (float(5.96e-08))

#define static
#define REF

#endif

struct alignas(16) sky_uniform_t
{
  mat4 camera_rotation_matrix;
  vec3 direction_to_sun;
  float camera_fov_y;
  uint32_t width;
  uint32_t height;
};

struct alignas(16) mesh_uniform_t
{
  mat4 projection_matrix;
  mat4 view_matrix;
  mat4 model_matrix;
  vec4 direction_to_sun;
  mat4 bone_trans_rots[256];
  // storage buffer
};

struct alignas(16) joint_uniform_t
{
  mat4 vp;
  mat4 model;
  vec4 color;
  float node_size;
};

#endif // BRIDGING_HEADER_H
