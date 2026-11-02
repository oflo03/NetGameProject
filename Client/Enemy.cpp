#include "Enemy.h"
#include <string>
#include <algorithm>
#include "Effect.h"
#include "Sound.h"

// 적 등장(소환) 애니메이션 - 모든 적 공용
static Animation spawnAnim;

static void DrawSpawn(HDC mDC, const Vector2D<float>& pos, float frame)
{
	spawnAnim.resource.Draw(mDC, pos.x - spawnAnim.size.right, pos.y - spawnAnim.size.bottom, spawnAnim.size.right * 2, spawnAnim.size.bottom * 2,
		(int)frame * spawnAnim.size.right, 0, spawnAnim.size.right, spawnAnim.size.bottom);
}

// ---- GunEnemy: PistolMan / RifleMan / ShotgunMan ----

class GunEnemy : public Enemy
{
private:
	static Animation animation[3][4][6];	// [type][STATE][DIRECTION]
	int type;	// PISTOLMAN, RIFLEMAN, SHOTGUNMAN
	CImage hand;
	Vector2D<float> handPos;
	Weapon* weapon;
	float moveTime;
	void SetDirection();
	void attack();
public:
	GunEnemy(int type, float x, float y);
	~GunEnemy();
	static void init();
	static void release();
	virtual void draw_character(HDC mDC);
	virtual void handle_event();
	virtual void update();
	virtual void handle_collision(int otherLayer, int damage);
};

struct GunEnemyInfo
{
	int hp;
	int deadFrame;
	int yOffset;	// 그림 세로 위치 보정
	int drop;
};

static const GunEnemyInfo gunInfo[3] = {
	{ 50, 5, 22, DPISTOL },		// PISTOLMAN
	{ 50, 5, 22, DRIFLE },		// RIFLEMAN
	{ 100, 3, 25, DSHOTGUN },	// SHOTGUNMAN
};

// resources/enemy_*.png   [type][STATE][FRONT, FRONT_RIGHT, FRONT_LEFT, BACK, BACK_RIGHT, BACK_LEFT]
static const wchar_t* const gunImage[3][4][6] = {
	{
		{ L"pistol_right", L"pistol_right", L"pistol_left", L"pistol_back", L"pistol_back", L"pistol_back" },
		{ L"pistol_run_front", L"pistol_run_right", L"pistol_run_left", L"pistol_run_back", L"pistol_run_back", L"pistol_run_back" },
		{ L"pistol_damaged_front", L"pistol_damaged_right", L"pistol_damaged_left", L"pistol_damaged_front", L"pistol_damaged_right", L"pistol_damaged_left" },
		{ L"pistol_dead_front", L"pistol_dead_front", L"pistol_dead_front", L"pistol_dead_back", L"pistol_dead_back", L"pistol_dead_back" },
	},
	{
		{ L"rifle_right", L"rifle_right", L"rifle_left", L"rifle_back", L"rifle_back", L"rifle_back" },
		{ L"rifle_run_front", L"rifle_run_front_right", L"rifle_run_front_left", L"rifle_run_back", L"rifle_run_back", L"rifle_run_back" },
		{ L"rifle_damaged_front", L"rifle_damaged_right", L"rifle_damaged_left", L"rifle_damaged_front", L"rifle_damaged_right", L"rifle_damaged_left" },
		{ L"rifle_dead_front", L"rifle_dead_front", L"rifle_dead_front", L"rifle_dead_back", L"rifle_dead_back", L"rifle_dead_back" },
	},
	{
		{ L"shotgun_front", L"shotgun_front", L"shotgun_front", L"shotgun_back", L"shotgun_back", L"shotgun_back" },
		{ L"shotgun_run_front", L"shotgun_run_right", L"shotgun_run_left", L"shotgun_run_back", L"shotgun_run_back", L"shotgun_run_back" },
		{ L"shotgun_damaged_front", L"shotgun_damaged_right", L"shotgun_damaged_left", L"shotgun_damaged_front", L"shotgun_damaged_right", L"shotgun_damaged_left" },
		{ L"shotgun_dead", L"shotgun_dead", L"shotgun_dead", L"shotgun_dead", L"shotgun_dead", L"shotgun_dead" },
	},
};

