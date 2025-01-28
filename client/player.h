#pragma once
#include <iostream>
#include "gameState.h"
#include <mutex>

struct Player {
    std::string name;
    GameState gameState;
};
