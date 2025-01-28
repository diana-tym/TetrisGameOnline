#pragma once
#include <iostream>
#include "winsock_wrapper.h"
#include <nlohmann/json.hpp>
#include "player.h"
#include <functional>

using json = nlohmann::json;

class Client {
public:
	Client();
	//Client(const std::string& ip, int port, const std::string& name);
	bool connectToServer();
	json receiveMessage();
	void handleServerMessage(const json& message, std::vector<Player>& players);
	void run(std::vector<Player>& players);
	void setServerIP(const std::string& ip);
	void setServerPort(int port);
	void setPlayerName(const std::string& name);
	std::string getPlayerName();
	bool sendMessage(const json& message);
	using GameOverCallback = std::function<void(const std::string& winnerName, int score)>;
	using GameStartCallback = std::function<void()>;
	void setGameOverCallback(GameOverCallback callback);
	void setGameStartCallback(GameStartCallback callback);

private:
	std::string serverIP;
	int serverPort;
	SOCKET clientSocket;
	std::string playerName;
	GameOverCallback gameOverCallback;
	GameStartCallback gameStartCallback;

};