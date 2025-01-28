#pragma once
#include <array>
#include <nlohmann/json.hpp>


using json = nlohmann::json;

struct GameState {
	std::array<std::array<int, 10>, 20> grid;
	int score;
	
};

inline void to_json(json& j, const GameState& state) {
	j = json{ {"grid", state.grid}, {"score", state.score} };
}

inline void from_json(const json& j, GameState& state) {
	j.at("grid").get_to(state.grid);
	j.at("score").get_to(state.score);
}