Animation GunEnemy::animation[3][4][6];

void GunEnemy::init()
{
	for (int t = 0; t < 3; t++)
		for (int d = 0; d < 6; d++) {
			auto path = [&](int s) { return std::wstring(L"resources/enemy_") + gunImage[t][s][d] + L".png"; };
			LoadAnimation(animation[t][STATE_IDLE][d], path(STATE_IDLE).c_str(), 2, 2);
			LoadAnimation(animation[t][STATE_RUN][d], path(STATE_RUN).c_str(), 6, 2);
			LoadAnimation(animation[t][STATE_DAMAGED][d], path(STATE_DAMAGED).c_str(), 2, 3);
			LoadAnimation(animation[t][STATE_DEAD][d], path(STATE_DEAD).c_str(), gunInfo[t].deadFrame, 2);
		}
}

void GunEnemy::release()
{
	for (auto& t : animation)
		for (auto& s : t)
			for (auto& a : s)
				a.resource.Destroy();
}

GunEnemy::GunEnemy(int type, float x, float y) : Enemy(x, y), type(type)
{
	hand.Load(L"resources/enemy_pistol_hand.png");
	shadow.Load(L"resources/shadow.png");
	velocity = 100;
	moveTime = 200;
	HP = gunInfo[type].hp;
	state = STATE_IDLE;
	switch (type) {
	case PISTOLMAN: weapon = new Pistol(); break;
	case RIFLEMAN: weapon = new Rifle(); break;
	default: weapon = new Shotgun(); break;
	}
	weapon->Enemy();
	attackRange = 400;
	attackCoolTime = 0;
	lastPos = pos;
}

GunEnemy::~GunEnemy()
{
	delete weapon;
}

void GunEnemy::draw_character(HDC mDC)
{
	if (!col && HP > 0) {
		DrawSpawn(mDC, pos, frame);
		return;
	}
	handPos = pos;
	if (abs(angle) < 90)
	{
		handPos.x += 18;
		handPos.y += 8;
	}
	else
	{
		handPos.x -= 17;
		handPos.y += 8;
	}
	Animation& anim = animation[type][state][direction];
	float yDest = pos.y - (animation[type][STATE_IDLE][FRONT].size.bottom - gunInfo[type].yOffset) * 2;
	auto drawBody = [&]() {
		anim.resource.Draw(mDC, pos.x - anim.size.right, yDest - 20, anim.size.right * 2, anim.size.bottom * 2,
			(int)frame * anim.size.right, 0, anim.size.right, anim.size.bottom);
	};
	auto drawHand = [&]() {
		weapon->draw_weapon(mDC, handPos, target->GetPos());
		hand.Draw(mDC, handPos.x - hand.GetWidth(), handPos.y - hand.GetHeight(), hand.GetWidth() * 2, hand.GetHeight() * 2);
	};
	if (state == STATE_DEAD) {
		drawBody();
		return;
	}
	shadow.Draw(mDC, pos.x - shadow.GetWidth(), pos.y + col->size.y - 2 - shadow.GetHeight(), shadow.GetWidth() * 2, shadow.GetHeight() * 2);
	if (direction == FRONT || direction == FRONT_RIGHT || direction == FRONT_LEFT) {
		drawBody();
		drawHand();
	}
	else {
		drawHand();
		drawBody();
	}
}

void GunEnemy::handle_event()
{
	if (col) {
		if (!attackable() && state != STATE_DAMAGED) {
			state = STATE_RUN;
			dir = (target->GetPos() - pos).Normalize();
		}
		else {
			attack();
			if (state == STATE_DAMAGED && (int)frame == 2) {
				state = STATE_IDLE;
				dir.x = dir.y = 0;
				moveTime = 0;
				frame = 0;
			}
			if ((int)moveTime == 0) {
				moveTime = ranTime(dre);
				state = ran(dre);
				if (state == STATE_IDLE)
					dir.x = dir.y = 0;
				else if (state == STATE_RUN)
					dir = Vector2D<float>(1, 0).Rotate(rad(dre));
				frame = 0;
			}
		}
		if (weapon->IsRunOut() && attackCoolTime == 0)
			attackCoolTime = 300;
		else if (weapon->IsRunOut() && attackCoolTime == 1)
			weapon->ReLoad();
	}
}

