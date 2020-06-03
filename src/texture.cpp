#include "texture.h"

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
#include <tiny_gltf.h>

using AnimationViewer::Texture;

std::unique_ptr<Texture>
Texture::load_from_file(const std::string& filename)
{
  int width, height, channels;
  float* data = stbi_loadf(filename.c_str(), &width, &height, &channels, 0);
  if (data == nullptr) {
    std::printf("Unable to load \"%s\" texture, make sure it is in the current path\n",
                filename.c_str());
    return nullptr;
  }
  std::vector<glm::vec3> color;
  color.resize(width * height);
  for (uint32_t y = 0; y < static_cast<uint32_t>(height); ++y) {
    for (uint32_t x = 0; x < static_cast<uint32_t>(width); ++x) {
      color[x + y * width].x = data[(x + y * width) * channels];
      if (channels > 2) {
        color[x + y * width].y = data[(x + y * width) * channels + 1];
        color[x + y * width].z = data[(x + y * width) * channels + 2];
      } else {

        color[x + y * width].y = data[(x + y * width) * channels];
        color[x + y * width].z = data[(x + y * width) * channels];
      }
    }
  }
  stbi_image_free(data);
  return std::unique_ptr<Texture>(new Texture(width, height, std::move(color)));
}

std::unique_ptr<Texture>
Texture::load_from_gltf_image(const tinygltf::Image& image)
{
  // FIXME: Assuming 8 bit pixels data
  assert(image.bits == 8);
  std::vector<glm::vec3> color;
  color.resize(image.width * image.height);
  for (uint32_t y = 0; y < static_cast<uint32_t>(image.height); ++y) {
    for (uint32_t x = 0; x < static_cast<uint32_t>(image.width); ++x) {
      color[x + y * image.width].x = image.image[(x + y * image.width) * image.component] / 255.0f;
      if (image.component > 2) {
        color[x + y * image.width].y =
          image.image[(x + y * image.width) * image.component + 1] / 255.0f;
        color[x + y * image.width].z =
          image.image[(x + y * image.width) * image.component + 2] / 255.0f;
      } else {

        color[x + y * image.width].y =
          image.image[(x + y * image.width) * image.component] / 255.0f;
        color[x + y * image.width].z =
          image.image[(x + y * image.width) * image.component] / 255.0f;
      }
    }
  }
  return std::unique_ptr<Texture>(new Texture(image.width, image.height, std::move(color)));
}

Texture::Texture(uint32_t width, uint32_t height, std::vector<glm::vec3>&& data)
  : width_(width)
  , height_(height)
  , data_(data)
{}

Texture::~Texture() = default;

const glm::vec3&
Texture::sample(const glm::vec3& texture_coordinates) const
{
  uint32_t x = static_cast<uint32_t>(texture_coordinates.x * (width_ - 0.0001)) % width_;
  uint32_t y =
    static_cast<uint32_t>((1.0f - fmod(texture_coordinates.y, 1.0f)) * (height_ - 0.0001)) %
    height_;
  return data_[(x + y * width_)];
}
