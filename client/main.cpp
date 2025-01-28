#include "client.h"
#include "raylib.h"
#include "game.h"
#include "colors.h"
#include <iostream>
#include "colors.h"
#include <thread>

double lastUpdateTime = 0;

enum WindowState {
    MAIN_MENU,
    SINGLE_PLAYER,
    ONLINE_GAME,
    SETTINGS,
    EXIT_GAME
};

bool EventTriggered(double interval) {
    double currentTime = GetTime();
    if (currentTime - lastUpdateTime >= interval) {
        lastUpdateTime = currentTime;
        return true;
    }
    return false;
}

void DrawOtherPlayers(const std::vector<Player>& players, Font font, Client& client) {
    const int smallGridSize = 15;
    int startX = 600;
    int startY = 30;
    int gridSpacing = 10;
    int spacePlayers = 160;

    std::vector<Color> colors = GetCellColors();
    for (size_t i = 0; i < players.size(); i++) {
        if (players[i].name == client.getPlayerName()) continue;

        DrawText(players[i].name.c_str(), startX, startY - 20, 10, WHITE);

        for (int row = 0; row < players[i].gameState.grid.size(); row++) {
            for (int col = 0; col < players[i].gameState.grid[i].size(); col++) {

                int cellValue = players[i].gameState.grid[row][col];
                DrawRectangle(startX + col * smallGridSize,
                    startY + row * smallGridSize + 11,
                    smallGridSize - 1, smallGridSize - 1, colors[cellValue]);
            }
        }

        char scoreText[20];
        sprintf_s(scoreText, "Score: %d", players[i].gameState.score);
        DrawText(scoreText, startX, startY - 10, 10, WHITE);
    }
}

void sendState(Game& game, Client client) {

    if (game.isStateChanged) {
        GameState newGameState = game.getGameState();
        json message;
        message["type"] = "state";
        message["state"] = newGameState;
        client.sendMessage(message);
        game.isStateChanged = false;
    }
}

void sendGameOver(Game& game, Client client, bool& sentGameOver) {

    if (game.gameOver && !sentGameOver) {
        json message;
        message["type"] = "finished";
        bool res = client.sendMessage(message);
        sentGameOver = true;
    }
}

