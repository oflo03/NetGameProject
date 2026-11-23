#pragma once
#include <array>
#include <cstdint>
#include <memory>
#include <vector>
#include "Protocol.h"
#include "TileMap.h"
#include "ServerPlayer.h"

class ServerPlayer;
class ServerEnemy;
class Bullet;
class ServerBoss;
struct ServerDrop;

// 게임 로직 스레드만 접근하는 월드
class World
{
private:
	std::array<ServerPlayer, MAX_PLAYERS> players{};	// 인덱스는 playerId
	std::vector<std::unique_ptr<ServerEnemy>> enemies;
	std::vector<std::unique_ptr<Bullet>> bullets;
	std::vector<ServerDrop*> drops;
	std::unique_ptr<ServerBoss> boss;
	TileMap map;
	int stage = 0;
	int phase = 0;
	std::uint32_t tick = 0;
	std::uint16_t nextBulletId = 0;
	bool portalOpen = false;
	Vector2D<float> portalPos;
	std::vector<std::vector<char>> events;	// 이번 틱의 Protocol.h 이벤트 패킷 바이트
	bool HitBulletTarget(const Bullet& bullet, Vector2D<float> pos, std::array<bool, MAX_PLAYERS>& hitPlayers);

public:
	World();
	~World();
	bool Initialize(const S2C_GAME_START_PACKET& start, TileMap initialMap);
	void Tick(float deltaTime);	// 경과 시간은 초 단위
	void UpdatePlayers(float deltaTime);
	Bullet* SpawnBullet(std::uint8_t type, std::uint8_t side, Vector2D<float> pos, Vector2D<float> dir);
	void UpdateBullets(float deltaTime);
	void ResolveCollisions();
	void SwordSlash(ServerPlayer& player);
	void DamagePlayer(ServerPlayer& player, int damage);
	void UpdateEnemies(float deltaTime);
	const ServerPlayer* FindNearestPlayer(const Vector2D<float>& pos) const;
	void DamageEnemy(ServerEnemy& enemy, int damage);
	void SpawnWave();
	void UpdateStage();
	bool LoadStage(int nextStage);
	bool CheckGameEnd();		// 게임 종료가 판정되면 true
	void RemoveLeftPlayers();
	void PushEvent(const void* packet, int size);	// Protocol.h에 정의된 이벤트 패킷
	void FlushEvents();
	void SendSnapshot();
};
