#include <cstdlib>

#include "game.h"

int
main(int argc, char* argv[])
{

  auto game = AnimationViewer::Game::create("Animation Viewer", 800, 600);
  game->run();

  return EXIT_SUCCESS;
}