void GunEnemy::update()
{
	if (col || HP <= 0) {
		lastPos = pos;
		Animation& anim = animation[type][state][direction];
		frame = (frame + frame_time * anim.velocity * anim.frame);
		if (state == STATE_DEAD) {
			if ((int)frame >= anim.frame) {
				if (randrop(dre) == 0)
					drops.emplace_back(new DropItem(gunInfo[type].drop, pos));
				if (type == SHOTGUNMAN)
					EffectManager::getInstance()->set_effect(new Effect(CEffect::SHOTGUNDIE, pos));
				EnemyManager::getInstance()->delete_enemy(this);
				deleteSet.insert(this);
			}
		}
		else {
			if (state != STATE_DAMAGED) {
				pos = pos + dir * velocity * frame_time;
				if ((int)moveTime) moveTime--;
				if ((int)frame >= anim.frame) frame = 0;
			}
			SetDirection();
			weapon->update();
			if (attackCoolTime)
				attackCoolTime--;
			col->pos = pos;
		}
	}
	else {
		frame = (frame + frame_time * spawnAnim.velocity * spawnAnim.frame);
		if ((int)frame == spawnAnim.frame) {
			col = new Collider(Vector2D<float>(animation[type][STATE_IDLE][FRONT].size.right, animation[type][STATE_IDLE][FRONT].size.bottom));
			col->owner = this;
			col->layer = enemy;
			col->pos = pos;
			col->damage = 5;
			COLL.emplace_back(col);
			weapon->SetCurTime(100);
			frame = 0;
		}
	}
}

void GunEnemy::SetDirection()
{
	angle = std::atan2(target->GetPos().y - pos.y, target->GetPos().x - pos.x) * (180.0f / M_PI);
	if (angle >= -20 && angle <= 60) {
		direction = FRONT_RIGHT;
	}
	else if (angle > 60 && angle <= 120) {
		direction = FRONT;
	}
	else if (angle >= 120 && angle <= 180 || angle > -180 && angle < -160) {
		direction = FRONT_LEFT;
	}
	else if (angle >= -160 && angle <= -120) {
		direction = BACK_LEFT;
	}
	else if (angle >= -120 && angle <= -60) {
		direction = BACK;
	}
	else if (angle >= -60 && angle < -20) {
		direction = BACK_RIGHT;
	}
}

void GunEnemy::attack()
{
	weapon->attack(handPos, target->GetPos(), enemyBullet);
}

void GunEnemy::handle_collision(int otherLayer, int damage)
{
	switch (otherLayer)
	{
	case wall:
	case water:
		if (!isWallCollision(Vector2D<float>(pos.x + velocity * frame_time / 2 * (pos.x > lastPos.x ? 1 : -1), lastPos.y), col->size))
		{
			pos.x += velocity * frame_time / 2 * (pos.x > lastPos.x ? 1 : -1);
			pos.y = lastPos.y;
		}
		else if (!isWallCollision(Vector2D<float>(lastPos.x, pos.y + velocity * frame_time / 2 * (pos.x > lastPos.x ? 1 : -1)), col->size))
		{
			pos.x = lastPos.x;
			pos.y += velocity * frame_time / 2 * (pos.y > lastPos.y ? 1 : -1);
		}
		else
			pos = lastPos;
		lastPos = pos;
		col->pos = pos;
		break;
	case rolled_player:
		pos -= (target->GetPos() - pos).Normalize() * 5;
		break;
	case player:
	case damaged_player:
		if (!isWallCollision(Vector2D<float>(lastPos.x, lastPos.y), col->size))
			pos = lastPos;
		break;
	case playerMelee:
	case playerBullet:
		if (col == nullptr) return;
		if (otherLayer == playerMelee)
			EffectManager::getInstance()->set_effect(new Effect(CEffect::SWORDATTACK, col->pos));
		moveTime = 2;
		lastPos = pos;
		pos -= (target->GetPos() - pos).Normalize() * (otherLayer == playerMelee ? 10 : 2);	// 넉백
		if (!isWallCollision(Vector2D<float>(pos.x, lastPos.y), col->size))
			pos.y = lastPos.y;
		else if (!isWallCollision(Vector2D<float>(lastPos.x, pos.y), col->size))
			pos.x = lastPos.x;
		else
			pos = lastPos;
		lastPos = pos;
		col->pos = pos;
		HP -= damage;
		if (HP <= 0) {
			state = STATE_DEAD;
			releaseCollider();
			SoundManager::getInstance()->play(ENEMYDEATH);
		}
		else {
			state = STATE_DAMAGED;
			SoundManager::getInstance()->play(ENEMYHURT);
		}
		frame = 0;
		break;
	default:
		break;
	}
}

