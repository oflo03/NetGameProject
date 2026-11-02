#pragma once
#include "Player.h"

#define BOSSHP 1000

enum HAND {
	LEFT, RIGHT
};

class BossState;	// 보스 패턴 상태들은 Boss.cpp 안에만 있음

class Boss : public Master
{
private:
	static Boss* instance;
	Animation body;
	Animation hand[2];
	CImage shadow;
	BossState* state;
	int HP = BOSSHP;
	Boss();
	~Boss();
public:
	Player* target;
	std::vector<int> pattern;	// 남은 패턴 순서 (뒤에서부터 꺼냄)
	Vector2D<float> pos;
	Vector2D<float> handPos[2];
	float frame;
	int handType[2];
	static Boss* getInstance();
	static void release();
	void draw(HDC mDC);
	int getHP() { return HP; }
	void handle_event();
	void update();
	void handle_collision(int otherLayer, int damage);
};
