#pragma once

#include <chrono>
#include <memory>

namespace AnimationViewer {
class Scene;
class Ui;

class Input
{
public:
  static std::unique_ptr<Input> create();

  virtual ~Input();
  void run(Ui& ui, Scene& scene, std::chrono::microseconds& dt);
  bool should_quit() const;

protected:
  Input();

private:
  bool quit_;
};
} // namespace AnimationViewer
