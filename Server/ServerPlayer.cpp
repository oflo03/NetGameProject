#include "ServerPlayer.h"
#include "Session.h"
#include "GameData.h"
#include "World.h"
#include <algorithm>
#include <cmath>
#include <random>
#include "Bullet.h"

void ServerPlayer::ApplyInput(const InputState& input, float deltaTime, World& world)
{
	// [S10] 이동 속도와 구르기 시간을 서버의 초 단위로 계산한다.
	attackRequested = false;
	if (!used)
		return;
	lastPos = pos;
	if (hp <= 0) {
		state = GameData::PLAYER_DEAD;
		return;
	}
	fireCooldown = (std::max)(0.0f, fireCooldown - deltaTime);
	invulnerableTime = (std::max)(0.0f, invulnerableTime - deltaTime);
	swordTime = (std::max)(0.0f, swordTime - deltaTime);
	interactRequested = input.interact;
	if (std::isfinite(input.aimX) && std::isfinite(input.aimY)) {
		aimPos = Vector2D<float>(input.aimX, input.aimY);
		const double aimX = static_cast<double>(aimPos.x) - pos.x;
		const double aimY = static_cast<double>(aimPos.y) - pos.y;
		if (aimX != 0 || aimY != 0)
			angle = static_cast<float>(std::atan2(aimY, aimX) * 180.0 / M_PI);
	}
	Vector2D<float> direction;
	direction.x = (input.keys & KEY_RIGHT) ? 1.0f : (input.keys & KEY_LEFT) ? -1.0f : 0.0f;
	direction.y = (input.keys & KEY_UP) ? -1.0f : (input.keys & KEY_DOWN) ? 1.0f : 0.0f;
	const float length = std::hypot(direction.x, direction.y);
	if (length > 0)
		direction /= length;
	if (rollTime <= 0 && (input.keys & KEY_ROLL) && length > 0) {
		rollTime = GameData::RollDurationSeconds;
		rollDir = direction;
	}
	if (rollTime > 0) {
		const float movingTime = (std::clamp)(rollTime - (GameData::RollDurationSeconds - GameData::RollMoveSeconds), 0.0f, deltaTime);
		pos += rollDir * (moveSpeedPerSecond * GameData::RollSpeedMultiplier * movingTime);
		rollTime = (std::max)(0.0f, rollTime - deltaTime);
		if (rollTime <= 0.000001f)
			rollTime = 0;
		state = rollTime > 0.000001f ? GameData::PLAYER_ROLL : GameData::PLAYER_IDLE;
		return;
	}
	pos += direction * (moveSpeedPerSecond * deltaTime);
	state = invulnerableTime > 0 ? GameData::PLAYER_DAMAGED : length > 0 ? GameData::PLAYER_RUN : GameData::PLAYER_IDLE;

	// [S10] 탄약이 있는 무기만 선택하며 교체로 발사 대기를 줄이지 않는다.
	if (weapon >= WEAPON_COUNT)
		weapon = WEAPON_SWORD;
	const auto previousWeapon = weapon;
	if (input.weapon < WEAPON_COUNT) {
		if (input.weapon == WEAPON_SWORD || ammo[input.weapon] > 0)
			weapon = static_cast<std::uint8_t>(input.weapon);
	} else if (input.keys & (KEY_WEAPON_PREV | KEY_WEAPON_NEXT)) {
		const int step = (input.keys & KEY_WEAPON_PREV) ? -1 : 1;
		for (int count = 0; count < WEAPON_COUNT; ++count) {
			weapon = static_cast<std::uint8_t>((weapon + WEAPON_COUNT + step) % WEAPON_COUNT);
			if (weapon == WEAPON_SWORD || ammo[weapon] > 0)
				break;
		}
	}
	if (weapon != WEAPON_SWORD && ammo[weapon] == 0 && fireCooldown <= 0.000001f)
		weapon = WEAPON_SWORD;
	if (weapon != previousWeapon)
		fireCooldown = (std::max)(fireCooldown, GameData::Weapons[weapon].cooldownSeconds - GameData::WeaponSwitchLeadSeconds);
	attackRequested = (input.keys & KEY_ATTACK) != 0;
}

bool ServerPlayer::TryFire(World& world)
{
	// [S10] ServerPlayer::TryFire: 발사·탄약·쿨타임을 서버에서 결정한다.
	if (!used || hp <= 0 || rollTime > 0 || weapon >= WEAPON_COUNT || fireCooldown > 0.000001f)
		return false;
	const auto& stats = GameData::Weapons[weapon];
	if (weapon == WEAPON_SWORD) {
		swordPending = true;
		swordTime = GameData::SwordWindupSeconds;
		swordAngle = angle;
		S2C_PLAYER_EVENT_PACKET packet{};
		packet.size = sizeof(packet);
		packet.type = S2C_PLAYER_EVENT;
		packet.playerId = playerId;
		packet.event = PE_SWORD;
		world.PushEvent(&packet, sizeof(packet));
		fireCooldown = stats.cooldownSeconds;
		return true;
	}
	if (ammo[weapon] == 0)
		return false;
	const double aimX = static_cast<double>(aimPos.x) - pos.x;
	const double aimY = static_cast<double>(aimPos.y) - pos.y;
	angle = static_cast<float>(std::atan2(aimY, aimX) * 180.0 / M_PI);
	const auto hand = pos + Vector2D<float>(angle > -90 && angle < 90 ? 18.0f : -17.0f, 8.0f);
	const double dx = static_cast<double>(aimPos.x) - hand.x;
	const double dy = static_cast<double>(aimPos.y) - hand.y;
	const double distance = std::hypot(dx, dy);
	if (distance == 0 || !std::isfinite(distance))
		return false;
	const Vector2D<float> heading(static_cast<float>(dx / distance), static_cast<float>(dy / distance));
	static thread_local std::mt19937 random(std::random_device{}());
	std::uniform_int_distribution<int> spread(-stats.halfSpreadDegrees, stats.halfSpreadDegrees);
	int spawned = 0;
	for (int pellet = 0; pellet < stats.projectileCount; ++pellet) {
		const float degrees = stats.projectileCount == 1 ? static_cast<float>(spread(random)) :
			static_cast<float>(-stats.halfSpreadDegrees + pellet * 2 * stats.halfSpreadDegrees / (stats.projectileCount - 1));
		auto direction = heading;
		direction.Rotate(degrees);
		const auto muzzle = hand + direction * 30.0f + Vector2D<float>(0, -10);
		const float scale = weapon == WEAPON_ROCKET ? 5.0f : 10.0f;
		if (auto* bullet = world.SpawnBullet(stats.bulletType, GameData::BULLET_SIDE_PLAYER, muzzle, direction * scale)) {
			bullet->lastPos = pos;	// 몸에서 총구 사이에 있는 벽도 검사한다.
			++spawned;
		}
	}
	if (spawned == 0)
		return false;
	--ammo[weapon];
	fireCooldown = stats.cooldownSeconds;
	return true;
}
