#include "LogicThread.h"
#include <chrono>
#include <thread>
#include "World.h"
#include "ClientThread.h"
#include "GameData.h"

void LogicThreadMain()
{
	// [S9] LogicThreadMain: 시작 대기 후 33ms 고정 틱 실행
	constexpr auto tickInterval = std::chrono::milliseconds(GameData::LogicIntervalMs);
	std::unique_ptr<World> world;
	auto nextTick = std::chrono::steady_clock::now();
	while (serverRunning) {
		if (!world)
			world = StartGame();
		else
			world->Tick(GameData::LogicIntervalMs / 1000.0f);
		nextTick += tickInterval;
		const auto now = std::chrono::steady_clock::now();
		// 짧은 지연은 따라잡고, 오래 멈췄을 때만 누적 지연을 버린다.
		if (now - nextTick >= tickInterval * 5)
			nextTick = now;
		std::this_thread::sleep_until(nextTick);
	}
}

std::unique_ptr<World> StartGame()
{
	// [S9] StartGame: 맵을 준비한 뒤 대기실 정보를 한 번 복사한다.
	{
		std::lock_guard<std::mutex> lock(room.lock);
		if (room.state != ROOM_LOBBY || !room.startRequested)
			return nullptr;
	}
	TileMap initialMap;
	if (!initialMap.Load(1)) {
		std::lock_guard<std::mutex> lock(room.lock);
		room.startRequested = false;
		return nullptr;
	}
	auto world = std::make_unique<World>();
	S2C_GAME_START_PACKET start{};
	start.size = sizeof(start);
	start.type = S2C_GAME_START;
	{
		std::lock_guard<std::mutex> lock(room.lock);
		if (room.state != ROOM_LOBBY || !room.startRequested)
			return nullptr;
		room.startRequested = false;
		for (const auto& session : room.sessions) {
			if (!session.used || !session.connected || session.playerId < 0)
				continue;
			const int id = session.playerId;
			if (id >= MAX_PLAYERS || session.charType > CHAR_KNIGHT ||
				session.ready != READY_YES || start.players[id].used)
				return nullptr;
			auto& player = start.players[id];
			player.used = 1;
			player.charType = session.charType;
			player.x = GameData::PlayerSpawns[0].x + id * GameData::PlayerSpawnSpacing;
			player.y = GameData::PlayerSpawns[0].y;
		}
		if (room.hostId < 0 || room.hostId >= MAX_PLAYERS || !start.players[room.hostId].used ||
			!world->Initialize(start, std::move(initialMap)))
			return nullptr;
		room.state = ROOM_IN_GAME;
		room.leftPlayers.clear();
	}
	Broadcast(&start, sizeof(start));
	S2C_STAGE_LOAD_PACKET stage{};
	stage.size = sizeof(stage);
	stage.type = S2C_STAGE_LOAD;
	stage.stage = 1;
	Broadcast(&stage, sizeof(stage));
	return world;
}

void EndGame(GameResult result)
{
	// [S12] EndGame: S2C_GAME_END 전송, room.lock으로 대기실 상태·레디 정리, 대기실 정보 전파
	// Broadcast와 BroadcastRoomInfo는 room.lock을 푼 뒤 호출한다
}
