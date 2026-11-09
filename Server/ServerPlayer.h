#pragma once
#include <array>
#include <cstdint>
#include "Protocol.h"
#include "Vector2D.h"

struct InputState;
class World;

// [S10] 게임 로직 스레드에서 관리하는 플레이어 상태
class ServerPlayer
{
public:
	bool used = false;
	std::uint8_t playerId = 0;
	std::uint8_t charType = 0;	// playertype
	std::uint8_t state = 0;		// STATE
	std::int16_t hp = 0;
	Vector2D<float> pos;
	Vector2D<float> lastPos;
	Vector2D<float> size;		// 충돌 박스 절반 크기
	Vector2D<float> aimPos;
	Vector2D<float> rollDir;
	float angle = 0;
	float moveSpeedPerSecond = 0;
	std::uint8_t weapon = 0;		// ID
	std::array<std::uint8_t, WEAPON_COUNT> ammo{};
	float fireCooldown = 0;		// 남은 시간은 초 단위
	float rollTime = 0;
	float invulnerableTime = 0;
	bool interactRequested = false;

	void ApplyInput(const InputState& input, float deltaTime, World& world);
	bool TryFire(World& world);
};
