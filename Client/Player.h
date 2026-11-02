#pragma once
#include "Weapon.h"

enum playertype
{
	marin, knight
};
extern int selectedPlayer;
extern Vector2D<float> spawnpoint[4];

class PlayerState;

class Player : public Master
{
private:
	static Player* instance;
	Player(int type, float x, float y);
	~Player();
	void hit();	// 피격 처리 (무적/구르기 중이면 무시)

	int type;	// playertype
	Animation animation[5][6];	// [STATE][DIRECTION]
	CImage hand;
	CImage shadow[2];
	float frame;
	float damageCnt;
	float angle;
	Vector2D<float> pos;
	Vector2D<float> lastPos;
	Vector2D<float> handPos;
	Vector2D<float> mPos;
	Vector2D<float> dir;
	int direction;
	PlayerState* state;
	int curstate;
	int hp;
	int velocity;
	int selectedWeapon;
	std::vector<Weapon*> myWeapons;
public:
	static Player* getInstance();
	static void release();
	void draw_character(HDC mDC);
	void handle_event();
	void update();
	void SetDirection();
	void attack() { myWeapons[selectedWeapon]->attack(handPos, mPos, playerBullet); }
	virtual void handle_collision(int otherLayer, int damage);

	void SetPos(Vector2D<float> temp) { pos = temp; }
	Vector2D<float> GetPos() { return pos; }
	float GetAngle() { return angle; }
	void SetDir(Vector2D<float> temp) { dir = temp; }
	Vector2D<float> GetDir() { return dir; }
	Vector2D<float> GetMouseVector() { return (mPos - handPos).Normalize(); }
	double GetVelocity() { return velocity; };
	void SetDirection(int direction) { this->direction = direction; }
	void SetCurState(int state) { curstate = state; }
	int GetHP() { return hp; }

	void WeaponReload(int type) { myWeapons[type]->ReLoad(); }
	int GetWeaponCurAmmo() { return myWeapons[selectedWeapon]->GetCurAmmo(); }
	int GetWeaponMaxAmmo() { return myWeapons[selectedWeapon]->GetMaxAmmo(); }
	bool GetIfWeaponFull(int type) { return myWeapons[type]->IsFull(); }
	int GetWeapon() { return selectedWeapon; }
	void SetWeapon(int type) { if (!myWeapons[type]->IsRunOut()) selectedWeapon = type, myWeapons[selectedWeapon]->SetCurTime(cooltime[type] - 3); }
	void SetWeaponUp() { do selectedWeapon += 4, selectedWeapon %= 5; while (myWeapons[selectedWeapon]->IsRunOut()); }
	void SetWeaponDown() { do selectedWeapon += 1, selectedWeapon %= 5; while (myWeapons[selectedWeapon]->IsRunOut()); }
	void SetCurAmmoZero(int type) { myWeapons[type]->SetCurAmmo(0); }
	std::vector<Weapon*>& getWeapons() { return myWeapons; }
};
