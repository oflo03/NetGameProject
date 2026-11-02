#include "Boss.h"
#include <algorithm>
#include "Enemy.h"
#include "Effect.h"
#include "Sound.h"

// ---- 보스 상태 (FSM) ----
// BossIdle -> PatternA~D (pattern 순서대로) -> 다 쓰면 SpawnerPattern -> 다시 섞어서 반복. HP 0이면 BossDie.

class BossState
{
public:
	virtual ~BossState() = default;
	virtual void enter(Boss& boss) = 0;
	virtual BossState* handle_event(Boss& boss) = 0;
	virtual void update(Boss& boss) = 0;
};

class BossIdle : public BossState
{
private:
	float time;
	float dirX;
public:
	BossIdle() { dirX = 0.01, time = 0; }
	virtual void enter(Boss& boss);
	virtual BossState* handle_event(Boss& boss);
	virtual void update(Boss& boss);
};

// 왼손 레이저 조준 사격
class PatternA : public BossState
{
private:
	float time;
	float attackCoolTime;
public:
	PatternA() { time = 0, attackCoolTime = 0; }
	virtual void enter(Boss& boss);
	virtual BossState* handle_event(Boss& boss);
	virtual void update(Boss& boss);
};

// 몸 주위로 회전하는 십자 탄막
class PatternB : public BossState
{
private:
	float time;
	bool isAttack;
public:
	PatternB() { time = 0, isAttack = false; }
	virtual void enter(Boss& boss);
	virtual BossState* handle_event(Boss& boss);
	virtual void update(Boss& boss);
};

// 양손 번갈아 원형 탄막
class PatternC : public BossState
{
private:
	float time;
	float attackCoolTime;
	bool isLeft;
public:
	PatternC() { time = 0, attackCoolTime = 0, isLeft = true; }
	virtual void enter(Boss& boss);
	virtual BossState* handle_event(Boss& boss);
	virtual void update(Boss& boss);
};

// 플레이어 위치에 경고 후 낙뢰
class PatternD : public BossState
{
private:
	float time;
	float attackCoolTime;
	Effect* warning;
	bool isAttack;
	Vector2D<float> pos;
public:
	PatternD() {
		time = 0, attackCoolTime = 0;
		isAttack = false;
		warning = nullptr;
	}
	virtual void enter(Boss& boss);
	virtual BossState* handle_event(Boss& boss);
	virtual void update(Boss& boss);
};

// 잡몹 소환
class SpawnerPattern : public BossState
{
private:
	int time;
public:
	SpawnerPattern() { time = 0; }
	virtual void enter(Boss& boss);
	virtual BossState* handle_event(Boss& boss);
	virtual void update(Boss& boss) { time++; }
};

class BossDie : public BossState
{
private:
	float dirX;
public:
	BossDie() { dirX = 0.01; }
	virtual void enter(Boss& boss);
	virtual BossState* handle_event(Boss& boss);
	virtual void update(Boss& boss);
};

// -- BossIdle --

void BossIdle::enter(Boss& boss)
{
	boss.handType[LEFT] = 1;
	boss.handType[RIGHT] = 0;
	boss.frame = 0;
}

BossState* BossIdle::handle_event(Boss& boss)
{
	if ((int)dirX == (int)13.14) {
		if (boss.pattern.empty())
			return new SpawnerPattern();
		int num = boss.pattern.back();
		boss.pattern.pop_back();
		if (num == 0)
			return new PatternA();
		else if (num == 1)
			return new PatternB();
		else if (num == 2)
			return new PatternC();
		else if (num == 3)
			return new PatternD();
	}
	if (boss.getHP() <= 0)
		return new BossDie;
	return nullptr;
}

void BossIdle::update(Boss& boss)
{
	boss.pos.y += std::sin(dirX) / 2;
	for (int i = 0; i < 2; i++)
		boss.handPos[i].y += std::sin(dirX) / 2;
	dirX += 0.05;
}

// -- PatternA --

void PatternA::enter(Boss& boss)
{
	boss.handType[LEFT] = 0;
	boss.handType[RIGHT] = 1;
	boss.frame = 2;
}

