#ifndef RAYLEIGH_H
#define RAYLEIGH_H

// https://www.scratchapixel.com/lessons/procedural-generation-virtual-worlds/simulating-sky/simulating-colors-of-the-sky
// Rayleigh scattering: The scattering of light by air molecules, responsible
// for the blue color of the sky during the day and the red orange color at
// sunrise and sunset.
// Mie scattering: The scattering of light by aerosols (dust or sand),
// responsible for white-gray haze seen in smog.

const vec3 rayleigh_coefficients = vec3(5.5e-6f, 13.0e-6f, 22.4e-6f);
const float mie_coefficient = 21e-6f;
// Scale height is the altitude by which the density of the atmosphere decreases by a factor of e.
const float scale_height_rayleigh = 7994.0f;
const float scale_height_mie = 1200.0f;
const float planet_radius_m = 6.36e6f; // 636000 km
const float atmosphere_radius_m = 6.42e6f;  // 642000 km
const float atmosphere_radius_2 = atmosphere_radius_m * atmosphere_radius_m;
const float sun_intensity = 20;
const int ray_march_steps_eye = 16;
const int ray_march_steps_light = 8;
const vec3 ground = vec3 (0, planet_radius_m, 0);

float rayleigh_phase_func(float cos)
{
  const float factor = 3.0f / (16.0f * M_PI);
  return factor * cos * cos + factor;
}

float schlick_phase_func(float cos)
{
  const float medium_anisotropy = 0.76f;
  const float k = 1.55f * medium_anisotropy - 0.55f * (medium_anisotropy * medium_anisotropy * medium_anisotropy);
  const float k2 = k * k;
  const float factor = (1.0f - k2) / (4.0f * M_PI);
  float denom = 1.0f + k * cos;
  float denom_2 = denom * denom;

  return factor / denom_2;
}

struct ray_t
{
  vec3 origin;
  vec3 direction;
};

/// Simplified version of sphere hit which assumes it always hits
float atmosphere_hit(ray_t ray)
{
  float a = dot(ray.direction, ray.direction);
  float b = dot(ray.origin, ray.direction);
  float c = dot(ray.origin, ray.origin) - atmosphere_radius_2;
  float discriminant = b * b - a * c;
  float det_sqrt = sqrt(discriminant);
  if (-b < det_sqrt) {
    return (-b + det_sqrt) / a;
  } else {
    return (-b - det_sqrt) / a;
  }
}

vec3 compute_incident_light(const ray_t ray_eye, const vec3 direction_to_sun)
{
  float t = atmosphere_hit(ray_eye);
  // March eye ray
  float ray_march_step_size_eye = t / ray_march_steps_eye;
  vec3 rayleigh = vec3(0, 0, 0);
  vec3 mie = vec3(0, 0, 0);
  float total_optical_depth_rayleigh_eye = 0;
  float total_optical_depth_mie_eye = 0;
  float t_eye = 0;
  for (uint i = 0; i < ray_march_steps_eye; ++i) {
    const vec3 sample_position_eye = ray_eye.origin + ray_eye.direction * (t_eye + ray_march_step_size_eye * 0.5f);
    const float height_eye = length(sample_position_eye) - planet_radius_m;

    const float step_optical_depth_rayleigh = exp(-height_eye / scale_height_rayleigh) * ray_march_step_size_eye;
    const float step_optical_depth_mie = exp(-height_eye / scale_height_mie) * ray_march_step_size_eye;
    total_optical_depth_rayleigh_eye += step_optical_depth_rayleigh;
    total_optical_depth_mie_eye += step_optical_depth_mie;

    const ray_t ray_light = ray_t(sample_position_eye, direction_to_sun);
    float t = atmosphere_hit(ray_light);

    // March light ray
    bool light_landed = false;
    const float ray_march_step_size_light = t / ray_march_steps_light;
    float t_light = 0;
    float total_optical_depth_rayleigh_light = 0;
    float total_optical_depth_mie_light = 0;
    for (uint j = 0; j < ray_march_steps_light; ++j) {
      const vec3 sample_position_light = ray_light.origin + ray_light.direction * (t_light + ray_march_step_size_light * 0.5f);
      const float height_light = length(sample_position_light) - planet_radius_m;
      if (height_light < 0) {
        light_landed = true;
        break;
      }

      total_optical_depth_rayleigh_eye += exp(-height_light / scale_height_rayleigh) * ray_march_step_size_light;
      total_optical_depth_mie_eye += exp(-height_light / scale_height_mie) * ray_march_step_size_light;
      t_light += ray_march_step_size_light;
    }

    if (!light_landed) {
      vec3 tau = rayleigh_coefficients * (total_optical_depth_rayleigh_eye + total_optical_depth_rayleigh_light)
        + 1.1f * mie_coefficient * (total_optical_depth_mie_eye + total_optical_depth_mie_light);
      vec3 attenuation = exp(-tau);
      rayleigh += attenuation * step_optical_depth_rayleigh;
      mie += attenuation * step_optical_depth_mie;
    }

    // March eye ray forward
    t_eye += ray_march_step_size_eye;
  }

  const float cos = dot(ray_eye.direction, direction_to_sun);
  const float phase_rayleigh = rayleigh_phase_func(cos);
  const float phase_mie = schlick_phase_func(cos);

  vec3 color = sun_intensity * (rayleigh * rayleigh_coefficients * phase_rayleigh + mie * mie_coefficient * phase_mie);

  if (ray_eye.direction.y < 0) {
    return vec3(0.1, 0.1, 0.1) + color;
  }
  return color;
}

#endif // RAYLEIGH_H