// ---- Bat: 플레이어에게 날아와 자폭 ----

class Bat : public Enemy
{
private:
	enum { IDLE, DEAD };
	static Animation animation[2];
	void attack();
public:
	Bat(float x, float y);
	static void init();
	static void release();
	virtual void draw_character(HDC mDC);
	virtual void handle_event();
	virtual void update();
	virtual void handle_collision(int otherLayer, int damage);
};

Animation Bat::animation[2];

void Bat::init()
{
	LoadAnimation(animation[IDLE], L"resources/bat.png", 6, 2);
	LoadAnimation(animation[DEAD], L"resources/bat_damaged.png", 4, 2);
}

void Bat::release()
{
	for (int i = 0; i < 2; i++) {
		animation[i].resource.Destroy();
	}
}

Bat::Bat(float x, float y) : Enemy(x, y)
{
	shadow.Load(L"resources/shadow.png");
	velocity = 200;
	HP = 10;
	state = IDLE;
	attackRange = 50;
	lastPos = pos;
}

void Bat::draw_character(HDC mDC)
{
	if (col || state == DEAD) {
		float yDest = pos.y - animation[IDLE].size.bottom;
		shadow.Draw(mDC, pos.x - shadow.GetWidth(), pos.y + 60 - 2 - shadow.GetHeight(), shadow.GetWidth() * 2, shadow.GetHeight() * 2);
		animation[state].resource.Draw(mDC, pos.x - animation[state].size.right, yDest - 20, animation[state].size.right * 2, animation[state].size.bottom * 2,
			(int)frame * animation[state].size.right, 0, animation[state].size.right, animation[state].size.bottom);
	}
	else
		DrawSpawn(mDC, pos, frame);
}

void Bat::handle_event()
{
	if (col) {
		if (target != nullptr && (target->GetPos() - pos).GetLenth() <= 600)
			targetLocked = true;
		if (targetLocked)
			dir = (target->GetPos() - pos).Normalize();
		else
			dir.x = dir.y = 0;
		if (attackable())
			attack();
	}
}

void Bat::update()
{
	if (col || state == DEAD) {
		lastPos = pos;
		frame = (frame + frame_time * animation[state].velocity * animation[state].frame);
		if (state == IDLE) {
			pos = pos + dir * velocity * frame_time;
			if (frame >= animation[state].frame) frame = 0;
			col->pos = pos;
		}
		else {
			if ((int)frame == animation[state].frame) {
				EnemyManager::getInstance()->delete_enemy(this);
				deleteSet.insert(this);
				EffectManager::getInstance()->set_effect(new Effect(CEffect::BATDIE, pos));
				// 폭발 범위(100) 안의 플레이어에게 피해
				for (auto& other : COLL) {
					for (int i = 0; i < 4; i++) {
						if (other->layer == player) {
							Vector2D<float> dot[4] = { Vector2D<float>(other->pos.x - other->size.x, other->pos.y - other->size.y),
							Vector2D<float>(other->pos.x + other->size.x, other->pos.y - other->size.y),
							Vector2D<float>(other->pos.x + other->size.x, other->pos.y + other->size.y),
							Vector2D<float>(other->pos.x - other->size.x, other->pos.y + other->size.y) };
							if ((dot[i] - pos).GetLenth() <= 100) {
								other->owner->handle_collision(enemyBullet, 1);
								break;
							}
						}
					}
				}
			}
		}
	}
	else {
		frame = (frame + frame_time * spawnAnim.velocity * spawnAnim.frame);
		if ((int)frame == spawnAnim.frame) {
			col = new Collider(10);
			col->owner = this;
			col->layer = enemy;
			col->pos = pos;
			col->damage = 5;
			COLL.emplace_back(col);
			frame = 0;
		}
	}
}

