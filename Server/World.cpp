#define NOMINMAX
#include "World.h"
#include "Bullet.h"
#include "ServerPlayer.h"
#include "ServerEnemy.h"
#include "ServerBoss.h"
#include "ClientThread.h"
#include "GameData.h"
#include "CollisionCheck.h"
#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstring>
#include <limits>

namespace
{
	bool IsPlayerBlocked(const TileMap& map, Vector2D<float> pos, Vector2D<float> size)
	{
		if (pos.x - size.x < 0 || pos.y - size.y < 0 ||
			pos.x + size.x > TileMap::Columns * TileMap::TileExtent ||
			pos.y + size.y > TileMap::Rows * TileMap::TileExtent)
			return true;
		for (const auto& tile : map.blockedTiles)
			if (CollisionCheck::RectRect(pos, size, tile.pos, tile.size))
				return true;
		return false;
	}
}

World::World() = default;
World::~World() = default;

bool World::Initialize(const S2C_GAME_START_PACKET& start, TileMap initialMap)
{
	// [S9] 시작 정보와 맵을 모두 확인한 뒤 월드를 교체한다.
	std::array<ServerPlayer, MAX_PLAYERS> loadedPlayers{};
	int playerCount = 0;
	for (int id = 0; id < MAX_PLAYERS; ++id) {
		const auto& info = start.players[id];
		if (!info.used)
			continue;
		if (info.charType > CHAR_KNIGHT || !std::isfinite(info.x) || !std::isfinite(info.y))
			return false;
		const auto& stats = GameData::Players[info.charType];
		if (info.x < stats.halfWidth || info.x >= TileMap::Columns * TileMap::TileExtent - stats.halfWidth ||
			info.y < stats.halfHeight || info.y >= TileMap::Rows * TileMap::TileExtent - stats.halfHeight)
			return false;
		auto& player = loadedPlayers[id];
		player.used = true;
		player.playerId = static_cast<std::uint8_t>(id);
		player.charType = static_cast<std::uint8_t>(info.charType);
		player.hp = static_cast<std::int16_t>(stats.hp);
		player.moveSpeedPerSecond = stats.speedPerSecond;
		player.size = Vector2D<float>(stats.halfWidth, stats.halfHeight);
		player.pos = player.lastPos = Vector2D<float>(info.x, info.y);
		player.aimPos = player.pos + Vector2D<float>(1, 0);
		player.weapon = WEAPON_SWORD;
		player.ammo[WEAPON_SWORD] = 1;
		++playerCount;
	}
	if (playerCount == 0)
		return false;
	for (const auto& player : loadedPlayers)
		if (player.used && IsPlayerBlocked(initialMap, player.pos, player.size))
			return false;
	players = std::move(loadedPlayers);
	map = std::move(initialMap);
	enemies.clear();
	bullets.clear();
	drops.clear();
	boss.reset();
	events.clear();
	stage = phase = 1;
	tick = 0;
	nextBulletId = 0;
	portalOpen = false;
	portalPos = {};
	return true;
}

void World::Tick(float deltaTime)
{
	// [S9] World::Tick: S10 → S11 → S12 → S13 순서로 고정 틱 실행
	if (stage == 0)
		return;
	++tick;
	UpdatePlayers(deltaTime);
	UpdateBullets(deltaTime);
	ResolveCollisions();
	UpdateEnemies(deltaTime);
	if (boss && boss->active)
		boss->Update(deltaTime, *this);
	UpdateStage();
	CheckGameEnd();
	FlushEvents();
	SendSnapshot();
}

void World::UpdatePlayers(float deltaTime)
{
	// [S10] 세션 입력 잠금은 TakeInput 내부에서 처리한다.
	for (auto& player : players)
		if (player.used)
			player.ApplyInput(TakeInput(player.playerId), deltaTime, *this);
}