BossState* PatternA::handle_event(Boss& boss)
{
	if ((int)time == 600) {
		return new BossIdle();
	}
	else if ((int)time >= 25 && (int)time < 575) {
		if ((int)attackCoolTime == 0) {
			EffectManager::getInstance()->set_effect(new Effect(CEffect::PATTERNA, Vector2D<float>(boss.handPos[0].x, boss.handPos[0].y - 40)));
		}
		else if ((int)attackCoolTime == 8) {
			Vector2D<float> t(boss.target->GetPos() - Vector2D<float>(boss.handPos[0].x, boss.handPos[0].y - 40));
			t.Normalize();
			Bullets.emplace_back(new Bullet(BOSSBULLET2, enemyBullet, Vector2D<float>(boss.handPos[0].x, boss.handPos[0].y - 40) + (t * 5), t * 10));
			SoundManager::getInstance()->play(BOSSLAZOR);
		}
	}
	if (boss.getHP() <= 0)
		return new BossDie;
	return nullptr;
}

void PatternA::update(Boss& boss)
{
	if ((int)time < 25)
		boss.handPos[0].y -= 4;
	else if ((int)time >= 25 && (int)time < 550) {
		if ((int)attackCoolTime < 8) {
			attackCoolTime = (attackCoolTime + frame_time * 3 * 8);
			if (SoundManager::getInstance()->isPlaying(BOSSLOCKON))
				SoundManager::getInstance()->play(BOSSLOCKON);
		}
		else
			attackCoolTime = -5;
	}
	else if ((int)time >= 575) {
		boss.handPos[0].y += 4;
	}
	time++;
}

// -- PatternB --

void PatternB::enter(Boss& boss)
{
	boss.handType[LEFT] = 3;
	boss.handType[RIGHT] = 3;
	boss.frame = 1;
}

BossState* PatternB::handle_event(Boss& boss)
{
	if ((int)time == 10 && !isAttack) {
		Vector2D<float> t(10, 0);
		Vector2D<float> p = boss.pos;
		for (int i = 0, angle = 0; i < 4; i++, angle += 90) {
			for (int j = 0, x = 0; j < 40; j++, x += 30) {
				Bullets.emplace_back(new Bullet(BOSSBULLET1, enemyBullet, Vector2D<float>(t.x + x, t.y).Rotate(angle) + p, boss.pos));
			}
		}
		isAttack = true;
	}
	else if ((int)time == 130) {
		return new BossIdle();
	}
	if (boss.getHP() <= 0)
		return new BossDie;
	return nullptr;
}

void PatternB::update(Boss& boss)
{
	if ((int)time < 4) {
		boss.handPos[0].x += 1;
		boss.handPos[RIGHT].x -= 1;
	}
	else if ((int)time >= 126) {
		boss.handPos[0].x -= 1;
		boss.handPos[RIGHT].x += 1;
	}
	time = (time + frame_time * 10);
}

// -- PatternC --

void PatternC::enter(Boss& boss)
{
	boss.handType[LEFT] = 2;
	boss.handType[RIGHT] = 2;
	boss.frame = 0;
}

BossState* PatternC::handle_event(Boss& boss)
{
	if ((int)time == 600) {
		return new BossIdle();
	}
	else if ((int)time >= 25 && (int)time < 575) {
		if ((int)attackCoolTime == 0) {
			EffectManager::getInstance()->set_effect(new Effect(CEffect::PATTERNC, Vector2D<float>(boss.handPos[isLeft].x, boss.handPos[isLeft].y)));
		}
		else if ((int)attackCoolTime == 9) {
			Vector2D<float> p = boss.handPos[isLeft];
			Vector2D<float> t(2, 0);
			for (int i = 0; i < 40; i++) {
				Bullets.emplace_back(new Bullet(BOSSBULLET3, enemyBullet, t.Rotate(9) + p, t * 2));
			}
			isLeft = !isLeft;
		}
	}
	if (boss.getHP() <= 0)
		return new BossDie;
	return nullptr;
}

