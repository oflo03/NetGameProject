#pragma once
#include "Common.h"
#include "Protocol.h"

class EntityView
{
protected:
	Vector2D<float> previousPosition{};
	Vector2D<float> currentPosition{};

public:
	Vector2D<float> Interpolate(float alpha) const;
};

class PlayerView : public EntityView
{
private:
	PlayerSnap snapshot{};

public:
	void update();
	void draw(HDC dc);
};

class EnemyView : public EntityView
{
private:
	EnemySnap snapshot{};

public:
	void update();
	void draw(HDC dc);
};

class BossView : public EntityView
{
private:
	BossSnap snapshot{};

public:
	void update();
	void draw(HDC dc);
};
