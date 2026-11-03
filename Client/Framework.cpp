#include "Framework.h"

std::vector<GameState*> stateStack;

void GameState::process_packet(const PacketHeader* packet)
{
	// [C4] process_packet: 현재 화면에서 받은 패킷 처리
}

void change_state(GameState* state)
{
	if (!stateStack.empty()) {
		delete stateStack.back();
		stateStack.pop_back();
	}
	stateStack.push_back(state);
}

void push_state(GameState* state)
{
	stateStack.push_back(state);
}

void pop_state()
{
	if (!stateStack.empty()) {
		delete stateStack.back();
		stateStack.pop_back();
	}
}

void start_game(GameState* state)
{
	stateStack.push_back(state);
}

void end_game()
{
	while (!stateStack.empty()) {
		delete stateStack.back();
		stateStack.pop_back();
	}
}
