#include <winsock2.h>
#include <ws2tcpip.h>
#include <iostream>
#include <vector>
#include <nlohmann/json.hpp>
#include <chrono>
#include <future>
#include <thread>

using json = nlohmann::json;

#define BUFFER_SIZE 2048

const int MAX_PLAYERS = 2;
const int PORT = 5400;
bool gameStarted = false;

struct GameState {
    std::array<std::array<int, 10>, 20> grid;
    int score;
};

struct Player {
    int id;
    std::string name;
    SOCKET socket;
    bool connected;
    bool inPlay;
    GameState gameState;
};


void to_json(json& j, const GameState& state) {
    j = json{ {"grid", state.grid}, {"score", state.score} };
}


void from_json(const json& j, GameState& state) {
    j.at("grid").get_to(state.grid);
    j.at("score").get_to(state.score);
}

void sendAll(const std::vector<Player>& players, const std::string& message) {
    for (const auto& player : players) {
        if (player.connected) {
            std::cout << "send to " << player.name << " message\n";
            int result = send(player.socket, message.c_str(), message.size(), 0);
            if (result == SOCKET_ERROR) {
                std::cerr << "Sent error: " << WSAGetLastError() << std::endl;
            }
        }
    }
}

bool isGameOver(const std::vector<Player>& players) {
    for (const auto& player : players) {
        if (player.inPlay)
            return false;
    }
    return true;
}

void handleMessage(const std::string& message, Player& player, std::vector<Player>& players) {
    json data;
    try {
        data = json::parse(message);
    }
    catch (const json::parse_error& e) {
        std::cerr << "JSON parse error: " << e.what() << std::endl;
    }

    std::string type = data["type"].get<std::string>();

    if (type == "state") {

        player.gameState = data["state"].get<GameState>();
        //std::cout << "Updated state for player " << player.name << " Score " << player.gameState.score << std::endl;
        //for (int row = 0; row < 20; row++)
        //{
        //    for (int column = 0; column < 10; column++)
        //    {
        //        std::cout << player.gameState.grid[row][column] << " ";
        //    }
        //    std::cout << std::endl;
        //}
        
        json message;
        message["type"] = "state";
        message["name"] = player.name;
        message["state"] = player.gameState;

        std::string serializedMessage = message.dump();
        sendAll(players, serializedMessage);
        std::cout << "Send to client player state: " << player.name << std::endl;
    }

    else if (type == "finished") {
        player.inPlay = false;
        std::cout << "Player " << player.name << " finished their game." << std::endl;

        // check if game is finished
        if (isGameOver(players)) {
            // find winer and send
            auto winner = std::max_element(players.begin(), players.end(), [](const Player& a, const Player& b) {
                return a.gameState.score < b.gameState.score;
                });

            json message = { {"type", "gameOver"},
                             {"winner", winner->name},
                             {"score",  winner->gameState.score} };

            std::string serializedMessage = message.dump();
            sendAll(players, serializedMessage);
            std::cout << "All players finished the game." << std::endl;
        }
    }
}


void sendNames(const std::vector<Player>& players) {
    json message;
    message["type"] = "namesAll";

    json names = json::array();
    for (const auto& player : players) {
        if (player.connected) {
            names.push_back(player.name);
        }
    }
    message["names"] = names;
    std::string serializedMessage = message.dump();
    std::cout << "Send names: " << serializedMessage << std::endl;
    sendAll(players, serializedMessage);
}


int main() {
    std::cout << "========= SERVER ==========" << std::endl;

    WSADATA wsaData;
    int result = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (result != 0) {
        std::cerr << "WSAStartup failed: " << result << std::endl;
        return 1;
    }

    std::cout << "Winsock initialized successfully!" << std::endl;

    SOCKET serverSocket = socket(AF_INET, SOCK_STREAM, 0);

    sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(PORT);
    InetPton(AF_INET, L"147.250.224.189", &serverAddr.sin_addr.s_addr);

    if (bind(serverSocket, (sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
        std::cout << "bind failed" << std::endl;
        WSACleanup();
        return 0;
    }

    listen(serverSocket, MAX_PLAYERS);
    std::cout << "Server is running on port " << PORT << std::endl;

    std::vector<Player> players;
    int playerIdCounter = 1;

    while (!gameStarted) {
        sockaddr_in clientAddr;
        int clientSize = sizeof(clientAddr);
        SOCKET clientSocket = accept(serverSocket, (sockaddr*)&clientAddr, &clientSize);

        if (clientSocket != INVALID_SOCKET) {
            std::cout << "New player connected: " << clientSocket << "\n";

            Player newPlayer = { playerIdCounter++, "", clientSocket, true, true, {{{0}}, 0} };

            char nameBuffer[BUFFER_SIZE];
            int bytesReceived = recv(clientSocket, nameBuffer, sizeof(nameBuffer) - 1, 0);
            if (bytesReceived > 0) {
                nameBuffer[bytesReceived] = '\0';
                newPlayer.name = std::string(nameBuffer);
                std::cout << "Player name: " << newPlayer.name << std::endl;
            }
            players.push_back(newPlayer);
        }

        if (players.size() >= MAX_PLAYERS && !gameStarted) {
            std::cout << "Maximum players reached. Starting the game!" << std::endl;
            gameStarted = true;
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(100));

    }
    std::cout << "Game is starting with " << players.size() << " players!" << std::endl;
    sendNames(players);

    while(true) {
            
        for (auto& player : players) {
            if (player.inPlay) {
                char buffer[BUFFER_SIZE];
                int bytesReceived = recv(player.socket, buffer, sizeof(buffer) - 1, 0);

                if (bytesReceived > 0) { 
                    buffer[bytesReceived] = '\0';

                    std::string receivedMessage(buffer);

                    size_t pos;
                    while ((pos = receivedMessage.find('\n')) != std::string::npos) {
                        std::string message = receivedMessage.substr(0, pos);
                        receivedMessage.erase(0, pos + 1);
                        std::cout << "RECEIVED MESSAGE: " << message << std::endl;

                        handleMessage(message, player, players);
                    }
                }
            }

        }
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    for (const auto& player : players) {
        if (player.connected) {
            closesocket(player.socket);
        }
    }

    closesocket(serverSocket);
    WSACleanup();

    return 0;
}