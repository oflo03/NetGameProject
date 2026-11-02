#include "Collision.h"

//	wall, player, rolled_player, enemy, playerBullet, enemyBullet, playerMelee, damaged_player, dropitem, water
bool collisionable[10][10]
{
	0,0,0,0,0,0,0,0,0,0,
	1,0,0,1,0,1,0,0,1,1,
	1,0,0,1,0,0,0,0,1,1,
	1,1,1,1,1,0,1,1,0,1,
	1,0,0,1,0,0,0,0,0,0,
	1,1,0,0,0,0,1,0,0,0,
	0,0,0,0,0,0,0,0,0,0,
	1,0,0,1,0,0,0,0,1,1,
	0,1,1,0,0,0,0,1,0,0,
	0,0,0,0,0,0,0,0,0,0
};

std::queue<CollisionMessage> collisionMsg;
std::unordered_set<Master*> deleteSet;

void Collider::detection()
{
	for (auto& other : COLL) {
		if (this == other) continue;
		if (!collisionable[this->layer][other->layer]) continue;
		if (this->shape == rect) {
			if (other->shape == rect) {
				if (this->pos - other->pos <= this->size + other->size) {
					collisionMsg.emplace(CollisionMessage(this->owner, other->layer, other->damage));
				}
			}
			else if (other->shape == circle) {
				if (((this->pos - other->pos <= this->size + other->size) ||
					((this->pos - other->pos) > this->size) && ((this->pos - other->pos).Vabs() - this->size.Vabs()).GetLenth() < other->size.x)) {
					collisionMsg.emplace(CollisionMessage(this->owner, other->layer, other->damage));
				}
			}
		}
		else if (this->shape == circle) {
			if (other->shape == rect) {
				if (((this->pos - other->pos <= this->size + other->size) ||
					((this->pos - other->pos) > other->size) && ((this->pos - other->pos).Vabs() - other->size.Vabs()).GetLenth() < this->size.x)) {
					collisionMsg.emplace(CollisionMessage(this->owner, other->layer, other->damage));
				}
			}
			else if (other->shape == circle) {
				if ((this->pos - other->pos).GetLenth() <= this->size.x + other->size.x) {
					collisionMsg.emplace(CollisionMessage(this->owner, other->layer, other->damage));
				}
			}
		}
	}
}

HBRUSH GREENB = CreateSolidBrush(RGB(0, 255, 0));
HPEN GREENP = CreatePen(PS_SOLID, 1, RGB(0, 255, 0));

void Collider::draw_range(HDC mDC)
{
	RECT FR(pos.x - size.x, pos.y - size.y, pos.x + size.x, pos.y + size.y);
	if (this->shape == rect)
		FrameRect(mDC, &FR, GREENB);
	else if (this->shape == circle)
	{
		HPEN old = (HPEN)SelectObject(mDC, GREENP);
		Arc(mDC, FR.left, FR.top, FR.right, FR.bottom, pos.x, FR.top, pos.x, FR.top);
		SelectObject(mDC, old);
	}
}

void Collider::Clear()
{
	while (!COLL.empty())
	{
		if ((*COLL.begin())->layer != player && (*COLL.begin())->layer != damaged_player && (*COLL.begin())->layer != rolled_player)
			delete (*COLL.begin());
		COLL.erase(COLL.begin());
	}
}

void Master::releaseCollider()
{
	std::erase(COLL, col);
	delete col;
	col = nullptr;
}

void ColliderUpdate()
{
	for (auto& c : COLL)
		c->detection();
	while (!collisionMsg.empty())
	{
		if (collisionMsg.front().collided && collisionMsg.front().collided->col != nullptr)
			collisionMsg.front().collided->handle_collision(collisionMsg.front().otherLayer, collisionMsg.front().damage);
		collisionMsg.pop();
	}
	while (!deleteSet.empty())
	{
		delete (*deleteSet.begin());
		deleteSet.erase(deleteSet.begin());
	}
}

bool isWallCollision(Vector2D<float> pos, Vector2D<float> size)
{
	for (auto& other : COLL) {
		if (other->layer != wall && other->layer != water) continue;
		if (pos - other->pos <= size + other->size)
			return true;
	}
	return false;
}
