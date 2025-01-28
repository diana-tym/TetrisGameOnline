#include "game.h"
#include <random>
#include <chrono>

Game::Game()
{
    grid = Grid();
    blocks = GetAllBlocks();
    currentBlock = GetRandomBlock();
    nextBlock = GetRandomBlock();
    gameOver = false;

    isOnline = false;
    isStateChanged = false;
    finishedAll = false;
    isGameStarted = false;

    score = 0;
    InitAudioDevice();
    music = LoadMusicStream("D:/ENSTA Projets/IN204/resources/music.mp3");
    PlayMusicStream(music);
    rotateSound = LoadSound("D:/ENSTA Projets/IN204/resources/rotate.mp3");
    clearSound = LoadSound("D:/ENSTA Projets/IN204/resources/clear.mp3");
    failSound = LoadSound("D:/ENSTA Projets/IN204/resources/fail.mp3");
}

Game::~Game()
{
    UnloadSound(rotateSound);
    UnloadSound(clearSound);
    UnloadSound(failSound);
    UnloadMusicStream(music);
    CloseAudioDevice();
}


void Game::onGameOver(const std::string& winnerName, int score) {
    finishedAll = true;
    winner = winnerName;
    finalScore = score;
}

void Game::onGameStart() {
    isGameStarted = true;
}

Block Game::GetRandomBlock()
{
    if (blocks.empty())
    {
        blocks = GetAllBlocks();
    }
    int randomIndex = rand() % blocks.size();
    Block block = blocks[randomIndex];
    blocks.erase(blocks.begin() + randomIndex);
    return block;
}

std::vector<Block> Game::GetAllBlocks()
{
    return { IBlock(), JBlock(), LBlock(), OBlock(), SBlock(), TBlock(), ZBlock() };
}

void Game::Draw()
{
    grid.Draw();
    currentBlock.Draw(11, 11);
    switch (nextBlock.id)
    {
    case 1:
        nextBlock.Draw(275, 290);
        break;
    case 2:
        nextBlock.Draw(275, 270);
        break;
    default:
        nextBlock.Draw(290, 270);
        break;
    }
}

void Game::HandleInput()
{
    int keyPressed = GetKeyPressed();

    if (gameOver && keyPressed != 0 && !isOnline)
    {
        gameOver = false;
        Reset();
    }

    switch (keyPressed)
    {
    case KEY_LEFT:
        MoveBlockLeft();
        break;
    case KEY_RIGHT:
        MoveBlockRight();
        break;
    case KEY_DOWN:
        MoveBlockDown();
        break;
    case KEY_UP:
        RotateBlock();
        break;
    };
}

void Game::MoveBlockLeft()
{
    if (!gameOver)
    {
        currentBlock.Move(0, -1);
        if (IsBlockOutside() || BlockFits() == false)
        {
            currentBlock.Move(0, 1);
        }
    }
}

void Game::MoveBlockRight()
{
    if (!gameOver)
    {
        currentBlock.Move(0, 1);
        if (IsBlockOutside() || BlockFits() == false)
        {
            currentBlock.Move(0, -1);
        }
    }
}

void Game::MoveBlockDown()
{
    if (!gameOver)
    {
        currentBlock.Move(1, 0);
        if (IsBlockOutside() || BlockFits() == false)
        {
            currentBlock.Move(-1, 0);
            LockBlock();
        }
    }
}

bool Game::IsBlockOutside()
{
    std::vector<Position> tiles = currentBlock.GetCellPositions();
    for (Position item : tiles)
    {
        if (grid.IsCellOutside(item.row, item.column))
        {
            return true;
        }
    }
    return false;
}

void Game::RotateBlock()
{
    if (!gameOver)
    {
        currentBlock.Rotate();
        if (IsBlockOutside() || BlockFits() == false)
        {
            currentBlock.UndoRotation();
        }
        else
        {
            if (currentBlock.id != 4)
            {
                PlaySound(rotateSound);
            }
        }
    }
}

void Game::LockBlock()
{
    std::vector<Position> tiles = currentBlock.GetCellPositions();
    for (Position item : tiles)
    {
        grid.grid[item.row][item.column] = currentBlock.id;
        if (item.row == 0)
        {
            gameOver = true;
            StopMusicStream(music);
            PlaySound(failSound);
            isStateChanged = true;
            return;
        }
    }
    currentBlock = nextBlock;
    if (!BlockFits())
    {
        gameOver = true;
        StopMusicStream(music);
        PlaySound(failSound);
        isStateChanged = true;
        return;
    }
    nextBlock = GetRandomBlock();
    int rowsCleared = grid.ClearFullRows();
    if (rowsCleared > 0)
    {
        PlaySound(clearSound);
        UpdateScore(rowsCleared);
    }

    isStateChanged = true;

}

bool Game::BlockFits()
{
    std::vector<Position> tiles = currentBlock.GetCellPositions();
    for (Position item : tiles)
    {
        if (grid.IsCellEmpty(item.row, item.column) == false)
        {
            return false;
        }
    }
    return true;
}

void Game::Reset()
{
    grid.Initialize();
    blocks = GetAllBlocks();
    currentBlock = GetRandomBlock();
    nextBlock = GetRandomBlock();
    score = 0;
    gameOver = false;
    PlayMusicStream(music);
    StopSound(failSound);
}

void Game::UpdateScore(int linesCleared)
{
    switch (linesCleared)
    {
    case 1:
        score += 40;
        break;
    case 2:
        score += 100;
        break;
    case 3:
        score += 300;
        break;
    case 4:
        score += 1200;
        break;
    default:
        score += 40 * (linesCleared + 1);
        break;
    }
}

GameState Game::getGameState() {
    GameState state;

    for (int i = 0; i < 20; ++i) {
        for (int j = 0; j < 10; ++j) {
            state.grid[i][j] = grid.grid[i][j];
        }
    }
    state.score = score;
    return state;
}