void Bat::attack()
{
	state = DEAD;
	releaseCollider();
	frame = 0;
}

void Bat::handle_collision(int otherLayer, int damage)
{
	switch (otherLayer)
	{
	case wall:
		if (!isWallCollision(Vector2D<float>(pos.x + velocity * frame_time / 2 * (pos.x > lastPos.x ? 1 : -1), lastPos.y), col->size))
		{
			pos.x += velocity * frame_time / 2 * (pos.x > lastPos.x ? 1 : -1);
			pos.y = lastPos.y;
		}
		else if (!isWallCollision(Vector2D<float>(lastPos.x, pos.y + velocity * frame_time / 2 * (pos.x > lastPos.x ? 1 : -1)), col->size))
		{
			pos.x = lastPos.x;
			pos.y += velocity * frame_time / 2 * (pos.y > lastPos.y ? 1 : -1);
		}
		else
			pos = lastPos;
		lastPos = pos;
		col->pos = pos;
		break;
	case player:
	case damaged_player:
		if (!isWallCollision(Vector2D<float>(lastPos.x, lastPos.y), col->size))
			pos = lastPos;
		break;
	case playerMelee:
	case playerBullet:
		if (col == nullptr) return;
		if (otherLayer == playerMelee)
			EffectManager::getInstance()->set_effect(new Effect(CEffect::SWORDATTACK, col->pos));
		HP -= damage;
		if (HP <= 0) {
			state = DEAD;
			releaseCollider();
		}
		else {
			lastPos = pos;
			pos -= (target->GetPos() - pos).Normalize() * 10;
			if (!isWallCollision(Vector2D<float>(pos.x, lastPos.y), col->size))
				pos.y = lastPos.y;
			else if (!isWallCollision(Vector2D<float>(lastPos.x, pos.y), col->size))
				pos.x = lastPos.x;
			else
				pos = lastPos;
			lastPos = pos;
			col->pos = pos;
		}
		frame = 0;
		break;
	default:
		break;
	}
}

// ---- Bombshe: 가까워지면 퍼지는 음파 공격 ----

class Bombshe : public Enemy
{
private:
	enum { BIDLE, BATTACK, BDAMAGED, BDEAD };
	static Animation animation[4][3];	// [상태][FRONT, FRONT_RIGHT, FRONT_LEFT]
	float attackSize;
	float moveTime;
	Effect* wave;
	bool isAttack;
	void SetDirection();
	void attack();
public:
	Bombshe(float x, float y);
	static void init();
	static void release();
	virtual void draw_character(HDC mDC);
	virtual void handle_event();
	virtual void update();
	virtual void handle_collision(int otherLayer, int damage);
};

Animation Bombshe::animation[4][3];

void Bombshe::init()
{
	LoadAnimation(animation[BIDLE][FRONT], L"resources/Bombshe_front.png", 4, 2);
	LoadAnimation(animation[BIDLE][FRONT_RIGHT], L"resources/Bombshe_right.png", 4, 2);
	LoadAnimation(animation[BIDLE][FRONT_LEFT], L"resources/Bombshe_left.png", 4, 2);
	for (int i = 0; i < 3; i++) {
		LoadAnimation(animation[BATTACK][i], L"resources/Bombshe_attack.png", 5, 2);
		LoadAnimation(animation[BDAMAGED][i], L"resources/Bombshe_damaged.png", 1, 8);
		LoadAnimation(animation[BDEAD][i], L"resources/Bombshe_dead.png", 2, 3);
	}
}

void Bombshe::release()
{
	for (int i = 0; i < 4; i++) {
		for (int j = 0; j < 3; j++)
			animation[i][j].resource.Destroy();
	}
}

Bombshe::Bombshe(float x, float y) : Enemy(x, y), isAttack(false)
{
	shadow.Load(L"resources/shadow.png");
	attackSize = 0;
	velocity = 100;
	moveTime = 200;
	HP = 70;
	state = BIDLE;
	attackRange = 200;
	attackCoolTime = 200;
	wave = nullptr;
	lastPos = pos;
}

