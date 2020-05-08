#pragma once

#include <memory>
#include <string>
#include <vector>

#include <glm/vec3.hpp>

namespace tinygltf {
struct Image;
}

namespace AnimationViewer {
class Texture
{
public:
  static std::unique_ptr<Texture> load_from_file(const std::string& filename);
  static std::unique_ptr<Texture> load_from_gltf_image(
    const tinygltf::Image& image);
  virtual ~Texture();

  const glm::vec3& sample(const glm::vec3& texture_coordinates) const;

private:
  Texture(uint32_t width, uint32_t height, std::vector<glm::vec3>&& data);

  const uint32_t width;
  const uint32_t height;
  const std::vector<glm::vec3> data;
};
} // namespace AnimationViewer
