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
	CharType charType = CHAR_MARIN;
	wchar_t nickname[NAME_LEN + 1] = {};
	Vector2D<float> renderPosition{};
	float interpolation = 1.0f;
	int animationState = STATE_IDLE;
	DIRECTION direction = FRONT;
	float animationFrame = 0.0f;

public:
	void update();
	void draw(HDC dc);
};

class EnemyView : public EntityView
{
	friend class PlayState;
private:
	EnemySnap snapshot{};
	EnemyType type = ENEMY_PISTOLMAN;
	bool hasType = false;
	bool dying = false;
	bool finished = false;
	float spawnTime = 0.0f;
	float interpolation = 1.0f;
	float frame = 0.0f;
	int animationState = -1;
	Vector2D<float> renderPosition{};
	const Animation* animation = nullptr;

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
	friend class PlayState;
private:
	BulletSpawn spawn{};
	Vector2D<float> position{};
	Vector2D<float> direction{};
	float age = 0.0f;

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