Bullet* World::SpawnBullet(std::uint8_t type, std::uint8_t side, Vector2D<float> pos, Vector2D<float> dir)
{
	// [S10] World::SpawnBullet: 활성 총알과 겹치지 않는 ID를 발급한다.
	if (stage == 0 || type >= std::size(GameData::Bullets) ||
		(side != GameData::BULLET_SIDE_PLAYER && side != GameData::BULLET_SIDE_ENEMY) ||
		!std::isfinite(pos.x) || !std::isfinite(pos.y) || !std::isfinite(dir.x) || !std::isfinite(dir.y))
		return nullptr;
	if (type != GameData::BULLET_BOSS_ORBIT && dir.x == 0 && dir.y == 0)
		return nullptr;
	for (std::uint32_t attempt = 0; attempt <= std::numeric_limits<std::uint16_t>::max(); ++attempt) {
		const auto id = nextBulletId++;
		if (std::any_of(bullets.begin(), bullets.end(), [id](const auto& bullet) {
			return bullet->active && bullet->id == id;
		}))
			continue;
		auto bullet = std::make_unique<Bullet>();
		bullet->active = true;
		bullet->id = id;
		bullet->type = type;
		bullet->side = side;
		bullet->pos = bullet->lastPos = pos;
		bullet->dir = dir;
		bullet->orbitCenter = dir;
		const auto& stats = GameData::Bullets[type];
		bullet->damage = stats.damage;
		bullet->radius = stats.radius;
		bullet->speedPerSecond = stats.speedPerSecond;
		auto* result = bullet.get();
		bullets.push_back(std::move(bullet));
		S2C_BULLET_SPAWN_PACKET packet{};
		packet.type = S2C_BULLET_SPAWN;
		packet.count = 1;
		packet.size = static_cast<UINT16>(offsetof(S2C_BULLET_SPAWN_PACKET, bullets) + sizeof(BulletSpawn));
		packet.bullets[0] = { id, type, side, pos.x, pos.y, dir.x, dir.y };
		PushEvent(&packet, packet.size);
		return result;
	}
	return nullptr;
}

void World::UpdateBullets(float deltaTime)
{
	// [S10] World::UpdateBullets: 원작의 초당 속도로 직선·회전 이동한다.
	for (auto& bullet : bullets) {
		if (!bullet->active)
			continue;
		if (!bullet->firstUpdate)
			bullet->lastPos = bullet->pos;
		bullet->firstUpdate = false;
		if (bullet->type == GameData::BULLET_BOSS_ORBIT) {
			bullet->stepRotationDegrees = std::min(GameData::OrbitBulletDegreesPerSecond * deltaTime,
				600.0f - bullet->rotationDegrees);
			bullet->pos = (bullet->pos - bullet->orbitCenter).Rotate(bullet->stepRotationDegrees) + bullet->orbitCenter;
			bullet->rotationDegrees += bullet->stepRotationDegrees;
		} else {
			const double length = std::hypot(static_cast<double>(bullet->dir.x), static_cast<double>(bullet->dir.y));
			const Vector2D<float> direction(static_cast<float>(bullet->dir.x / length), static_cast<float>(bullet->dir.y / length));
			bullet->pos += direction * (bullet->speedPerSecond * deltaTime);
		}
	}
}

