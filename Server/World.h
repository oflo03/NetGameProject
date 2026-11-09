#pragma once
#include <array>
#include <cstdint>
#include <vector>
#include "Protocol.h"
#include "TileMap.h"

class ServerPlayer;
class ServerEnemy;
class Bullet;
class ServerBoss;
struct ServerDrop;

// 게임 로직 스레드만 접근하는 월드
class World
{
private:
	// 엔티티 정의와 생성·해제는 후속 일정에서 추가
	std::array<ServerPlayer*, MAX_PLAYERS> players{};	// 인덱스는 playerId
	std::vector<ServerEnemy*> enemies;
	std::vector<Bullet*> bullets;
	std::vector<ServerDrop*> drops;
	ServerBoss* boss = nullptr;
	TileMap map;
	int stage = 0;
	int phase = 0;
	std::uint32_t tick = 0;

public:
	void Tick(float deltaTime);	// 경과 시간은 초 단위
	void UpdatePlayers(float deltaTime);
	Bullet* SpawnBullet(std::uint8_t type, std::uint8_t side, Vector2D<float> pos, Vector2D<float> dir);
	void UpdateBullets(float deltaTime);
	void ResolveCollisions();
	void SwordSlash(ServerPlayer& player);
	void DamagePlayer(ServerPlayer& player, int damage);
};
