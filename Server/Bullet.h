#pragma once
#include <cstdint>
#include "Vector2D.h"

// [S10] 서버 총알 상태. 생성·이동·충돌은 World에서 처리한다.
class Bullet
{
public:
	bool active = false;
	std::uint16_t id = 0;
	std::uint8_t type = 0;	// bullet_table
	std::uint8_t side = 4;	// layertype: playerBullet 4, enemyBullet 5
	Vector2D<float> pos;
	Vector2D<float> lastPos;	// 이동 구간 충돌 검사의 시작점
	Vector2D<float> dir;
	Vector2D<float> orbitCenter;
	float rotationDegrees = 0;
	float speedPerSecond = 0;
	float radius = 0;
	int damage = 0;
};
