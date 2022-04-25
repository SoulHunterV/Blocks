#include <iostream>

#include "config.h"

#include "game.hpp"

const unsigned int SCR_WIDTH = 1280;
const unsigned int SCR_HEIGHT = 720;


int main()
{
  std::cout << "Blocks application" << std::endl;
  std::cout << "Version " << Blocks_VERSION_MAJOR << "." << Blocks_VERSION_MINOR << std::endl;

  Game game(SCR_WIDTH, SCR_HEIGHT);

  return game.Run();
}