void Bombshe::draw_character(HDC mDC)
{
	if (col || HP <= 0) {
		float yDest = pos.y - (animation[BIDLE][FRONT].size.bottom - 22) * 2;
		shadow.Draw(mDC, pos.x - shadow.GetWidth(), pos.y + animation[state][direction].size.bottom - 2 - shadow.GetHeight(), shadow.GetWidth() * 2, shadow.GetHeight() * 2);
		animation[state][direction].resource.Draw(mDC, pos.x - animation[state][direction].size.right, yDest - 20, animation[state][direction].size.right * 2, animation[state][direction].size.bottom * 2,
			(int)frame * animation[state][direction].size.right, 0, animation[state][direction].size.right, animation[state][direction].size.bottom);
		if (isAttack && lookRange) {
			HPEN old = (HPEN)SelectObject(mDC, GREENP);
			Arc(mDC, pos.x - attackSize, pos.y - attackSize, pos.x + attackSize, pos.y + attackSize, 0, 0, 0, 0);
			SelectObject(mDC, old);
		}
	}
	else
		DrawSpawn(mDC, pos, frame);
}

void Bombshe::handle_event()
{
	if (col) {
		if (attackable() && !targetLocked || (state == BDAMAGED && (int)frame == 1) && !targetLocked) {
			targetLocked = true;
			state = BATTACK;
			frame = 0;
		}
		if (targetLocked) {
			if (!isAttack && (int)frame == 4) {
				wave = new Effect(CEffect::BOMBSHE1, col->pos);
				EffectManager::getInstance()->set_effect(wave);
				isAttack = true;
			}
			if (isAttack) {
				attack();
				if ((int)(wave->time + 1) == wave->effect[wave->type].frame) {
					if (wave->effect[wave->type].frame == 13) {
						wave->type = CEffect::BOMBSHE2;
						wave->time = 0;
					}
					else if (wave->effect[wave->type].frame == 3)
						wave->time = 0;
				}
				if (state == BDAMAGED && (int)frame == 1) {
					state = BATTACK;
					frame = 0;
				}
			}
		}
	}
}

void Bombshe::update()
{
	if (col || HP <= 0) {
		frame = (frame + frame_time * animation[state][direction].velocity * animation[state][direction].frame);
		if (state == BIDLE) {
			if (frame >= animation[state][direction].frame) frame = 0;
			SetDirection();
			col->pos = pos;
		}
		else if (state == BATTACK && (int)frame < 5) {
			if (attackSize < 100)
				attackSize += 1;
			SetDirection();
			col->pos = pos;
		}
		else if (state == BDEAD) {
			if ((int)frame == animation[state][direction].frame) {
				if (randrop(dre) == 0)
					drops.emplace_back(new DropItem(DROP::DROCKET, pos));
				EnemyManager::getInstance()->delete_enemy(this);
				deleteSet.insert(this);
				EffectManager::getInstance()->set_effect(new Effect(CEffect::BOMBSHEDIE, pos));
			}
		}
		if ((int)frame == animation[state][direction].frame && state != BDAMAGED) frame = 0;
	}
	else {
		frame = (frame + frame_time * spawnAnim.velocity * spawnAnim.frame);
		if ((int)frame == spawnAnim.frame) {
			col = new Collider(Vector2D<float>(animation[BIDLE][FRONT].size.right, animation[BIDLE][FRONT].size.bottom));
			col->owner = this;
			col->layer = enemy;
			col->pos = pos;
			col->damage = 5;
			COLL.emplace_back(col);
			frame = 0;
		}
	}
}

void Bombshe::SetDirection()
{
	angle = std::atan2(target->GetPos().y - pos.y, target->GetPos().x - pos.x) * (180.0f / M_PI);
	if (angle >= -20 && angle <= 60) {
		direction = FRONT_RIGHT;
	}
	else if (angle > 60 && angle <= 120) {
		direction = FRONT;
	}
	else if (angle >= 120 && angle <= 180 || angle > -180 && angle < -160) {
		direction = FRONT_LEFT;
	}
	else if (angle >= -160 && angle <= -120) {
		direction = FRONT_LEFT;
	}
	else if (angle >= -120 && angle <= -60) {
		direction = FRONT;
	}
	else if (angle >= -60 && angle < -20) {
		direction = FRONT_RIGHT;
	}
}

