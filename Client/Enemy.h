#pragma once
#include "Player.h"

enum enemytype
{
	PISTOLMAN, RIFLEMAN, SHOTGUNMAN, BAT, BOMBSHE
};

// 실제 적 클래스(GunEnemy, Bat, Bombshe)는 Enemy.cpp 안에만 있음. 생성은 EnemyManager::spawn으로.
class Enemy : public Master
{
protected:
	CImage shadow;
	float frame;
	float angle;
	Vector2D<float> pos;
	Vector2D<float> dir;
	Vector2D<float> lastPos;
	int direction;
	int attackCoolTime;
	int state;
	int HP;
	float velocity;
	float attackRange;
	bool targetLocked;
public:
	Player* target;
	Enemy(float x, float y) : pos(x, y), dir(0, 0), frame(0), angle(90), direction(FRONT), state(STATE_IDLE), velocity(0), target(Player::getInstance()), targetLocked(false), attackCoolTime(0) {}
	virtual ~Enemy() {}
	virtual void draw_character(HDC mDC) = 0;
	virtual void handle_event() = 0;
	virtual void update() = 0;
	Vector2D<float> GetPos() const { return pos; }
	bool attackable() {
		return (target != nullptr && (target->GetPos() - pos).GetLenth() <= attackRange);
	}
};

class EnemyManager {
private:
	static EnemyManager* instance;
	std::vector<Enemy*> enemy;
	EnemyManager();
	~EnemyManager();
public:
	static EnemyManager* getInstance();
	void Clear();
	void SetEnemy(int stage);
	void spawn(int type, float x, float y);
	void delete_enemy(Enemy* e);
	bool isclear();
	void handle_event();
	void update();
	void draw(HDC mDC);
};
