#pragma once
#include <queue>
#include <unordered_set>
#include "Common.h"
#include "CollisionCheck.h"

enum shapetype
{
	point, rect, circle
};

enum layertype
{
	wall, player, rolled_player, enemy, playerBullet, enemyBullet, playerMelee, damaged_player, dropitem, water
};

struct Master;

struct Collider
{
	Vector2D<float> pos;
	Vector2D<float> size;
	int shape;
	int layer;
	int damage;
	Master* owner;
	Collider() : size(0, 0), shape(point) { }
	Collider(float rad) : size(rad, rad), shape(circle) { }
	Collider(Vector2D<float> size) : size(size), shape(rect) { }
	void detection();
	void draw_range(HDC mDC);
	static void Clear();
};

// 충돌체를 가진 모든 오브젝트의 부모
struct Master
{
	Collider* col = nullptr;
	virtual ~Master() = default;
	virtual void handle_collision(int otherLayer, int damage) = 0;
	bool isDead() { return (col == nullptr); }
	void releaseCollider();	// COLL에서 빼고 삭제
};

struct CollisionMessage
{
	Master* collided;
	int otherLayer;
	int damage;
};

extern std::queue<CollisionMessage> collisionMsg;
extern std::unordered_set<Master*> deleteSet;
extern HPEN GREENP;

void ColliderUpdate();
bool isWallCollision(Vector2D<float> pos, Vector2D<float> size);
