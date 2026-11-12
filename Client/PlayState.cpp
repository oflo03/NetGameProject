#include "PlayState.h"

void PlayState::update()
{
	// [C5] update: 서버에서 받은 상태로 화면 갱신
}

void PlayState::handle_events()
{
	// [C7] handle_events: 서버로 보낼 입력 처리
}

void PlayState::draw()
{
	// [C9] draw: 서버 상태를 바탕으로 화면 그리기
}

void PlayState::OnBulletSpawn(const S2C_BULLET_SPAWN_PACKET& packet)
{
	// [C6] OnBulletSpawn: 총알 생성 연출
}

void PlayState::OnBulletRemove(const S2C_BULLET_REMOVE_PACKET& packet)
{
	// [C6] OnBulletRemove: 총알 제거와 피격 연출
}

void PlayState::OnEnemySpawn(const S2C_ENEMY_SPAWN_PACKET& packet)
{
	// [C6] OnEnemySpawn: 적 등장 연출
}

void PlayState::OnEnemyDead(const S2C_ENEMY_DEAD_PACKET& packet)
{
	// [C6] OnEnemyDead: 적 사망 연출
}

void PlayState::OnPlayerEvent(const S2C_PLAYER_EVENT_PACKET& packet)
{
	// [C6] OnPlayerEvent: 플레이어 이벤트 연출
}

void PlayState::OnDrop(const S2C_DROP_PACKET& packet)
{
	// [C6] OnDrop: 드랍 아이템 생성·제거
}

void PlayState::OnEffect(const S2C_EFFECT_PACKET& packet)
{
	// [C6] OnEffect: 이펙트와 사운드 재생
}

void PlayState::OnBoss(const S2C_BOSS_PACKET& packet)
{
	// [C6] OnBoss: 보스 패턴 연출
}

void PlayState::OnPortal(const S2C_PORTAL_PACKET& packet)
{
	// [C6] OnPortal: 포탈 표시
}

void PlayState::OnStageLoad(const S2C_STAGE_LOAD_PACKET& packet)
{
	// [C6] OnStageLoad: 맵 교체와 보스 컷신
}

void PlayState::SampleInput()
{
	// [C7] SampleInput: 키보드·마우스 상태 수집
}

Vector2D<float> PlayState::ScreenToWorld(POINT point) const
{
	// [C7] ScreenToWorld: 화면 좌표를 맵 좌표로 변환
	return {};
}

void PlayState::SendInput()
{
	// [C7] SendInput: 33ms마다 C2S_INPUT 전송, E키는 C2S_INTERACT
}