void PatternC::update(Boss& boss)
{
	if ((int)time < 25) {
		boss.handPos[0] += Vector2D<float>(-1, -2);
		boss.handPos[1] += Vector2D<float>(+2, 1);
	}
	else if ((int)time >= 25 && (int)time < 575) {
		if ((int)attackCoolTime < 9)
			attackCoolTime = (attackCoolTime + frame_time * 2 * 9);
		else
			attackCoolTime = -10;
	}
	else if ((int)time >= 575) {
		boss.handPos[0] += Vector2D<float>(1, 2);
		boss.handPos[1] += Vector2D<float>(-2, -1);
	}
	time++;
}

// -- PatternD --

void PatternD::enter(Boss& boss)
{
	boss.handType[LEFT] = 1;
	boss.handType[RIGHT] = 2;
	boss.frame = 3;
}

BossState* PatternD::handle_event(Boss& boss)
{
	if ((int)time == 600) {
		return new BossIdle();
	}
	else if ((int)time >= 25 && (int)time < 575) {
		if ((int)attackCoolTime == 0 && !isAttack) {
			warning = new Effect(CEffect::PATTERND, Vector2D<float>(boss.target->GetPos().x, boss.target->GetPos().y + 22 - Effect::effect[CEffect::PATTERND].size.bottom));
			EffectManager::getInstance()->set_effect(warning);
			SoundManager::getInstance()->play(BOSSCHARGING);
			isAttack = true;
		}
		else if ((int)attackCoolTime == 5) {
			EffectManager::getInstance()->set_effect(new Effect(CEffect::PATTERNB, Vector2D<float>(boss.target->GetPos().x, boss.target->GetPos().y + 22 - Effect::effect[CEffect::PATTERNB].size.bottom)));
			isAttack = false;
			SoundManager::getInstance()->play(BOSSLIGHTNING);
			RECT t;
			RECT r = { pos.x - Effect::effect[CEffect::PATTERNB].size.right,pos.y - 5,pos.x + Effect::effect[CEffect::PATTERNB].size.right,pos.y + 20 };
			for (auto& other : COLL) {
				for (int i = 0; i < 4; i++) {
					if (other->layer == player) {
						RECT rect = { other->pos.x - other->size.x, other->pos.y - other->size.y ,other->pos.x + other->size.x, other->pos.y + other->size.y };
						if (IntersectRect(&t, &rect, &r)) {
							other->owner->handle_collision(enemyBullet, 1);
							break;
						}
					}
				}
			}
		}
	}
	if (boss.getHP() <= 0)
		return new BossDie;
	return nullptr;
}

void PatternD::update(Boss& boss)
{
	if ((int)time < 25) {
		boss.handPos[0].x += 1;
		boss.handPos[1].x -= 1;
	}
	else if ((int)time >= 25 && (int)time < 575) {
		if ((int)attackCoolTime >= 0 && (int)attackCoolTime < 4) {
			warning->pos = Vector2D<float>(boss.target->GetPos().x, boss.target->GetPos().y + 22 - Effect::effect[CEffect::PATTERND].size.bottom);
			attackCoolTime = (attackCoolTime + frame_time * 2 * 5);
			pos = boss.target->GetPos();
		}
		else if ((int)attackCoolTime == 5)
			attackCoolTime = -10;
		else if ((int)attackCoolTime < 0 || (int)attackCoolTime >= 4 && (int)attackCoolTime < 5)
			attackCoolTime = (attackCoolTime + frame_time * 2 * 5);
	}
	else if ((int)time >= 575) {
		boss.handPos[0].x -= 1;
		boss.handPos[1].x += 1;
	}
	time++;
}

// -- SpawnerPattern --

void SpawnerPattern::enter(Boss& boss)
{
	boss.handType[LEFT] = 1;
	boss.handType[RIGHT] = 0;
	boss.frame = 3;
}