int main() {
    const int screenWidth = 540;
    const int screenHeight = 620;
    SetTraceLogLevel(LOG_ERROR);
    InitWindow(screenWidth, screenHeight, "Jeu de Tetris");
    SetTargetFPS(60);

    WindowState windowState = MAIN_MENU;
    Font font = LoadFontEx("D:/ENSTA Projets/IN204/resources/Arial.ttf", 64, 0, 0);

    Game game = Game();
    float musicVolume = 1.0f;
    bool musicEnabled = true;
    SetMusicVolume(game.music, musicVolume);

    //----------- online
    const int maxInputChars = 30;
    char nameText[64] = "\0";
    int letterCount = 0;
    bool mouseOnText = false;
    Rectangle nameBox = { screenWidth / 2.0f - 100, 200, 200, 40 };
    bool textBoxActive = false;

    Rectangle button = { screenWidth / 2.0f - 50, 260, 100, 40 };
    bool buttonHovered = false;
    bool enterGame = false;
    bool sentGameOver = false;
    //-----------

    // Client
    std::vector<Player> players;
    bool isConnected = false;
    const int port = 5400;
    Client client;
    std::thread clientThread;


    client.setGameOverCallback([&game](const std::string& winnerName, int score) {
        game.onGameOver(winnerName, score);
        });

    client.setGameStartCallback([&game]() {
        game.onGameStart();
        });

    while (!WindowShouldClose()) {
        BeginDrawing();
        ClearBackground(darkBlue);

        switch (windowState) {
        case MAIN_MENU:
            DrawTextEx(font, "TETRIS", { 195, 100 }, 42, 2, WHITE);

            DrawRectangle(120, 200, 300, 50, lightBlue);
            DrawTextEx(font, "Joueur unique", { 155, 205 }, 38, 2, WHITE);

            DrawRectangle(120, 270, 300, 50, lightBlue);
            DrawTextEx(font, "Jeu en ligne", { 167, 275 }, 38, 2, WHITE);

            DrawRectangle(120, 340, 300, 50, lightBlue);
            DrawTextEx(font, "Parametres", { 175, 345 }, 38, 2, WHITE);

            DrawRectangle(120, 410, 300, 50, lightBlue);
            DrawTextEx(font, "Quitter", { 215, 415 }, 38, 2, WHITE);

            if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
                Vector2 mousePoint = GetMousePosition();
                if (CheckCollisionPointRec(mousePoint, { 120, 200, 300, 50 })) {
                    windowState = SINGLE_PLAYER;
                }
                else if (CheckCollisionPointRec(mousePoint, { 120, 270, 300, 50 })) {
                    windowState = ONLINE_GAME;
                }
                else if (CheckCollisionPointRec(mousePoint, { 120, 340, 300, 50 })) {
                    windowState = SETTINGS;
                }
                else if (CheckCollisionPointRec(mousePoint, { 120, 410, 300, 50 })) {
                    windowState = EXIT_GAME;
                }
            }
            break;

        case SINGLE_PLAYER:
            UpdateMusicStream(game.music);
            game.HandleInput();
            if (EventTriggered(0.2))
            {
                game.MoveBlockDown();
            }

            BeginDrawing();
            ClearBackground(darkBlue);

            // Text samples
            DrawTextEx(font, "Score", { 372, 15 }, 38, 2, WHITE);
            DrawTextEx(font, " Figure \nsuivante", { 358, 135 }, 38, 2, WHITE);
            if (game.gameOver)
            {
                DrawTextEx(font, "Jeu Termine!", { 317, 450 }, 38, 2, WHITE);
            }

            // Score panel
            DrawRectangleRounded({ 320, 55, 210, 60 }, 0.3, 6, lightBlue);
            char scoreText[10];
            sprintf_s(scoreText, sizeof(scoreText), "%d", game.score);
            DrawTextEx(font, scoreText, { 320 + (210 - MeasureTextEx(font, scoreText, 38, 2).x) / 2, 65 }, 38, 2, WHITE);

            // Figure panel
            DrawRectangleRounded({ 320, 215, 210, 180 }, 0.3, 6, lightBlue);
            game.Draw();

            if (IsKeyPressed(KEY_BACKSPACE)) {
                StopMusicStream(game.music);
                game.Reset();
                windowState = MAIN_MENU;
            }
            break;

        case SETTINGS:
            ClearBackground(darkBlue);
            DrawTextEx(font, "Parametres", { 175, 50 }, 38, 2, WHITE);

            DrawTextEx(font, "Volume de la musique", { 170, 150 }, 20, 2, WHITE);
            DrawRectangle(120, 180, 300, 20, darkBlue);
            DrawRectangle(120, 180, static_cast<int>(300 * musicVolume), 20, BLACK);

            DrawText(musicEnabled ? "Musique ON" : "Musique OFF", 180, 220, 20, WHITE);
            DrawRectangle(120, 250, 300, 40, LIGHTGRAY);
            DrawTextEx(font, "Toggle musique", { 200, 260 }, 20, 2, WHITE);
            if (IsMouseButtonDown(MOUSE_LEFT_BUTTON)) {
                Vector2 mousePoint = GetMousePosition();
                if (CheckCollisionPointRec(mousePoint, { 120, 180, 300, 20 })) {
                    musicVolume = (mousePoint.x - 120) / 300.0f;
                    if (musicVolume < 0.0f) musicVolume = 0.0f;
                    if (musicVolume > 1.0f) musicVolume = 1.0f;
                    SetMusicVolume(game.music, musicVolume);
                }
                else if (CheckCollisionPointRec(mousePoint, { 120, 250, 300, 40 })) {
                    musicEnabled = !musicEnabled;
                    if (musicEnabled) {
                        ResumeMusicStream(game.music);
                    }
                    else {
                        PauseMusicStream(game.music);
                    }
                }
            }

            DrawTextEx(font, "Pour revenir touche BACKSPACE", { 125, 500 }, 20, 2, WHITE);
            if (IsKeyPressed(KEY_BACKSPACE)) {
                windowState = MAIN_MENU;
            }
            break;


        case ONLINE_GAME: {

            if (CheckCollisionPointRec(GetMousePosition(), nameBox)) mouseOnText = true;
            else mouseOnText = false;

            if (mouseOnText)
            {
                SetMouseCursor(MOUSE_CURSOR_IBEAM);
                int key = GetCharPressed();
                while (key > 0)
                {
                    if ((key >= 32) && (key <= 125) && (letterCount < maxInputChars))
                    {
                        nameText[letterCount] = (char)key;
                        nameText[letterCount + 1] = '\0';
                        letterCount++;
                    }
                    key = GetCharPressed();
                }
                if (IsKeyPressed(KEY_BACKSPACE))
                {
                    letterCount--;
                    if (letterCount < 0) letterCount = 0;
                    nameText[letterCount] = '\0';
                }
            }
            else SetMouseCursor(MOUSE_CURSOR_DEFAULT);
            buttonHovered = CheckCollisionPointRec(GetMousePosition(), button);
            if (buttonHovered && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
                enterGame = true;
            }

            BeginDrawing();
            ClearBackground(darkBlue);

            if (!enterGame) {
                DrawText("Enter name:", screenWidth / 2 - MeasureText("Enter name:", 20) / 2, 150, 20, GRAY);
                DrawRectangleRec(nameBox, LIGHTGRAY);
                if (mouseOnText) DrawRectangleLinesEx(nameBox, 2, BLUE);
                else DrawRectangleLinesEx(nameBox, 2, DARKGRAY);
                DrawText(nameText, nameBox.x + 5, nameBox.y + 10, 20, MAROON);
                DrawRectangleRec(button, buttonHovered ? DARKGRAY : GRAY);
                DrawText("OK", button.x + 30, button.y + 10, 20, WHITE);
            }

            if (!isConnected && enterGame) {
                DrawText("Waiting for the server...", 150, 150, 20, GRAY);
                client.setServerIP("127.0.0.1");
                client.setServerPort(port);
                client.setPlayerName(nameText);

                isConnected = client.connectToServer();
                std::cout << "CONNECTED: " << isConnected << std::endl;
                if (!isConnected) {
                    ClearBackground(darkBlue);
                    DrawText("Connection failed", 150, 150, 20, RED);
                }
                else
                {
                    clientThread = std::thread(&Client::run, &client, std::ref(players));
                    SetWindowSize(800, screenHeight);
                }
            }
            if (game.isGameStarted) {
                ClearBackground(darkBlue);

                UpdateMusicStream(game.music);
                game.isOnline = true;
                game.HandleInput();
                if (EventTriggered(0.2))
                {
                    game.MoveBlockDown();
                }

                BeginDrawing();
                ClearBackground(darkBlue);

                DrawTextEx(font, "Score", { 372, 15 }, 38, 2, WHITE);
                DrawTextEx(font, " Figure \nsuivante", { 358, 135 }, 38, 2, WHITE);

                if (game.finishedAll)
                {
                    char winnerText[100];
                    sprintf_s(winnerText, sizeof(winnerText), "Winner: %s", game.winner.c_str());
                    DrawTextEx(font, winnerText, { 320 + (210 - MeasureTextEx(font, winnerText, 38, 2).x) / 2, 475 }, 38, 2, WHITE);

                    char finalScoreText[100];
                    sprintf_s(finalScoreText, sizeof(finalScoreText), "Score: %d", game.finalScore);
                    DrawTextEx(font, finalScoreText, { 320 + (210 - MeasureTextEx(font, finalScoreText, 38, 2).x) / 2, 400 }, 38, 2, WHITE);
                    DrawTextEx(font, "Jeu Termine!", { 320 + (210 - MeasureTextEx(font, "Jeu Termine!", 38, 2).x) / 2, 550 }, 38, 2, WHITE);
                }

                DrawRectangleRounded({ 320, 55, 210, 60 }, 0.3, 6, lightBlue);
                char scoreText[10];
                sprintf_s(scoreText, sizeof(scoreText), "%d", game.score);
                DrawTextEx(font, scoreText, { 320 + (210 - MeasureTextEx(font, scoreText, 38, 2).x) / 2, 65 }, 38, 2, WHITE);

                // Figure panel
                DrawRectangleRounded({ 320, 215, 210, 180 }, 0.3, 6, lightBlue);
                game.Draw();

                sendState(game, client);
                sendGameOver(game, client, sentGameOver);
                DrawOtherPlayers(players, font, client);

                if (IsKeyPressed(KEY_BACKSPACE)) {
                    SetWindowSize(screenWidth, screenHeight);
                    StopMusicStream(game.music);
                    game.Reset();
                    windowState = MAIN_MENU;
                }
            }
            break;
        }

        case EXIT_GAME:
            CloseWindow();
            return 0;
        }

        EndDrawing();
    }

    CloseWindow();
    return 0;
}
