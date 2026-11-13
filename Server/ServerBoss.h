#pragma once
#include <array>
#include <vector>
#include "Protocol.h"
#include "Vector2D.h"

class World;

// [S11] 게임 로직 스레드에서 관리하는 보스 FSM 상태
class ServerBoss
{
public:
	bool active = false;
	std::int16_t hp = 0;
	Vector2D<float> pos;
	std::array<Vector2D<float>, 2> handPos{};
	std::array<std::uint8_t, 2> handType{};	// 원작 HAND 순서 (왼손, 오른손)
	float radius = 0;
	int targetPlayerId = -1;
	BossPattern pattern = BOSS_IDLE;
	std::vector<BossPattern> remainingPatterns;	// 뒤에서부터 꺼내는 패턴 순서
	float patternTime = 0;		// 경과·대기 시간은 초 단위
	float attackCooldown = 0;
	bool attackStarted = false;
	Vector2D<float> attackPos;	// 조준·낙뢰 위치

	void Update(float deltaTime, World& world);
};