BossState* SpawnerPattern::handle_event(Boss& boss)
{
	if (time == 600) {
		boss.pattern.push_back(3);
		boss.pattern.push_back(2);
		boss.pattern.push_back(1);
		boss.pattern.push_back(0);
		std::shuffle(boss.pattern.begin(), boss.pattern.end(), rd);
		return new BossIdle();
	}
	if (time == 50) {
		EnemyManager* spawner = EnemyManager::getInstance();
		spawner->spawn(PISTOLMAN, 260, 310);
		spawner->spawn(RIFLEMAN, 1700, 310);
		spawner->spawn(SHOTGUNMAN, 260, 780);
		spawner->spawn(BOMBSHE, 950, 880);
		spawner->spawn(BAT, 1400, 780);
	}
	if (boss.getHP() <= 0)
		return new BossDie;
	return nullptr;
}

// -- BossDie --

void BossDie::enter(Boss& boss)
{
	boss.frame = 4;
	boss.handType[LEFT] = 4;
	boss.handType[RIGHT] = 4;
}

BossState* BossDie::handle_event(Boss& boss)
{
	if ((int)dirX == (int)13.14) {
		EffectManager::getInstance()->set_effect(new Effect(TELEPORT, boss.pos));
	}
	return nullptr;
}

void BossDie::update(Boss& boss)
{
	boss.pos.y += std::sin(dirX) / 2;
	for (int i = 0; i < 2; i++)
		boss.handPos[i].y += std::sin(dirX) / 2;
	dirX += 0.05;
}

// ---- Boss ----

Boss* Boss::instance = nullptr;

Boss::Boss()
{
	LoadAnimation(body, L"resources/boss_body.png", 5, 0);
	LoadAnimation(hand[LEFT], L"resources/boss_lefthand.png", 5, 0);
	LoadAnimation(hand[RIGHT], L"resources/boss_righthand.png", 5, 0);
	shadow.Load(L"resources/shadow.png");
	state = new BossIdle();
	pattern.push_back(3);
	pattern.push_back(2);
	pattern.push_back(1);
	pattern.push_back(0);
	frame = 0;
	target = Player::getInstance();
	pos.x = 960;
	pos.y = 500;
	handType[LEFT] = 1;
	handType[RIGHT] = 0;
	handPos[0].x = pos.x - 60;
	handPos[0].y = pos.y - 10;
	handPos[1].x = pos.x + 60;
	handPos[1].y = pos.y - 10;
	col = new Collider(body.size.right - 50);
	col->owner = this;
	col->layer = enemy;
	col->pos = pos;
	col->damage = 5;
	COLL.emplace_back(col);
}

Boss::~Boss()
{
	delete state;
}

Boss* Boss::getInstance()
{
	if (instance == nullptr)
		instance = new Boss();
	return instance;
}

void Boss::release()
{
	if (instance != nullptr)
		delete instance;
	instance = nullptr;
}

void Boss::draw(HDC mDC)
{
	shadow.Draw(mDC, 960 - shadow.GetWidth() * 2, 500 + body.size.bottom - 2 - shadow.GetHeight() * 2, shadow.GetWidth() * 4, shadow.GetHeight() * 4);
	body.resource.Draw(mDC, pos.x - body.size.right, pos.y - body.size.bottom, body.size.right * 2, body.size.bottom * 2, body.size.right * frame, 0, body.size.right, body.size.bottom);
	for (int i = 0; i < 2; i++)
		hand[i].resource.Draw(mDC, handPos[i].x - hand[i].size.right, handPos[i].y - hand[i].size.bottom, hand[i].size.right * 2, hand[i].size.bottom * 2, hand[i].size.right * handType[i], 0, hand[i].size.right, hand[i].size.bottom);
}

void Boss::handle_event()
{
	BossState* temp = this->state->handle_event(*this);
	if (temp != nullptr) {
		delete this->state;
		this->state = temp;
		this->state->enter(*this);
	}
}

void Boss::update()
{
	state->update(*this);
	if (col != nullptr)
		col->pos = pos;
}

void Boss::handle_collision(int otherLayer, int damage)
{
	switch (otherLayer)
	{
	case playerMelee:
	case playerBullet:
		if (otherLayer == playerMelee)
			EffectManager::getInstance()->set_effect(new Effect(CEffect::SWORDATTACK, col->pos));
		HP -= damage;
		if (HP <= 0)
			releaseCollider();
		break;
	default:
		break;
	}
}
