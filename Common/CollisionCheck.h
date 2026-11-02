#pragma once
#include "Vector2D.h"

// 위치는 중심, 크기는 절반 크기. 원의 반지름은 size.x다.
namespace CollisionCheck
{
	inline bool RectRect(Vector2D<float> pos, Vector2D<float> size, Vector2D<float> otherPos, Vector2D<float> otherSize)
	{
		return pos - otherPos <= size + otherSize;
	}

	inline bool RectCircle(Vector2D<float> pos, Vector2D<float> size, Vector2D<float> otherPos, Vector2D<float> otherSize)
	{
		return (pos - otherPos <= size + otherSize) ||
			((pos - otherPos) > size) && ((pos - otherPos).Vabs() - size.Vabs()).GetLenth() < otherSize.x;
	}

	inline bool CircleRect(Vector2D<float> pos, Vector2D<float> size, Vector2D<float> otherPos, Vector2D<float> otherSize)
	{
		return (pos - otherPos <= size + otherSize) ||
			((pos - otherPos) > otherSize) && ((pos - otherPos).Vabs() - otherSize.Vabs()).GetLenth() < size.x;
	}

	inline bool CircleCircle(Vector2D<float> pos, Vector2D<float> size, Vector2D<float> otherPos, Vector2D<float> otherSize)
	{
		return (pos - otherPos).GetLenth() <= size.x + otherSize.x;
	}
}
