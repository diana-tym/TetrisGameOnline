#include <vector>
#include "client.h"

#pragma comment(lib, "Ws2_32.lib")

Client::Client() {}

//Client::Client(const std::string& ip, int port, const std::string& name)
//    : serverIP(ip), serverPort(port), playerName(name) {}

void Client::setServerIP(const std::string& ip) {
    serverIP = ip;
}

void Client::setServerPort(int port) {
    serverPort = port;
}

void Client::setPlayerName(const std::string& name) {
    playerName = name;
}

std::string Client::getPlayerName() {
    return playerName;
}

bool Client::connectToServer() {
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        std::cerr << "WSAStartup failed." << std::endl;
        return false;
    }

    clientSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (clientSocket == INVALID_SOCKET) {
        std::cerr << "Socket creation failed." << std::endl;
        WSACleanup();
        return false;
    }

    sockaddr_in serverAddr = { 0 };
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(serverPort);
    InetPton(AF_INET, L"147.250.224.189", &serverAddr.sin_addr.s_addr);

    if (connect(clientSocket, (sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
        std::cerr << "Failed to connect to server." << std::endl;
        closesocket(clientSocket);
        WSACleanup();
        return false;
    }

    send(clientSocket, playerName.c_str(), playerName.size() + 1, 0);
    return true;
}

json Client::receiveMessage() {

    char buffer[2048];
    int bytesReceived = recv(clientSocket, buffer, sizeof(buffer) - 1, 0);
    if (bytesReceived == SOCKET_ERROR) {
        std::cerr << "Error recv(): " << WSAGetLastError() << std::endl;
    }
    else if (bytesReceived == 0) {
        std::cout << "Connection closed" << std::endl;
    }
    else if (bytesReceived > 0) {
            buffer[bytesReceived] = '\0';
            json message = json::parse(buffer);
            return json::parse(buffer);
        }
}

void Client::setGameOverCallback(GameOverCallback callback) {
    gameOverCallback = callback;
}

void Client::setGameStartCallback(GameStartCallback callback) {
    gameStartCallback = callback;
}

void Client::handleServerMessage(const json& message, std::vector<Player>& players) {
    std::string type = message["type"].get<std::string>();

    if (type == "state") {

        for (auto& player : players) {
            if (player.name == message["name"]) {
                GameState received_state = message["state"].get<GameState>();
                player.gameState.score = received_state.score;
                player.gameState.grid = received_state.grid;
                break;
            }
        }
    }
    else if (type == "namesAll") {
        std::string playerName;
        std::cout << "Players in the game: ";
        for (const auto& name : message["names"]) {
            playerName = name.get<std::string>();
            std::cout << playerName << " ";
            Player newPlayer = { playerName, {{{0}}, 0} };
            players.push_back(newPlayer);
            std::cout << "Added: " << newPlayer.name << std::endl;
        }

        std::cout << std::endl;

        if (gameStartCallback) {
            gameStartCallback();
        }

    }
    else if (type == "gameOver") {
        std::string winnerName = message["winner"];
        int score = message["score"];

        if (gameOverCallback) {
            gameOverCallback(winnerName, score);
        }
    }
}

void Client::run(std::vector<Player>& players) {
    std::cout << "Client is running. Waiting for server messages..." << std::endl;

    while (true) {
        json serverMessage = receiveMessage();
        handleServerMessage(serverMessage, players);
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
}

bool Client::sendMessage(const json& message) {
    std::string serializedMessage = message.dump() + "\n";
    int result = send(clientSocket, serializedMessage.c_str(), serializedMessage.size(), 0);
    return result != SOCKET_ERROR;
}
