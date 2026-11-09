#pragma once
#include <cstdint>
#include <memory>
#include "BehaviorTree.h"
#include "GameData.h"
#include "Vector2D.h"

// [S11] 서버 적 상태. 종류별 행동은 tree, 갱신과 피격은 World에서 처리한다.
class ServerEnemy
{
public:
	bool active = false;
	std::uint16_t id = 0;
	GameData::EnemyKind type = GameData::EnemyKind::Pistolman;
	std::uint8_t state = 0;	// 원작 적 종류별 상태 값
	std::int16_t hp = 0;
	Vector2D<float> pos;
	Vector2D<float> lastPos;
	Vector2D<float> dir;
	Vector2D<float> size;		// 충돌 박스 절반 크기, 박쥐는 원 반지름
	float angle = 0;
	float moveSpeedPerSecond = 0;
	float attackRange = 0;
	int targetPlayerId = -1;
	std::uint8_t weapon = 0;		// ID
	int ammo = 0;
	float attackCooldown = 0;	// 남은 시간은 초 단위
	float spawnTime = 0;
	float moveTime = 0;
	std::unique_ptr<BT::Node> tree;
};

std::unique_ptr<BT::Node> BuildGunnerTree();
std::unique_ptr<BT::Node> BuildBatTree();
std::unique_ptr<BT::Node> BuildBansheeTree();
