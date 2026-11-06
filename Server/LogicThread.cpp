#include "LogicThread.h"
#include <chrono>
#include <thread>
#include "World.h"

void LogicThreadMain()
{
	constexpr auto tickInterval = std::chrono::milliseconds(33);
	std::unique_ptr<World> world;

	while (serverRunning) {
		const auto nextTick = std::chrono::steady_clock::now() + tickInterval;
		// [S9] LogicThreadMain: room.lock을 잡고 시작 요청 확인, 게임 시작 정보 복사
		// [S9] StartGame: 시작 요청이 있으면 월드 생성, 게임 시작 패킷 전송, 스테이지 1 로드
		// [S9] World::Tick: 게임 중 33ms마다 한 틱 실행, 소켓 대신 입력 버퍼·송신 큐 사용
		std::this_thread::sleep_until(nextTick);
	}
}

std::unique_ptr<World> StartGame()
{
	// [S9] StartGame: 월드 생성, 게임 시작 패킷 전송, 스테이지 1 로드
	return nullptr;
}
