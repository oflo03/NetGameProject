#pragma once
#include "Framework.h"
#include "Common.h"
#include "Protocol.h"
#include "EntityView.h"
#include <array>
#include <unordered_map>

// 서버 상태를 표시하는 온라인 플레이 화면
class PlayState : public GameState
{
public:
	void update() override;
	void handle_events() override;
	void draw() override;
	void process_packet(const PacketHeader* packet) override;

private:
	std::array<PlayerView, MAX_PLAYERS> playerViews{};
	std::unordered_map<UINT16, EnemyView> enemyViews;
	BossView bossView;
	bool hasSnapshot = false;
	UINT32 snapshotTick = 0;

	void OnSnapshot(const S2C_SNAPSHOT_PACKET& packet);
	C2S_INPUT_PACKET input{{sizeof(C2S_INPUT_PACKET), C2S_INPUT}, {}, WEAPON_KEEP, 0, 0};
	UINT8 previousActions = 0;
	UINT8 previousWeapons = 0;
	bool interactHeld = false;
	bool interactPending = false;
	ULONGLONG lastInputSend = 0;

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