void Bombshe::attack()
{
	for (auto& other : COLL) {
		for (int i = 0; i < 4; i++) {
			if (other->layer == player) {
				Vector2D<float> dot[4] = { Vector2D<float>(other->pos.x - other->size.x, other->pos.y - other->size.y),
				Vector2D<float>(other->pos.x + other->size.x, other->pos.y - other->size.y),
				Vector2D<float>(other->pos.x + other->size.x, other->pos.y + other->size.y),
				Vector2D<float>(other->pos.x - other->size.x, other->pos.y + other->size.y) };
				if ((dot[i] - pos).GetLenth() <= attackSize) {
					other->owner->handle_collision(enemyBullet, 1);
					break;
				}
			}
		}
	}
}

void Bombshe::handle_collision(int otherLayer, int damage)
{
	switch (otherLayer)
	{
	case wall:
	case water:
		if (!isWallCollision(Vector2D<float>(pos.x + velocity * frame_time / 2 * (pos.x > lastPos.x ? 1 : -1), lastPos.y), col->size))
		{
			pos.x += velocity * frame_time / 2 * (pos.x > lastPos.x ? 1 : -1);
			pos.y = lastPos.y;
		}
		else if (!isWallCollision(Vector2D<float>(lastPos.x, pos.y + velocity * frame_time / 2 * (pos.x > lastPos.x ? 1 : -1)), col->size))
		{
			pos.x = lastPos.x;
			pos.y += velocity * frame_time / 2 * (pos.y > lastPos.y ? 1 : -1);
		}
		else
			pos = lastPos;
		lastPos = pos;
		col->pos = pos;
		break;
	case rolled_player:
		pos -= (target->GetPos() - pos).Normalize() * 5;
		break;
	case player:
	case damaged_player:
		if (!isWallCollision(Vector2D<float>(lastPos.x, lastPos.y), col->size))
			pos = lastPos;
		break;
	case playerMelee:
	case playerBullet:
		if (col == nullptr) return;
		if (otherLayer == playerMelee)
			EffectManager::getInstance()->set_effect(new Effect(CEffect::SWORDATTACK, col->pos));
		HP -= damage;
		if (HP <= 0) {
			state = BDEAD;
			releaseCollider();
			if (wave)
				wave->time = wave->effect[wave->type].frame;
			SoundManager::getInstance()->play(ENEMYDEATH);
		}
		else {
			state = BDAMAGED;
			SoundManager::getInstance()->play(ENEMYHURT);
		}
		frame = 0;
		break;
	default:
		break;
	}
}

// ---- EnemyManager ----

EnemyManager* EnemyManager::instance = nullptr;

EnemyManager::EnemyManager()
{
	GunEnemy::init();
	Bat::init();
	Bombshe::init();
	spawnAnim.resource.Load(L"resources/Enemy_Spawn.png");
	spawnAnim.frame = 17;
	spawnAnim.size = RECT(0, 0, 40, 40);
	spawnAnim.velocity = 0.5;
}

EnemyManager::~EnemyManager()
{
	enemy.clear();
	GunEnemy::release();
	Bat::release();
	Bombshe::release();
	spawnAnim.resource.Destroy();
}

EnemyManager* EnemyManager::getInstance()
{
	if (instance == nullptr)
		instance = new EnemyManager();
	return instance;
}

void EnemyManager::Clear()
{
	enemy.clear();
}

