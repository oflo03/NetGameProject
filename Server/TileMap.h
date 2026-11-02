#pragma once
#include <vector>
#include "Vector2D.h"

class TileMap
{
public:
	static constexpr int Rows = 17;
	static constexpr int Columns = 30;
	static constexpr int TileExtent = 64;
	static constexpr int HalfExtent = 32;
	static constexpr int WallType = 2;
	static constexpr int WaterType = 4;

	struct BlockedTile
	{
		int type = 0;	// WallType 또는 WaterType
		Vector2D<float> pos;	// 중심 (열 * 64 + 32, 행 * 64 + 32)
		Vector2D<float> size{ HalfExtent, HalfExtent };
	};

	std::vector<BlockedTile> blockedTiles;	// World의 벽 충돌 검사에 사용

	// ../Client/resources/map{stage}.txt, 각 칸은 type tilePos 두 정수
	bool Load(int stage);
};
