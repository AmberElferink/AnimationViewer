#include "scene.h"

#include <string>

using namespace AnimationViewer;

std::unique_ptr<Scene>
Scene::create()
{
  return std::unique_ptr<Scene>(new Scene());
}

Scene::Scene() = default;
Scene::~Scene() = default;
