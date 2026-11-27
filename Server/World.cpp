#define NOMINMAX
#include "World.h"
#include "Bullet.h"
#include "ServerPlayer.h"
#include "ServerEnemy.h"
#include "ServerBoss.h"
#include "ClientThread.h"
#include "LogicThread.h"
#include "GameData.h"
#include "CollisionCheck.h"
#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstring>
#include <limits>
#include <random>

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

	void PushEffect(World& world, std::uint8_t type, Vector2D<float> pos)
	{
		S2C_EFFECT_PACKET packet{};
		packet.size = sizeof(packet);
		packet.PacketHeader::type = S2C_EFFECT;
		packet.type = type;
		packet.x = pos.x;
		packet.y = pos.y;
		world.PushEvent(&packet, sizeof(packet));
	}

	bool InSwordArc(Vector2D<float> center, float angle, Vector2D<float> pos, Vector2D<float> size)
	{
		const float radians = angle * static_cast<float>(M_PI / 180.0);
		const Vector2D<float> forward(std::cos(radians), std::sin(radians));
		const float limit = std::cos(GameData::SwordHalfAngleDegrees * static_cast<float>(M_PI / 180.0));
		for (float x : { -size.x, size.x })
			for (float y : { -size.y, size.y }) {
				const auto offset = pos + Vector2D<float>(x, y) - center;
				const float distance = std::hypot(offset.x, offset.y);
				if (distance <= GameData::SwordRange && forward.Dot(offset) >= distance * limit)
					return true;
			}
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
	finished = false;
	nextBulletId = 0;
	nextEnemyId = 0;
	nextDropId = 0;
	spawnedStage = spawnedPhase = 0;
	portalOpen = false;
	portalPos = {};
	portalReady = {};
	SpawnWave();
	return true;
}

