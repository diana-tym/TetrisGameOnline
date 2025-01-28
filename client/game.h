#pragma once
#include "grid.h"
#include "blocks.cpp"
#include "gameState.h"

class Game
{
public:
    Game();
    ~Game();
    void Draw();
    void HandleInput();
    void MoveBlockDown();
    void Reset();
    bool gameOver;
    int score;
    Music music;
    bool isStateChanged;
    GameState getGameState();
    bool isOnline;

    bool finishedAll;
    bool isGameStarted;
    std::string winner;
    int finalScore;
    void onGameOver(const std::string& winnerName, int score);
    void onGameStart();


private:
    void MoveBlockLeft();
    void MoveBlockRight();
    Block GetRandomBlock();
    std::vector<Block> GetAllBlocks();
    bool IsBlockOutside();
    void RotateBlock();
    void LockBlock();
    bool BlockFits();
    void UpdateScore(int linesCleared);


    Grid grid;
    std::vector<Block> blocks;
    Block currentBlock;
    Block nextBlock;
    Sound rotateSound;
    Sound clearSound;
    Sound failSound;
};