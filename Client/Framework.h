#pragma once
#include <vector>

struct PacketHeader;

class GameState {
public:
	GameState() { };
	virtual ~GameState() = default;
	virtual void update() = 0;
	virtual void handle_events() = 0;
	virtual void draw() = 0;
	virtual void process_packet(const PacketHeader* packet);
};

extern std::vector<GameState*> stateStack;

void change_state(GameState* state);
void push_state(GameState* state);
void pop_state();
void start_game(GameState* state);
void end_game();