void World::ResolveCollisions()
{
	// [S10] 작은 이동 구간마다 축별로 검사해 벽·물에서 미끄러진다.
	for (auto& player : players) {
		if (!player.used || player.hp <= 0)
			continue;
		const auto movement = player.pos - player.lastPos;
		const float distance = std::max(std::abs(movement.x), std::abs(movement.y));
		const int steps = std::max(1, static_cast<int>(std::ceil(distance / 4.0f)));
		const auto step = movement / static_cast<float>(steps);
		auto resolved = player.lastPos;
		for (int i = 0; i < steps; ++i) {
			const auto horizontal = resolved + Vector2D<float>(step.x, 0);
			if (!IsPlayerBlocked(map, horizontal, player.size))
				resolved.x = horizontal.x;
			const auto vertical = resolved + Vector2D<float>(0, step.y);
			if (!IsPlayerBlocked(map, vertical, player.size))
				resolved.y = vertical.y;
		}
		player.pos = resolved;
	}
	// [S10] 총알 경로를 4픽셀 이하로 나눠 벽과 작은 대상을 검사한다.
	for (auto& bullet : bullets) {
		if (!bullet->active)
			continue;
		const bool orbit = bullet->type == GameData::BULLET_BOSS_ORBIT;
		const auto movement = bullet->pos - bullet->lastPos;
		const auto radiusVector = bullet->lastPos - bullet->orbitCenter;
		const float distance = orbit ? std::hypot(radiusVector.x, radiusVector.y) *
			bullet->stepRotationDegrees * static_cast<float>(M_PI / 180.0) : std::hypot(movement.x, movement.y);
		const int steps = std::max(1, static_cast<int>(std::ceil(distance / 4.0f)));
		std::array<bool, MAX_PLAYERS> hitPlayers{};
		bool hit = false;
		for (int step = 0; step <= steps; ++step) {
			const float ratio = static_cast<float>(step) / steps;
			auto position = bullet->lastPos + movement * ratio;
			if (orbit) {
				auto offset = radiusVector;
				position = offset.Rotate(bullet->stepRotationDegrees * ratio) + bullet->orbitCenter;
			}
			if (!orbit && (position.x < 0 || position.y < 0 ||
				position.x >= TileMap::Columns * TileMap::TileExtent || position.y >= TileMap::Rows * TileMap::TileExtent)) {
				bullet->pos = position;
				bullet->active = false;
				break;
			}
			if (HitBulletTarget(*bullet, position, hitPlayers)) {
				hit = true;
				if (!orbit) {
					bullet->pos = position;
					bullet->active = false;
					break;
				}
			}
		}
		if (orbit && bullet->rotationDegrees >= 600.0f)
			bullet->active = false;
		if (bullet->active)
			continue;
		if (hit && bullet->type == GameData::BULLET_ROCKET) {
			const Vector2D<float> blastSize(GameData::RocketBlastRadius, GameData::RocketBlastRadius);
			for (auto& enemy : enemies)
				if (enemy->active && enemy->hp > 0 && CollisionCheck::CircleRect(bullet->pos, blastSize, enemy->pos, enemy->size))
					DamageEnemy(*enemy, GameData::RocketBlastDamage);
		}
		S2C_BULLET_REMOVE_PACKET packet{};
		packet.size = sizeof(packet);
		packet.type = S2C_BULLET_REMOVE;
		packet.id = bullet->id;
		packet.hit = hit ? 1 : 0;
		packet.x = bullet->pos.x;
		packet.y = bullet->pos.y;
		PushEvent(&packet, sizeof(packet));
	}
	std::erase_if(bullets, [](const auto& bullet) { return !bullet->active; });
}

bool World::HitBulletTarget(const Bullet& bullet, Vector2D<float> pos, std::array<bool, MAX_PLAYERS>& hitPlayers)
{
	const Vector2D<float> size(bullet.radius, bullet.radius);
	if (bullet.type != GameData::BULLET_BOSS_ORBIT)
		for (const auto& tile : map.blockedTiles)
			if (tile.type == TileMap::WallType && CollisionCheck::CircleRect(pos, size, tile.pos, tile.size))
				return true;
	if (bullet.side == GameData::BULLET_SIDE_PLAYER) {
		for (auto& enemy : enemies) {
			if (!enemy->active || enemy->hp <= 0)
				continue;
			const bool overlap = enemy->type == GameData::EnemyKind::Bat ?
				CollisionCheck::CircleCircle(pos, size, enemy->pos, enemy->size) :
				CollisionCheck::CircleRect(pos, size, enemy->pos, enemy->size);
			if (overlap) {
				DamageEnemy(*enemy, bullet.damage);
				return true;
			}
		}
		if (boss && boss->active && boss->hp > 0 &&
			CollisionCheck::CircleCircle(pos, size, boss->pos, { boss->radius, boss->radius })) {
			boss->hp = static_cast<std::int16_t>(std::max(0, boss->hp - bullet.damage));
			return true;
		}
		return false;
	}
	bool hit = false;
	for (int id = 0; id < MAX_PLAYERS; ++id) {
		auto& player = players[id];
		if (hitPlayers[id] || !player.used || player.hp <= 0 || player.rollTime > 0 || player.invulnerableTime > 0)
			continue;
		if (CollisionCheck::RectCircle(player.pos, player.size, pos, size)) {
			DamagePlayer(player, bullet.damage);
			hitPlayers[id] = hit = true;
			if (bullet.type != GameData::BULLET_BOSS_ORBIT)
				break;
		}
	}
	return hit;
}

void World::SwordSlash(ServerPlayer& player)
{
	// [S10] World::SwordSlash: 칼 공격 범위 판정, 기사 총알 반사
}

void World::DamagePlayer(ServerPlayer& player, int damage)
{
	// [S10] World::DamagePlayer: 플레이어 피격, 사망 처리
}

