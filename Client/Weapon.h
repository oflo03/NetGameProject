#pragma once
#include "Collision.h"

// ---- 무기 ----

enum ID
{
	SWORD		// long sword
	, PISTOL	// semi-automatic pistol
	, RIFLE		// assault rifle
	, SHOTGUN	// double barrel shotgun
	, ROCKET	// rocket launcher
};

const int cooltime[]{ 30,15,7,30,10 };
const int rebound[]{ 8,10,5,20,100 };

class Weapon
{
protected:
	CImage resource[2];
	CImage reverseResource[2];
	int curAmmo;
	int maxAmmo;
	int angle;
	int coolTime;
	int curTime;
	int shotTime;
	void LoadImages(const wchar_t* name, bool hasShotImage);
public:
	Weapon() :maxAmmo(0), curAmmo(0), angle(0), coolTime(0), curTime(0), shotTime(0) {}
	virtual ~Weapon() {
		for (int i = 0; i < 2; i++) {
			resource[i].Destroy();
			reverseResource[i].Destroy();
		}
	}
	virtual void update();
	virtual void attack(const Vector2D<float>& center, const Vector2D<float>& mPos, int side) = 0;
	void ReLoad() {
		if (maxAmmo == 2) {
			if (curAmmo < 2)
				curAmmo++;
		}
		else
			curAmmo = maxAmmo;
	};
	void Enemy() { maxAmmo /= 3, coolTime *= 2, ReLoad(); };	// 적이 들 때 성능 하향
	void SetCurTime(int t) { curTime = t; }
	void SetCurAmmo(int t) { curAmmo = t; }
	int GetCurTime() { return curTime; }
	int GetShotTime() { return shotTime; }
	int GetCurAmmo() { return curAmmo; }
	int GetMaxAmmo() { return maxAmmo; }
	bool IsReBound() { return (coolTime - curTime < 3); }
	bool IsFull() { return maxAmmo == curAmmo; }
	bool IsRunOut() { return (curAmmo == 0); }
	virtual void draw_weapon(HDC mDC, const Vector2D<float>& center, const Vector2D<float>& mPos);
};

class Sword : public Weapon
{
private:
	Animation slash;
	float frame;
	float attackRange;
	Vector2D<float> centerPos;
	bool slashed;
public:
	Sword();
	~Sword() { slash.resource.Destroy(); }
	virtual void update();
	virtual void attack(const Vector2D<float>& center, const Vector2D<float>& mPos, int side);
	virtual void draw_weapon(HDC mDC, const Vector2D<float>& center, const Vector2D<float>& mPos);
};

class Pistol : public Weapon
{
public:
	Pistol();
	virtual void attack(const Vector2D<float>& center, const Vector2D<float>& mPos, int side);
};

class Rifle : public Weapon
{
public:
	Rifle();
	virtual void attack(const Vector2D<float>& center, const Vector2D<float>& mPos, int side);
};

class Shotgun : public Weapon
{
public:
	Shotgun();
	virtual void attack(const Vector2D<float>& center, const Vector2D<float>& mPos, int side);
};

class Rocket : public Weapon
{
public:
	Rocket();
	virtual void attack(const Vector2D<float>& center, const Vector2D<float>& mPos, int side);
};

// ---- 총알 ----

const int BulletDamage[]{ 10,7,8,5,5,80,10,10,10,100,10,10,10 };

enum bullet_table {
	BPISTOL,
	BRIFLE,
	BSHOTGUN,
	BENEMY,
	BBOUNCE,
	BOSSBULLET1,
	BOSSBULLET2,
	BOSSBULLET3,
	BOUNCEDBOSSBULLET2,
	BROCKET
};

class Bullet : public Master
{
protected:
	static Animation animation[10];
	int type;
	int side;
	float frame;
	Vector2D<float> pos;
	Vector2D<float> dir;
	double velocity;
	double angle;
	void remove();
public:
	Bullet(int type, int side, Vector2D<float> pos, Vector2D<float> dir);
	static void init();
	static void destroy();
	void draw_bullet(HDC mDC);
	void update();
	virtual void handle_collision(int otherLayer, int damage);
};

// ---- 드랍 아이템 ----

enum DROP
{
	DPISTOL,
	DRIFLE,
	DSHOTGUN,
	DROCKET
};

class DropItem : public Master
{
private:
	static CImage image[4];
	int type;
	Vector2D<float> pos;
	float xDir;
public:
	DropItem(int type, Vector2D<float> pos);
	static void init();
	static void destroy();
	static void Clear();
	void update();
	void Draw(HDC mDC);
	virtual void handle_collision(int otherLayer, int damage);
};
