#pragma once
#include "Common.h"
#include "Protocol.h"

class EntityView
{
	friend class PlayState;
protected:
	Vector2D<float> previousPosition{};
	Vector2D<float> currentPosition{};
	bool hasPosition = false;

public:
	Vector2D<float> Interpolate(float alpha) const;
};

class PlayerView : public EntityView
{
	friend class PlayState;
private:
	PlayerSnap snapshot{};

public:
	void update();
	void draw(HDC dc);
};

class EnemyView : public EntityView
{
	friend class PlayState;
private:
	EnemySnap snapshot{};

public:
	void update();
	void draw(HDC dc);
};

class BossView : public EntityView
{
	friend class PlayState;
private:
	BossSnap snapshot{};

public:
	void update();
	void draw(HDC dc);
};

class BulletView
{
private:
	BulletSpawn spawn{};
	Vector2D<float> position{};

public:
	void update();
	void draw(HDC dc);
};

class DropView
{
private:
	S2C_DROP_PACKET drop{};

public:
	void draw(HDC dc);
};
