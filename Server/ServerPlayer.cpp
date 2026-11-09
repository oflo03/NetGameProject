#include "ServerPlayer.h"

void ServerPlayer::ApplyInput(const InputState& input, float deltaTime, World& world)
{
	// [S10] ServerPlayer::ApplyInput: 이동, 구르기, 무기 교체, 공격 입력 반영
}

bool ServerPlayer::TryFire(World& world)
{
	// [S10] ServerPlayer::TryFire: 무기별 발사, 탄약과 발사 대기 시간 갱신
	return false;
}
