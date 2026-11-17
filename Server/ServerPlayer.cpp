#include "ServerPlayer.h"
#include "Session.h"
#include "GameData.h"
#include "World.h"
#include <algorithm>
#include <cmath>

void ServerPlayer::ApplyInput(const InputState& input, float deltaTime, World& world)
{
	// [S10] 이동 속도와 구르기 시간을 서버의 초 단위로 계산한다.
	if (!used)
		return;
	lastPos = pos;
	if (hp <= 0) {
		state = GameData::PLAYER_DEAD;
		return;
	}
	fireCooldown = (std::max)(0.0f, fireCooldown - deltaTime);
	invulnerableTime = (std::max)(0.0f, invulnerableTime - deltaTime);
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

	// [S10] 무기 교체와 공격은 후속 일정에서 구현한다.
}

bool ServerPlayer::TryFire(World& world)
{
	// [S10] ServerPlayer::TryFire: 무기별 발사, 탄약과 발사 대기 시간 갱신
	return false;
}