void World::UpdateEnemies(float deltaTime)
{
	// [S11] World::UpdateEnemies: 적 타깃 선택, Behavior Tree 실행, 이동
}

ServerPlayer* World::FindNearestPlayer(const Vector2D<float>& pos) const
{
	// [S11] World::FindNearestPlayer: 가장 가까운 생존 플레이어 찾기
	return nullptr;
}

void World::DamageEnemy(ServerEnemy& enemy, int damage)
{
	// [S11] World::DamageEnemy: 적 피격, 사망, 드랍 생성
}

void World::SpawnWave()
{
	// [S12] World::SpawnWave: 현재 스테이지·웨이브 표대로 적 생성
}

void World::UpdateStage()
{
	// [S12] World::UpdateStage: 다음 웨이브, 포탈 열기, 생존자 전원의 E키 상호작용으로 이동 판단
}

bool World::LoadStage(int nextStage)
{
	// [S12] World::LoadStage: 맵 교체, 플레이어 이동·부활, 보스 생성
	return false;
}

bool World::CheckGameEnd()
{
	// [S12] World::CheckGameEnd: 클리어·전멸·전원 이탈 판정, EndGame 호출
	return false;
}

void World::RemoveLeftPlayers()
{
	// [S12] World::RemoveLeftPlayers: room.lock을 잡고 이탈 목록을 꺼낸 뒤 월드의 해당 플레이어 제거
}

void World::PushEvent(const void* packet, int size)
{
	// [S13] Protocol.h 이벤트 패킷을 소유하는 바이트 배열로 복사한다.
	if (!packet || size < static_cast<int>(sizeof(PacketHeader)) || size > MAX_PACKET_SIZE)
		return;
	PacketHeader header{};
	std::memcpy(&header, packet, sizeof(header));
	if (header.size != size || header.type < S2C_BULLET_SPAWN || header.type > S2C_PLAYER_LEFT)
		return;
	std::vector<char> bytes(static_cast<std::size_t>(size));
	std::memcpy(bytes.data(), packet, bytes.size());
	events.push_back(std::move(bytes));
}

void World::FlushEvents()
{
	// [S13] 소켓 대신 세션별 송신 큐로 이벤트를 전달한다.
	for (const auto& event : events)
		Broadcast(event.data(), static_cast<int>(event.size()));
	events.clear();
}

void World::SendSnapshot()
{
	// [S13] 매 틱 플레이어·적·보스 상태를 가변 길이로 전송한다.
	S2C_SNAPSHOT_PACKET packet{};
	packet.type = S2C_SNAPSHOT;
	packet.tick = tick;
	for (int id = 0; id < MAX_PLAYERS; ++id) {
		const auto& player = players[id];
		if (!player.used)
			continue;
		auto& state = packet.players[id];
		state.used = 1;
		state.state = player.state;
		state.x = player.pos.x;
		state.y = player.pos.y;
		state.angle = player.angle;
		state.hp = static_cast<UINT16>(std::max(0, static_cast<int>(player.hp)));
		state.weapon = static_cast<WeaponType>(player.weapon);
		std::copy(player.ammo.begin(), player.ammo.end(), state.ammo);
	}
	for (const auto& enemy : enemies) {
		if (!enemy || !enemy->active || packet.enemyCount == MAX_ENEMIES)
			continue;
		auto& state = packet.enemies[packet.enemyCount++];
		state.id = enemy->id;
		state.state = enemy->state;
		state.x = enemy->pos.x;
		state.y = enemy->pos.y;
		state.angle = enemy->angle;
		state.hp = static_cast<UINT16>(std::max(0, static_cast<int>(enemy->hp)));
	}
	if (boss && boss->active) {
		packet.boss.active = 1;
		packet.boss.x = boss->pos.x;
		packet.boss.y = boss->pos.y;
		packet.boss.hp = static_cast<UINT16>(std::max(0, static_cast<int>(boss->hp)));
		for (int hand = 0; hand < 2; ++hand) {
			packet.boss.handX[hand] = boss->handPos[hand].x;
			packet.boss.handY[hand] = boss->handPos[hand].y;
			packet.boss.handType[hand] = boss->handType[hand];
		}
	}
	packet.size = static_cast<UINT16>(offsetof(S2C_SNAPSHOT_PACKET, enemies) + packet.enemyCount * sizeof(EnemySnap));
	Broadcast(&packet, packet.size);
}