void World::Tick(float deltaTime)
{
	// [S9] World::Tick: S10 → S11 → S12 → S13 순서로 고정 틱 실행
	if (stage == 0 || finished)
		return;
	RemoveLeftPlayers();
	if (CheckGameEnd())
		return;
	++tick;
	UpdatePlayers(deltaTime);
	UpdateBullets(deltaTime);
	ResolveCollisions();
	for (auto& player : players)
		if (player.used && player.swordPending && player.swordTime <= 0)
			SwordSlash(player);
	// [S10] 벽에 막힌 최종 위치에서 발사하고 다음 틱부터 총알을 이동한다.
	for (auto& player : players)
		if (player.used && player.attackRequested)
			player.TryFire(*this);
	UpdateEnemies(deltaTime);
	if (boss && boss->active)
		boss->Update(deltaTime, *this);
	UpdateStage();
	if (CheckGameEnd())
		return;
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
	if (stage == 0 || finished || type >= std::size(GameData::Bullets) ||
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
	// [S10] 드랍 하나는 한 플레이어에게만 지급한다.
	for (auto& drop : drops) {
		for (auto& player : players) {
			if (!drop.active || !player.used || player.hp <= 0 ||
				!CollisionCheck::RectRect(player.pos, player.size, drop.pos, drop.size))
				continue;
			const auto weapon = static_cast<std::uint8_t>(drop.type + WEAPON_PISTOL);
			const auto maxAmmo = GameData::Weapons[weapon].maxAmmo;
			if (player.ammo[weapon] >= maxAmmo)
				continue;
			player.ammo[weapon] = static_cast<std::uint8_t>(weapon == WEAPON_ROCKET ? player.ammo[weapon] + 1 : maxAmmo);
			player.weapon = weapon;
			player.fireCooldown = std::max(player.fireCooldown, GameData::Weapons[weapon].cooldownSeconds - GameData::WeaponSwitchLeadSeconds);
			drop.active = false;
			S2C_DROP_PACKET removed{};
			removed.size = sizeof(removed);
			removed.PacketHeader::type = S2C_DROP;
			removed.id = drop.id;
			removed.type = drop.type;
			removed.x = drop.pos.x;
			removed.y = drop.pos.y;
			PushEvent(&removed, sizeof(removed));
			S2C_PLAYER_EVENT_PACKET pickup{};
			pickup.size = sizeof(pickup);
			pickup.type = S2C_PLAYER_EVENT;
			pickup.playerId = player.playerId;
			pickup.event = PE_PICKUP;
			pickup.value = drop.type;
			PushEvent(&pickup, sizeof(pickup));
			break;
		}
	}
	std::erase_if(drops, [](const auto& drop) { return !drop.active; });
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
			if (!enemy->active || enemy->hp <= 0 || enemy->spawnTime > 0 ||
				(enemy->type == GameData::EnemyKind::Bat && enemy->attackStarted))
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
	// [S10] World::SwordSlash: 원작처럼 대상의 모서리로 부채꼴 범위를 검사한다.
	player.swordPending = false;
	if (!player.used || player.hp <= 0)
		return;
	const auto center = player.pos + Vector2D<float>(std::abs(player.swordAngle) < 90 ? 18.0f : -17.0f, 8.0f);
	for (auto& enemy : enemies)
		if (enemy->active && enemy->hp > 0 && enemy->spawnTime <= 0 &&
			!(enemy->type == GameData::EnemyKind::Bat && enemy->attackStarted) &&
			InSwordArc(center, player.swordAngle, enemy->pos, enemy->size))
		{
			DamageEnemy(*enemy, GameData::SwordDamage);
			PushEffect(*this, 5, enemy->pos); // CEffect::SWORDATTACK
		}
	if (boss && boss->active && boss->hp > 0 &&
		InSwordArc(center, player.swordAngle, boss->pos, { boss->radius, boss->radius }))
		boss->hp = static_cast<std::int16_t>(std::max(0, boss->hp - GameData::SwordDamage));
	for (auto& bullet : bullets) {
		if (!bullet->active || bullet->side != GameData::BULLET_SIDE_ENEMY ||
			bullet->type == GameData::BULLET_BOSS_ORBIT || bullet->type == GameData::BULLET_BOSS_RADIAL ||
			!InSwordArc(center, player.swordAngle, bullet->pos, { bullet->radius, bullet->radius }))
			continue;
		S2C_BULLET_REMOVE_PACKET removed{};
		removed.size = sizeof(removed);
		removed.type = S2C_BULLET_REMOVE;
		removed.id = bullet->id;
		removed.x = bullet->pos.x;
		removed.y = bullet->pos.y;
		PushEvent(&removed, sizeof(removed));
		if (player.charType != CHAR_KNIGHT) {
			bullet->active = false;
			continue;
		}
		// 반사는 같은 ID의 제거·생성을 보내 클라이언트 방향도 교체한다.
		const auto aim = player.aimPos - center;
		const float length = std::hypot(aim.x, aim.y);
		const float radians = player.swordAngle * static_cast<float>(M_PI / 180.0);
		bullet->dir = (length > 0 ? aim / length : Vector2D<float>(std::cos(radians), std::sin(radians))) * 10.0f;
		bullet->type = bullet->type == GameData::BULLET_BOSS_AIMED ? GameData::BULLET_BOSS_REFLECTED : GameData::BULLET_BOUNCE;
		bullet->side = GameData::BULLET_SIDE_PLAYER;
		bullet->speedPerSecond = GameData::Bullets[bullet->type].speedPerSecond;
		bullet->lastPos = bullet->pos;
		bullet->firstUpdate = false;
		S2C_BULLET_SPAWN_PACKET spawn{};
		spawn.type = S2C_BULLET_SPAWN;
		spawn.count = 1;
		spawn.size = static_cast<UINT16>(offsetof(S2C_BULLET_SPAWN_PACKET, bullets) + sizeof(BulletSpawn));
		spawn.bullets[0] = { bullet->id, bullet->type, bullet->side, bullet->pos.x, bullet->pos.y, bullet->dir.x, bullet->dir.y };
		PushEvent(&spawn, spawn.size);
	}
	std::erase_if(bullets, [](const auto& bullet) { return !bullet->active; });
}

void World::DamagePlayer(ServerPlayer& player, int damage)
{
	// [S10] World::DamagePlayer: 원작 체력은 피해량과 관계없이 한 칸씩 감소한다.
	if (!player.used || player.hp <= 0 || damage <= 0 || player.rollTime > 0 || player.invulnerableTime > 0)
		return;
	--player.hp;
	player.state = player.hp > 0 ? GameData::PLAYER_DAMAGED : GameData::PLAYER_DEAD;
	player.invulnerableTime = player.hp > 0 ? GameData::PlayerInvulnerableSeconds : 0;
	if (player.hp == 0) {
		player.attackRequested = player.interactRequested = player.swordPending = false;
		player.rollTime = 0;
	}
	S2C_PLAYER_EVENT_PACKET packet{};
	packet.size = sizeof(packet);
	packet.type = S2C_PLAYER_EVENT;
	packet.playerId = player.playerId;
	packet.event = player.hp > 0 ? PE_HIT : PE_DEAD;
	packet.value = static_cast<UINT8>(player.hp);
	PushEvent(&packet, sizeof(packet));
}

void World::UpdateEnemies(float deltaTime)
{
	// [S11] World::UpdateEnemies: 생성 대기 후 타깃·BT·이동·공격을 갱신한다.
	for (auto& pointer : enemies) {
		auto& enemy = *pointer;
		if (!enemy.active)
			continue;
		if (enemy.hp <= 0) {
			enemy.deathTime = std::max(0.0f, enemy.deathTime - deltaTime);
			if (enemy.deathTime <= 0)
				FinishEnemyDeath(enemy);
			continue;
		}
		if (enemy.hurtTime > 0) {
			enemy.hurtTime = std::max(0.0f, enemy.hurtTime - deltaTime);
			if (enemy.hurtTime > 0)
				continue;
			enemy.state = enemy.type == GameData::EnemyKind::Bombshe && enemy.attackStarted ? 1 : 0;
			enemy.moveTime = 0;
		}
		const float activeTime = std::max(0.0f, deltaTime - enemy.spawnTime);
		enemy.spawnTime = std::max(0.0f, enemy.spawnTime - deltaTime);
		if (activeTime <= 0)
			continue;
		enemy.lastPos = enemy.pos;
		enemy.attackCooldown = std::max(0.0f, enemy.attackCooldown - activeTime);
		enemy.moveTime = std::max(0.0f, enemy.moveTime - activeTime);
		const auto* target = FindNearestPlayer(enemy.pos);
		enemy.targetPlayerId = target ? target->playerId : -1;
		if (target) {
			const auto offset = target->pos - enemy.pos;
			enemy.angle = static_cast<float>(std::atan2(offset.y, offset.x) * 180.0 / M_PI);
		} else
			enemy.dir = {};
		if (enemy.tree)
			enemy.tree->Tick(*this, enemy, activeTime);

		if (enemy.type == GameData::EnemyKind::Bat && enemy.attackStarted) {
			enemy.actionTime = std::max(0.0f, enemy.actionTime - activeTime);
			if (enemy.actionTime <= 0) {
				enemy.hp = 0;
				FinishEnemyDeath(enemy);
			}
			continue;
		}
		if (enemy.type == GameData::EnemyKind::Bombshe && enemy.attackStarted) {
			enemy.actionTime += activeTime;
			enemy.waveRadius = std::min(GameData::BombsheWaveMaxRadius, enemy.actionTime * GameData::BombsheWaveGrowthPerSecond);
			if (enemy.actionTime >= GameData::BansheeWindupSeconds) {
				enemy.waveEffectTime -= activeTime;
				if (!enemy.waveActive || enemy.waveEffectTime <= 0) {
					S2C_EFFECT_PACKET effect{};
					effect.size = sizeof(effect);
					effect.PacketHeader::type = S2C_EFFECT;
					effect.S2C_EFFECT_PACKET::type = enemy.waveActive ? GameData::EFFECT_BANSHEE_LOOP : GameData::EFFECT_BANSHEE_START;
					effect.x = enemy.pos.x;
					effect.y = enemy.pos.y;
					PushEvent(&effect, sizeof(effect));
					enemy.waveEffectTime = enemy.waveActive ? 0.4f : 1.0f;
					enemy.waveActive = true;
				}
				const Vector2D<float> radius(enemy.waveRadius, enemy.waveRadius);
				for (auto& player : players)
					if (player.used && CollisionCheck::RectCircle(player.pos, player.size, enemy.pos, radius))
						DamagePlayer(player, GameData::ContactDamage);
			}
		}

		// 박쥐는 물 위를 날고, 나머지 적은 벽과 물을 모두 피한다.
		const bool bat = enemy.type == GameData::EnemyKind::Bat;
		auto blocked = [&](Vector2D<float> pos) {
			if (pos.x - enemy.size.x < 0 || pos.y - enemy.size.y < 0 ||
				pos.x + enemy.size.x > TileMap::Columns * TileMap::TileExtent ||
				pos.y + enemy.size.y > TileMap::Rows * TileMap::TileExtent)
				return true;
			for (const auto& tile : map.blockedTiles) {
				if (bat && tile.type == TileMap::WaterType)
					continue;
				if (bat ? CollisionCheck::CircleRect(pos, enemy.size, tile.pos, tile.size) :
					CollisionCheck::RectRect(pos, enemy.size, tile.pos, tile.size))
					return true;
			}
			return false;
		};
		const auto movement = enemy.dir * (enemy.moveSpeedPerSecond * activeTime);
		const float distance = std::max(std::abs(movement.x), std::abs(movement.y));
		const int steps = std::max(1, static_cast<int>(std::ceil(distance / 4.0f)));
		const auto step = movement / static_cast<float>(steps);
		for (int i = 0; i < steps; ++i) {
			const auto horizontal = enemy.pos + Vector2D<float>(step.x, 0);
			if (!blocked(horizontal))
				enemy.pos.x = horizontal.x;
			const auto vertical = enemy.pos + Vector2D<float>(0, step.y);
			if (!blocked(vertical))
				enemy.pos.y = vertical.y;
		}
		for (auto& player : players) {
			if (!player.used || player.hp <= 0)
				continue;
			const bool overlap = bat ? CollisionCheck::RectCircle(player.pos, player.size, enemy.pos, enemy.size) :
				CollisionCheck::RectRect(player.pos, player.size, enemy.pos, enemy.size);
			if (!overlap)
				continue;
			DamagePlayer(player, GameData::ContactDamage);
			enemy.pos = enemy.lastPos;
			if (!IsPlayerBlocked(map, player.lastPos, player.size))
				player.pos = player.lastPos;
		}
	}
}

const ServerPlayer* World::FindNearestPlayer(const Vector2D<float>& pos) const
{
	// [S11] World::FindNearestPlayer: 거리 제곱으로 생존 플레이어를 고른다.
	const ServerPlayer* nearest = nullptr;
	double nearestDistance = std::numeric_limits<double>::max();
	for (const auto& player : players) {
		if (!player.used || player.hp <= 0)
			continue;
		const double x = static_cast<double>(player.pos.x) - pos.x;
		const double y = static_cast<double>(player.pos.y) - pos.y;
		const double distance = x * x + y * y;
		if (distance < nearestDistance) {
			nearestDistance = distance;
			nearest = &player;
		}
	}
	return nearest;
}

void World::DamageEnemy(ServerEnemy& enemy, int damage)
{
	// [S11] World::DamageEnemy: 생성·사망 중인 적은 다시 피격되지 않는다.
	if (!enemy.active || enemy.hp <= 0 || enemy.spawnTime > 0 || damage <= 0 ||
		(enemy.type == GameData::EnemyKind::Bat && enemy.attackStarted))
		return;
	enemy.hp = static_cast<std::int16_t>(std::max(0, enemy.hp - damage));
	enemy.dir = {};
	if (enemy.hp > 0) {
		enemy.state = enemy.type == GameData::EnemyKind::Bat ? 0 : 2;
		enemy.hurtTime = enemy.type == GameData::EnemyKind::Bat ? 0.0f :
			enemy.type == GameData::EnemyKind::Bombshe ? 0.125f : 1.0f / 3.0f;
		return;
	}
	enemy.state = enemy.type == GameData::EnemyKind::Bat ? 1 : 3;
	enemy.deathTime = enemy.type == GameData::EnemyKind::Bombshe ? 1.0f / 3.0f : 0.5f;
	enemy.waveActive = false;
	enemy.deathNotified = true;
	S2C_ENEMY_DEAD_PACKET packet{};
	packet.size = sizeof(packet);
	packet.type = S2C_ENEMY_DEAD;
	packet.id = enemy.id;
	PushEvent(&packet, sizeof(packet));
}

void World::FinishEnemyDeath(ServerEnemy& enemy)
{
	if (!enemy.active)
		return;
	enemy.active = false;
	if (!enemy.deathNotified) {
		S2C_ENEMY_DEAD_PACKET packet{};
		packet.size = sizeof(packet);
		packet.type = S2C_ENEMY_DEAD;
		packet.id = enemy.id;
		PushEvent(&packet, sizeof(packet));
		enemy.deathNotified = true;
	}
	if (enemy.type == GameData::EnemyKind::Bat) {
		const Vector2D<float> radius(GameData::BatBlastRadius, GameData::BatBlastRadius);
		for (auto& player : players)
			if (player.used && CollisionCheck::RectCircle(player.pos, player.size, enemy.pos, radius))
				DamagePlayer(player, GameData::ContactDamage);
		PushEffect(*this, GameData::EFFECT_BAT_EXPLOSION, enemy.pos);
		return;
	}
	if (enemy.type == GameData::EnemyKind::Shotgunman)
		PushEffect(*this, 10, enemy.pos); // CEffect::SHOTGUNDIE
	else if (enemy.type == GameData::EnemyKind::Bombshe)
		PushEffect(*this, 9, enemy.pos); // CEffect::BOMBSHEDIE
	const int type = GameData::Enemies[static_cast<std::size_t>(enemy.type)].dropType;
	static thread_local std::mt19937 random(std::random_device{}());
	if (type < 0 || std::uniform_int_distribution<int>(0, 1)(random) != 0 || drops.size() >= 65536)
		return;
	std::uint16_t id = nextDropId++;
	while (std::any_of(drops.begin(), drops.end(), [id](const auto& drop) { return drop.active && drop.id == id; }))
		id = nextDropId++;
	const auto size = GameData::DropHalfSizes[type];
	drops.push_back({ id, static_cast<std::uint8_t>(type), enemy.pos, { size.x, size.y }, true });
	S2C_DROP_PACKET packet{};
	packet.size = sizeof(packet);
	packet.PacketHeader::type = S2C_DROP;
	packet.id = id;
	packet.add = 1;
	packet.type = static_cast<std::uint8_t>(type);
	packet.x = enemy.pos.x;
	packet.y = enemy.pos.y;
	PushEvent(&packet, sizeof(packet));
}

void World::SpawnWave()
{
	// [S12] World::SpawnWave: 같은 웨이브를 중복 생성하지 않는다.
	if (stage < 1 || stage > 3 || phase < 1 || phase > 3 ||
		(spawnedStage == stage && spawnedPhase == phase))
		return;
	std::erase_if(enemies, [](const auto& enemy) { return !enemy->active; });
	const auto wave = GameData::StageWaves[stage - 1][phase - 1];
	const auto activeCount = std::count_if(enemies.begin(), enemies.end(), [](const auto& enemy) { return enemy->active; });
	if (activeCount + wave.size() > MAX_ENEMIES)
		return;
	for (const auto& spawn : wave) {
		std::uint16_t id = nextEnemyId++;
		while (std::any_of(enemies.begin(), enemies.end(), [id](const auto& enemy) { return enemy->active && enemy->id == id; }))
			id = nextEnemyId++;
		const auto& stats = GameData::Enemies[static_cast<std::size_t>(spawn.kind)];
		auto enemy = std::make_unique<ServerEnemy>();
		enemy->active = true;
		enemy->id = id;
		enemy->type = spawn.kind;
		enemy->hp = static_cast<std::int16_t>(stats.hp);
		enemy->pos = enemy->lastPos = { spawn.position.x, spawn.position.y };
		enemy->size = { stats.halfWidth, stats.halfHeight };
		enemy->moveSpeedPerSecond = stats.moveSpeedPerSecond;
		enemy->attackRange = stats.attackRange;
		enemy->spawnTime = GameData::EnemySpawnSeconds;
		if (spawn.kind <= GameData::EnemyKind::Shotgunman) {
			enemy->weapon = static_cast<std::uint8_t>(spawn.kind) + WEAPON_PISTOL;
			enemy->ammo = GameData::Weapons[enemy->weapon].maxAmmo / GameData::GunnerAmmoDivisor;
			enemy->attackCooldown = GameData::GunnerFirstShotDelaySeconds;
			enemy->moveTime = 200 / GameData::OriginalUpdatesPerSecond;
			enemy->tree = BuildGunnerTree();
		} else if (spawn.kind == GameData::EnemyKind::Bat)
			enemy->tree = BuildBatTree();
		else
			enemy->tree = BuildBansheeTree();
		enemies.push_back(std::move(enemy));
		S2C_ENEMY_SPAWN_PACKET packet{};
		packet.size = sizeof(packet);
		packet.PacketHeader::type = S2C_ENEMY_SPAWN;
		packet.id = id;
		packet.S2C_ENEMY_SPAWN_PACKET::type = static_cast<EnemyType>(spawn.kind);
		packet.x = spawn.position.x;
		packet.y = spawn.position.y;
		PushEvent(&packet, sizeof(packet));
	}
	spawnedStage = stage;
	spawnedPhase = phase;
}

void World::UpdateStage()
{
	// [S12] World::UpdateStage: 사망 연출까지 끝난 뒤 다음 웨이브를 연다.
	if (stage < 1 || stage > 3 ||
		!std::any_of(players.begin(), players.end(), [](const auto& player) { return player.used && player.hp > 0; }) ||
		std::any_of(enemies.begin(), enemies.end(), [](const auto& enemy) { return enemy->active; }))
		return;
	if (phase < 3) {
		++phase;
		SpawnWave();
		return;
	}
	if (!portalOpen) {
		portalOpen = true;
		portalReady = {};
		const auto pos = GameData::PortalSpawns[stage - 1];
		portalPos = { pos.x, pos.y };
		S2C_PORTAL_PACKET packet{};
		packet.size = sizeof(packet);
		packet.type = S2C_PORTAL;
		packet.x = portalPos.x;
		packet.y = portalPos.y;
		PushEvent(&packet, sizeof(packet));
	}
	bool allReady = true;
	for (int id = 0; id < MAX_PLAYERS; ++id) {
		const auto& player = players[id];
		if (!player.used || player.hp <= 0) {
			portalReady[id] = false;
			continue;
		}
		const auto distance = player.pos - portalPos;
		const bool inPortalRange = std::hypot(distance.x, distance.y) < GameData::PortalUseRange;
		if (!inPortalRange)
			portalReady[id] = false;
		else if (player.interactRequested)
			portalReady[id] = true;
		allReady = allReady && inPortalRange && portalReady[id];
	}
	if (allReady)
		LoadStage(stage + 1);
}

bool World::LoadStage(int nextStage)
{
	// [S12] World::LoadStage: 맵·배치를 먼저 준비해 실패 시 현재 월드를 유지한다.
	if (finished || nextStage < 1 || nextStage > 4)
		return false;
	TileMap loadedMap;
	if (!loadedMap.Load(nextStage))
		return false;
	auto loadedPlayers = players;
	std::array<bool, MAX_PLAYERS> placed{};
	std::array<bool, MAX_PLAYERS> revived{};
	for (int id = 0; id < MAX_PLAYERS; ++id) {
		auto& player = loadedPlayers[id];
		if (!player.used)
			continue;
		const auto spawn = GameData::PlayerSpawns[nextStage - 1];
		const Vector2D<float> desired(spawn.x + id * GameData::PlayerSpawnSpacing, spawn.y);
		auto safe = [&](Vector2D<float> pos) {
			if (IsPlayerBlocked(loadedMap, pos, player.size))
				return false;
			for (int other = 0; other < MAX_PLAYERS; ++other)
				if (placed[other] && CollisionCheck::RectRect(pos, player.size, loadedPlayers[other].pos, loadedPlayers[other].size))
					return false;
			return true;
		};
		auto position = desired;
		if (!safe(position)) {
			double closest = std::numeric_limits<double>::max();
			for (int row = 0; row < TileMap::Rows; ++row)
				for (int column = 0; column < TileMap::Columns; ++column) {
					const Vector2D<float> candidate(static_cast<float>(column * TileMap::TileExtent + TileMap::HalfExtent),
						static_cast<float>(row * TileMap::TileExtent + TileMap::HalfExtent));
					const auto offset = candidate - desired;
					const double distance = static_cast<double>(offset.x) * offset.x + static_cast<double>(offset.y) * offset.y;
					if (distance < closest && safe(candidate)) {
						closest = distance;
						position = candidate;
					}
				}
			if (closest == std::numeric_limits<double>::max())
				return false;
		}
		revived[id] = player.hp <= 0;
		if (revived[id])
			player.hp = static_cast<std::int16_t>(GameData::Players[player.charType].hp);
		player.pos = player.lastPos = position;
		player.state = GameData::PLAYER_IDLE;
		player.aimPos = position + Vector2D<float>(1, 0);
		player.angle = player.swordAngle = 0;
		player.rollDir = {};
		player.rollTime = player.fireCooldown = player.swordTime = 0;
		player.invulnerableTime = revived[id] ? GameData::PlayerInvulnerableSeconds : 0;
		player.attackRequested = player.interactRequested = player.swordPending = false;
		placed[id] = true;
	}
	std::unique_ptr<ServerBoss> loadedBoss;
	if (nextStage == 4) {
		loadedBoss = std::make_unique<ServerBoss>();
		loadedBoss->active = true;
		loadedBoss->hp = GameData::BossHp;
		loadedBoss->radius = GameData::BossRadius;
		loadedBoss->pos = { GameData::BossSpawn.x, GameData::BossSpawn.y };
		loadedBoss->handPos = { loadedBoss->pos, loadedBoss->pos };
	}
	map = std::move(loadedMap);
	players = std::move(loadedPlayers);
	enemies.clear();
	bullets.clear();
	drops.clear();
	boss = std::move(loadedBoss);
	stage = nextStage;
	phase = 1;
	spawnedStage = spawnedPhase = 0;
	portalOpen = false;
	portalPos = {};
	portalReady = {};
	S2C_STAGE_LOAD_PACKET packet{};
	packet.size = sizeof(packet);
	packet.type = S2C_STAGE_LOAD;
	packet.stage = static_cast<UINT8>(stage);
	PushEvent(&packet, sizeof(packet));
	for (int id = 0; id < MAX_PLAYERS; ++id)
		if (revived[id]) {
			S2C_PLAYER_EVENT_PACKET event{};
			event.size = sizeof(event);
			event.type = S2C_PLAYER_EVENT;
			event.playerId = static_cast<UINT8>(id);
			event.event = PE_REVIVE;
			event.value = static_cast<UINT8>(players[id].hp);
			PushEvent(&event, sizeof(event));
		}
	SpawnWave();
	return true;
}

bool World::CheckGameEnd()
{
	// [S12] World::CheckGameEnd: 전멸·전원 이탈을 먼저 확인하고 보스 클리어를 판정한다.
	if (finished)
		return true;
	if (stage == 0)
		return false;
	const bool hasSurvivor = std::any_of(players.begin(), players.end(), [](const auto& player) {
		return player.used && player.hp > 0;
	});
	const bool cleared = stage == 4 && boss && boss->hp <= 0;
	if (hasSurvivor && !cleared)
		return false;
	finished = true;
	// 마지막 피격·사망 이벤트와 상태는 결과 패킷보다 먼저 전달한다.
	FlushEvents();
	SendSnapshot();
	EndGame(hasSurvivor ? RESULT_CLEAR : RESULT_WIPE);
	return true;
}

void World::RemoveLeftPlayers()
{
	// [S12] World::RemoveLeftPlayers: 세션 정보는 잠금 안에서 복사하고 월드는 밖에서 정리한다.
	std::vector<UINT8> left;
	std::array<bool, MAX_PLAYERS> connected{};
	{
		std::lock_guard<std::mutex> lock(room.lock);
		left.swap(room.leftPlayers);
		for (const auto& session : room.sessions)
			if (session.used && session.connected && session.playerId >= 0 && session.playerId < MAX_PLAYERS)
				connected[session.playerId] = true;
	}
	for (int id = 0; id < MAX_PLAYERS; ++id) {
		if (!players[id].used || (connected[id] && std::find(left.begin(), left.end(), id) == left.end()))
			continue;
		players[id] = {};
		portalReady[id] = false;
		for (auto& enemy : enemies)
			if (enemy->targetPlayerId == id) {
				enemy->targetPlayerId = -1;
				enemy->dir = {};
			}
		if (boss && boss->targetPlayerId == id)
			boss->targetPlayerId = -1;
	}
}

void World::PushEvent(const void* packet, int size)
{
	// [S13] Protocol.h 이벤트 패킷을 소유하는 바이트 배열로 복사한다.
	if (!packet || size < static_cast<int>(sizeof(PacketHeader)) || size > MAX_PACKET_SIZE)
		return;
	PacketHeader header{};
	std::memcpy(&header, packet, sizeof(header));
	if (header.size != size || (header.type != S2C_STAGE_LOAD &&
		(header.type < S2C_BULLET_SPAWN || header.type > S2C_PLAYER_LEFT)))
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
