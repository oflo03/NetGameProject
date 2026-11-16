#pragma once
#include "Framework.h"
#include "Common.h"
#include "Protocol.h"

// 서버 상태를 표시하는 온라인 플레이 화면
class PlayState : public GameState
{
public:
	void update() override;
	void handle_events() override;
	void draw() override;

private:
	C2S_INPUT_PACKET input{};

	void OnBulletSpawn(const S2C_BULLET_SPAWN_PACKET& packet);
	void OnBulletRemove(const S2C_BULLET_REMOVE_PACKET& packet);
	void OnEnemySpawn(const S2C_ENEMY_SPAWN_PACKET& packet);
	void OnEnemyDead(const S2C_ENEMY_DEAD_PACKET& packet);
	void OnPlayerEvent(const S2C_PLAYER_EVENT_PACKET& packet);
	void OnDrop(const S2C_DROP_PACKET& packet);
	void OnEffect(const S2C_EFFECT_PACKET& packet);
	void OnBoss(const S2C_BOSS_PACKET& packet);
	void OnPortal(const S2C_PORTAL_PACKET& packet);
	void OnStageLoad(const S2C_STAGE_LOAD_PACKET& packet);

	void SampleInput();
	Vector2D<float> ScreenToWorld(POINT point) const;
	void SendInput();
};
