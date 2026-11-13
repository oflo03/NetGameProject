#pragma once
#include <atomic>
#include <memory>
#include "Protocol.h"

class World;

extern std::atomic<bool> serverRunning;	// main.cpp의 서버 종료 플래그

void LogicThreadMain();
std::unique_ptr<World> StartGame();
void EndGame(GameResult result);