// 웨이브 번호 = (stage - 1) * 3 + phase
void EnemyManager::SetEnemy(int stage)
{
	switch (stage)
	{
	case 1:
		spawn(PISTOLMAN, 300, 400);
		spawn(PISTOLMAN, 1000, 400);
		spawn(PISTOLMAN, 1800, 400);
		break;
	case 2:
		spawn(PISTOLMAN, 300, 400);
		spawn(PISTOLMAN, 1000, 400);
		spawn(PISTOLMAN, 1800, 400);
		spawn(PISTOLMAN, 300, 900);
		spawn(PISTOLMAN, 1000, 900);
		spawn(PISTOLMAN, 1800, 900);
		break;
	case 3:
		spawn(RIFLEMAN, 300, 400);
		spawn(PISTOLMAN, 1000, 400);
		spawn(PISTOLMAN, 1800, 400);
		spawn(PISTOLMAN, 300, 900);
		spawn(PISTOLMAN, 1000, 900);
		spawn(RIFLEMAN, 1800, 900);
		break;
	case 4:
		spawn(PISTOLMAN, 600, 200);
		spawn(PISTOLMAN, 1500, 200);
		spawn(RIFLEMAN, 300, 600);
		spawn(RIFLEMAN, 1800, 600);
		spawn(SHOTGUNMAN, 760, 980);
		break;
	case 5:
		spawn(BAT, 300, 200);
		spawn(BAT, 1600, 200);
		spawn(BAT, 600, 600);
		spawn(BAT, 1500, 600);
		break;
	case 6:
		spawn(SHOTGUNMAN, 600, 200);
		spawn(SHOTGUNMAN, 1300, 200);
		spawn(BAT, 300, 600);
		spawn(BAT, 1800, 600);
		spawn(SHOTGUNMAN, 100, 970);
		spawn(SHOTGUNMAN, 1800, 970);
		spawn(BAT, 300, 1000);
		spawn(BAT, 1800, 1000);
		break;
	case 7:
		spawn(BOMBSHE, 400, 600);
		spawn(BOMBSHE, 1500, 600);
		spawn(PISTOLMAN, 800, 300);
		spawn(PISTOLMAN, 1000, 300);
		spawn(RIFLEMAN, 800, 800);
		spawn(RIFLEMAN, 1000, 800);
		break;
	case 8:
		spawn(SHOTGUNMAN, 800, 300);
		spawn(SHOTGUNMAN, 1000, 300);
		spawn(SHOTGUNMAN, 800, 800);
		spawn(SHOTGUNMAN, 1000, 800);
		spawn(SHOTGUNMAN, 300, 200);
		spawn(SHOTGUNMAN, 1600, 200);
		spawn(SHOTGUNMAN, 300, 600);
		spawn(SHOTGUNMAN, 1800, 600);
		break;
	case 9:
		spawn(PISTOLMAN, 800, 300);
		spawn(PISTOLMAN, 1000, 300);
		spawn(RIFLEMAN, 800, 800);
		spawn(RIFLEMAN, 1000, 800);
		spawn(SHOTGUNMAN, 300, 200);
		spawn(SHOTGUNMAN, 1600, 200);
		spawn(BAT, 300, 600);
		spawn(BAT, 1800, 600);
		break;
	default:
		break;
	}
}

void EnemyManager::spawn(int type, float x, float y)
{
	switch (type)
	{
	case PISTOLMAN:
	case RIFLEMAN:
	case SHOTGUNMAN:
		enemy.emplace_back(new GunEnemy(type, x, y));
		break;
	case BAT:
		enemy.emplace_back(new Bat(x, y));
		break;
	case BOMBSHE:
		enemy.emplace_back(new Bombshe(x, y));
		break;
	default:
		break;
	}
}

void EnemyManager::delete_enemy(Enemy* e)
{
	std::erase(enemy, e);
}

bool EnemyManager::isclear()
{
	return enemy.empty();
}

void EnemyManager::handle_event()
{
	for (int i = 0; i < enemy.size(); i++) {
		if (!(enemy[i]->isDead()))
			enemy[i]->handle_event();
	}
}

void EnemyManager::update()
{
	// update 중에 죽은 적이 delete_enemy로 빠지므로 복사본으로 순회
	for (auto E : std::vector<Enemy*>(enemy))
		E->update();
	std::sort(enemy.begin(), enemy.end(), [](const Enemy* e1, const Enemy* e2) {return e1->GetPos().y < e2->GetPos().y; });
}

void EnemyManager::draw(HDC mDC)
{
	for (auto& E : enemy)
		E->draw_character(mDC);
}
