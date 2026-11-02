#include "Weapon.h"
#include <string>
#include "Player.h"
#include "Boss.h"
#include "Effect.h"
#include "Sound.h"

// ---- Weapon ----

void Weapon::LoadImages(const wchar_t* name, bool hasShotImage)
{
	std::wstring base = std::wstring(L"resources/Item_Weapon_") + name;
	std::wstring shot = hasShotImage ? L"2" : L"";
	resource[0].Load((base + L".png").c_str());
	reverseResource[0].Load((base + L"_Reverse.png").c_str());
	resource[1].Load((base + shot + L".png").c_str());
	reverseResource[1].Load((base + L"_Reverse" + shot + L".png").c_str());
}

void Weapon::update()
{
	if (curTime)
		--curTime;
	if (shotTime)
		shotTime--;
}

void Weapon::draw_weapon(HDC mDC, const Vector2D<float>& center, const Vector2D<float>& mPos)
{
	angle = std::atan2(mPos.y - (center.y), mPos.x - center.x) * (180.0f / M_PI) * -1;
	float temp = angle;
	bool isShot = shotTime != 0;
	if (angle >= 90)
		angle = angle - 180;
	else if (angle <= -90)
		angle = angle + 180;
	int width = resource[isShot].GetWidth();
	int height = resource[isShot].GetHeight();
	CImage rotatedImage;
	rotatedImage.Create(width, height, 32);
	for (int y = 0; y < height; y++) {
		for (int x = 0; x < width; x++) {
			Vector2D<float> rotatedPos = (Vector2D<float>(x - width / 2, y - height / 2)).Rotate(angle) + Vector2D<float>(width / 2, height / 2);
			if (rotatedPos.x >= 0 && rotatedPos.x < width && rotatedPos.y >= 0 && rotatedPos.y < height) {
				BYTE* srcPixel;
				if (angle == temp)
					srcPixel = (BYTE*)resource[isShot].GetPixelAddress(rotatedPos.x, rotatedPos.y);
				else
					srcPixel = (BYTE*)reverseResource[isShot].GetPixelAddress(rotatedPos.x, rotatedPos.y);
				BYTE* destPixel = (BYTE*)rotatedImage.GetPixelAddress(x, y);
				memcpy(destPixel, srcPixel, sizeof(BYTE) * 4);
			}
		}
	}
	rotatedImage.TransparentBlt(mDC, center.x - width, center.y - height, width * 2, height * 2, RGB(0, 0, 0));
	rotatedImage.Destroy();
}

// ---- Sword ----

Sword::Sword() : Weapon(), slashed(false)
{
	coolTime = cooltime[SWORD];
	curAmmo = maxAmmo = 1;
	frame = 0;
	attackRange = 110;
	LoadImages(L"Sword", false);
	LoadAnimation(slash, L"resources/sword_attack.png", 8, 4);
}

void Sword::update()
{
	if (curTime)
		--curTime;
	if (frame > 0) {
		frame = (frame + frame_time * 4 * slash.frame);
		if (frame >= slash.frame) {
			frame = 0;
			slashed = false;
		}
	}
	if ((int)frame >= 4 && (int)frame <= 6 && !slashed)
	{
		Vector2D<float> mPos(cos(-angle * M_PI / 180) * attackRange, sin(-angle * M_PI / 180) * attackRange);
		mPos += centerPos;
		for (auto& other : COLL) {
			if (other->layer != enemy && other->layer != enemyBullet)continue;
			Vector2D<float> dot[4] = {
				Vector2D<float>(other->pos.x - other->size.x, other->pos.y - other->size.y),
				Vector2D<float>(other->pos.x + other->size.x, other->pos.y - other->size.y),
				Vector2D<float>(other->pos.x + other->size.x, other->pos.y + other->size.y),
				Vector2D<float>(other->pos.x - other->size.x, other->pos.y + other->size.y)
			};
			for (int i = 0; i < 4; i++) {
				if ((dot[i] - centerPos).GetLenth() <= attackRange) {
					if ((mPos - centerPos).GetRadian(dot[i] - centerPos) <= 60) {
						collisionMsg.emplace(CollisionMessage(other->owner, playerMelee, 10));
						break;
					}
				}
			}
		}
		slashed = true;
	}
}

void Sword::attack(const Vector2D<float>& center, const Vector2D<float>& mPos, int side)
{
	if (!curTime) {
		SoundManager::getInstance()->play(SWORD_ATK);
		curTime = coolTime;
		frame = 0.1;
	}
}

void Sword::draw_weapon(HDC mDC, const Vector2D<float>& center, const Vector2D<float>& mPos)
{
	centerPos = center;
	if (frame == 0.0f) {
		angle = std::atan2(mPos.y - (center.y), mPos.x - center.x) * (180.0f / M_PI) * -1;
		int width = resource[0].GetWidth();
		int height = resource[0].GetHeight();
		CImage rotatedImage;
		rotatedImage.Create(width, height, 32);
		for (int y = 0; y < height; y++) {
			for (int x = 0; x < width; x++) {
				Vector2D<float> rotatedPos = (Vector2D<float>(x - width / 2, y - height / 2)).Rotate(angle) + Vector2D<float>(width / 2, height / 2);
				if (rotatedPos.x >= 0 && rotatedPos.x < width && rotatedPos.y >= 0 && rotatedPos.y < height) {
					BYTE* srcPixel = (BYTE*)resource[0].GetPixelAddress(rotatedPos.x, rotatedPos.y);
					BYTE* destPixel = (BYTE*)rotatedImage.GetPixelAddress(x, y);
					memcpy(destPixel, srcPixel, sizeof(BYTE) * 4);
				}
			}
		}
		rotatedImage.TransparentBlt(mDC, center.x - width, center.y - height, width * 2, height * 2, RGB(0, 0, 0));
		rotatedImage.Destroy();
	}
	else {
		int width = slash.size.right;
		int height = slash.size.bottom;
		CImage temp;
		CImage rotatedImage;
		rotatedImage.Create(width, height, 32);
		temp.Create(width, height, 32);
		for (int y = 0; y < height; y++) {
			for (int x = 0; x < width; x++) {
				BYTE* srcPixel = (BYTE*)slash.resource.GetPixelAddress((int)frame * width + x, y);
				BYTE* destPixel = (BYTE*)temp.GetPixelAddress(x, y);
				memcpy(destPixel, srcPixel, sizeof(BYTE) * 4);
			}
		}
		for (int y = 0; y < height; y++) {
			for (int x = 0; x < width; x++) {
				Vector2D<float> rotatedPos = (Vector2D<float>(x - width / 2, y - height / 2)).Rotate(angle) + Vector2D<float>(width / 2, height / 2);
				if (rotatedPos.x >= 0 && rotatedPos.x < width && rotatedPos.y >= 0 && rotatedPos.y < height) {
					BYTE* srcPixel = (BYTE*)temp.GetPixelAddress(rotatedPos.x, rotatedPos.y);
					BYTE* destPixel = (BYTE*)rotatedImage.GetPixelAddress(x, y);
					memcpy(destPixel, srcPixel, sizeof(BYTE) * 4);
				}
			}
		}
		rotatedImage.TransparentBlt(mDC, center.x - slash.size.right, center.y - slash.size.bottom, slash.size.right * 2, slash.size.bottom * 2,
			0, 0, slash.size.right, slash.size.bottom, RGB(0, 0, 0)
		);
		if (lookRange)
		{
			Vector2D<float> lineP1 = Vector2D<float>(cos(-angle * M_PI / 180) * attackRange, sin(-angle * M_PI / 180) * attackRange);
			Vector2D<float> lineP2 = Vector2D<float>(cos(-angle * M_PI / 180) * attackRange, sin(-angle * M_PI / 180) * attackRange);
			lineP1 = lineP1.Normalize() * attackRange;
			lineP2 = lineP2.Normalize() * attackRange;
			lineP1.Rotate(-60);
			lineP2.Rotate(60);

			HPEN old = (HPEN)SelectObject(mDC, GREENP);
			MoveToEx(mDC, center.x + lineP1.x, center.y + lineP1.y, NULL);
			LineTo(mDC, center.x, center.y);
			LineTo(mDC, center.x + lineP2.x, center.y + lineP2.y);
			Arc(mDC, center.x - attackRange, center.y - attackRange, center.x + attackRange, center.y + attackRange,
				center.x + lineP2.x, center.y + lineP2.y, center.x + lineP1.x, center.y + lineP1.y);
			SelectObject(mDC, old);
		}
		rotatedImage.Destroy();
		temp.Destroy();
	}
}

// ---- 총기 ----

Pistol::Pistol() : Weapon()
{
	coolTime = cooltime[PISTOL];
	curAmmo = maxAmmo = 20;
	LoadImages(L"Pistol", true);
}

void Pistol::attack(const Vector2D<float>& hand, const Vector2D<float>& mPos, int side)
{
	if (!curTime && curAmmo > 0)
	{
		Vector2D<float> t = Vector2D<float>(mPos.x - hand.x, mPos.y - hand.y);
		t.Normalize();
		t.Rotate(uid(dre) % 11 - 5);
		Bullets.emplace_back(new Bullet(BPISTOL, side, hand + Vector2D<float>(t.x * 30, t.y * 30 - 10), t * 10));
		curTime = coolTime;
		shotTime = 10;
		curAmmo--;
	}
}

Rifle::Rifle() : Weapon()
{
	coolTime = cooltime[RIFLE];
	curAmmo = maxAmmo = 35;
	LoadImages(L"Rifle", true);
}

void Rifle::attack(const Vector2D<float>& hand, const Vector2D<float>& mPos, int side)
{
	if (!curTime && curAmmo > 0)
	{
		Vector2D<float> t = Vector2D<float>(mPos.x - hand.x, mPos.y - hand.y);
		t /= t.GetLenth();
		t.Rotate(uid(dre) % 15 - 7);
		Bullets.emplace_back(new Bullet(BRIFLE, side, hand + Vector2D<float>(t.x * 30, t.y * 30 - 10), t * 10));
		curTime = coolTime;
		shotTime = 5;
		curAmmo--;
	}
}

Shotgun::Shotgun() : Weapon()
{
	coolTime = cooltime[SHOTGUN];
	curAmmo = maxAmmo = 10;
	LoadImages(L"Shotgun", true);
}

void Shotgun::attack(const Vector2D<float>& hand, const Vector2D<float>& mPos, int side)
{
	if (!curTime && curAmmo > 0)
	{
		Vector2D<float> t = Vector2D<float>(mPos.x - hand.x, mPos.y - hand.y);
		t /= t.GetLenth();
		t.Rotate(-10);
		for (int i = 0; i < 5; ++i)
		{
			Bullets.emplace_back(new Bullet(BSHOTGUN, side, hand + Vector2D<float>(t.x * 30, t.y * 30 - 10), t * 10));
			t.Rotate(5);
		}
		curTime = coolTime;
		shotTime = 7;
		curAmmo--;
	}
}

Rocket::Rocket() : Weapon()
{
	coolTime = cooltime[ROCKET];
	maxAmmo = 2;
	curAmmo = 0;
	LoadImages(L"Rocket", true);
}

void Rocket::attack(const Vector2D<float>& hand, const Vector2D<float>& mPos, int side)
{
	if (!curTime && curAmmo > 0)
	{
		Vector2D<float> t = Vector2D<float>(mPos.x - hand.x, mPos.y - hand.y);
		t /= t.GetLenth();
		Bullets.emplace_back(new Bullet(BROCKET, side, hand + Vector2D<float>(t.x * 30, t.y * 30 - 10), t * 5));
		curTime = coolTime;
		shotTime = 30;
		curAmmo--;
	}
}

// ---- Bullet ----

Animation Bullet::animation[10];

void Bullet::init() {
	LoadAnimation(animation[BPISTOL], L"resources/Bullet_Pistol.png", 4, 3);
	LoadAnimation(animation[BRIFLE], L"resources/Bullet_Rifle.png", 4, 3);
	LoadAnimation(animation[BSHOTGUN], L"resources/Bullet_Shotgun.png", 4, 3);
	LoadAnimation(animation[BROCKET], L"resources/Bullet_Rocket.png", 5, 3);
	LoadAnimation(animation[BENEMY], L"resources/enemy_bullet3.png", 4, 3);
	LoadAnimation(animation[BBOUNCE], L"resources/bounced_bullet.png", 4, 3);
	LoadAnimation(animation[BOSSBULLET1], L"resources/boss_bullet1.png", 4, 3);
	LoadAnimation(animation[BOSSBULLET2], L"resources/boss_bullet2.png", 1, 0);
	LoadAnimation(animation[BOSSBULLET3], L"resources/boss_bullet3.png", 4, 2);
	LoadAnimation(animation[BOUNCEDBOSSBULLET2], L"resources/boss_bullet2_bounced.png", 1, 0);
}

void Bullet::destroy() {
	for (int i = 0; i < 10; i++)
		animation[i].resource.Destroy();
}

Bullet::Bullet(int type, int side, Vector2D<float> pos, Vector2D<float> dir) :
	pos(pos), dir(dir), frame(0), type(type), side(side)
{
	col = new Collider(8);
	if (side == enemyBullet) {
		switch (this->type)
		{
		case BOSSBULLET1:
			col->size.x = col->size.y = 10;
			break;
		case BOSSBULLET2:
			col->size.x = col->size.y = 10;
			velocity = 2;
			break;
		case BOSSBULLET3:
			col->size.x = col->size.y = 10;
			velocity = 2;
			break;
		default:
			this->type = BENEMY;
			velocity = 0.5;
			break;
		}
	}
	else {
		switch (this->type)
		{
		case BPISTOL:
			SoundManager::getInstance()->play(PISTOL_SHOT);
			break;
		case BRIFLE:
			SoundManager::getInstance()->play(RIFLE_SHOT);
			break;
		case BSHOTGUN:
			SoundManager::getInstance()->play(SHOTGUN_SHOT);
			break;
		case BROCKET:
			SoundManager::getInstance()->play(ROCKET_SHOT);
			break;
		default:
			break;
		}
		velocity = 2;
	}
	col->layer = side;
	col->owner = this;
	col->damage = BulletDamage[type];
	col->pos = pos;
	COLL.emplace_back(col);
	angle = std::atan2(dir.y, dir.x) * (180.0f / M_PI) * -1;
}

void Bullet::draw_bullet(HDC mDC) {
	int width = animation[type].size.right;
	int height = animation[type].size.bottom;
	if (type != BOSSBULLET1 && type != BOSSBULLET3) {
		CImage temp;
		temp.Create(width, height, 32);
		CImage rotatedImage;
		rotatedImage.Create(width, height, 32);
		for (int y = 0; y < height; y++) {
			for (int x = 0; x < width; x++) {
				BYTE* srcPixel = (BYTE*)animation[type].resource.GetPixelAddress((int)frame * width + x, y);
				BYTE* destPixel = (BYTE*)temp.GetPixelAddress(x, y);
				memcpy(destPixel, srcPixel, sizeof(BYTE) * 4);
			}
		}
		for (int y = 0; y < height; y++) {
			for (int x = 0; x < width; x++) {
				Vector2D<float> rotatedPos = (Vector2D<float>(x - width / 2, y - height / 2)).Rotate(angle) + Vector2D<float>(width / 2, height / 2);
				if (rotatedPos.x >= 0 && rotatedPos.x < width && rotatedPos.y >= 0 && rotatedPos.y < height) {
					BYTE* srcPixel = (BYTE*)temp.GetPixelAddress(rotatedPos.x, rotatedPos.y);
					BYTE* destPixel = (BYTE*)rotatedImage.GetPixelAddress(x, y);
					memcpy(destPixel, srcPixel, sizeof(BYTE) * 4);
				}
			}
		}
		rotatedImage.TransparentBlt(mDC, pos.x - animation[type].size.right / 2, pos.y - animation[type].size.bottom / 2, animation[type].size.right, animation[type].size.bottom,
			0, 0, animation[type].size.right, animation[type].size.bottom, RGB(0, 0, 0)
		);
		rotatedImage.Destroy();
		temp.Destroy();
	}
	else {
		animation[type].resource.Draw(mDC, pos.x - animation[type].size.right, pos.y - animation[type].size.bottom, animation[type].size.right * 2, animation[type].size.bottom * 2,
			animation[type].size.right * (int)frame, 0, animation[type].size.right, animation[type].size.bottom
		);
	}
}

void Bullet::update()
{
	if (type != BOSSBULLET1)
		pos += dir * velocity;
	else {
		pos = (pos - dir).Rotate(2) + dir;
		angle += 2;
		if (angle >= 600 || Boss::getInstance()->getHP() <= 0)
			collisionMsg.emplace(CollisionMessage(this, enemy, 0));
	}
	frame = (frame + frame_time * animation[type].velocity * animation[type].frame);
	if (frame >= animation[type].frame) frame = 0;
	col->pos = pos;
}

void Bullet::remove()
{
	releaseCollider();
	std::erase(Bullets, this);
	deleteSet.insert(this);
}

void Bullet::handle_collision(int otherLayer, int damage)
{
	switch (otherLayer)
	{
	case wall:
	case player:
		if (type == BOSSBULLET1)
			return;
	case enemy:
		switch (type)
		{
		case BPISTOL:
			EffectManager::getInstance()->set_effect(new Effect(CEffect::PISTOLBULLET, col->pos));
			break;
		case BRIFLE:
			EffectManager::getInstance()->set_effect(new Effect(CEffect::RIFLEBULLET, col->pos));
			break;
		case BSHOTGUN:
			EffectManager::getInstance()->set_effect(new Effect(CEffect::SHOTGUNBULLET, col->pos));
			break;
		case BROCKET:
			EffectManager::getInstance()->set_effect(new Effect(CEffect::EXPLOSION, col->pos));
			// 폭발로 죽은 적의 충돌체가 COLL에서 빠지므로 복사본으로 순회
			for (auto other : std::vector<Collider*>(COLL)) {
				for (int i = 0; i < 4; i++) {
					if (other->layer == enemy) {
						Vector2D<float> dot[4] = { Vector2D<float>(other->pos.x - other->size.x, other->pos.y - other->size.y),
						Vector2D<float>(other->pos.x + other->size.x, other->pos.y - other->size.y),
						Vector2D<float>(other->pos.x + other->size.x, other->pos.y + other->size.y),
						Vector2D<float>(other->pos.x - other->size.x, other->pos.y + other->size.y) };
						if ((dot[i] - pos).GetLenth() <= 100) {
							other->owner->handle_collision(playerBullet, 20);
							break;
						}
					}
				}
			}
			break;
		case BOSSBULLET1:
		case BOSSBULLET3:
			EffectManager::getInstance()->set_effect(new Effect(CEffect::PATTERNA, col->pos));
			break;
		case BOSSBULLET2:
		case BENEMY:
			EffectManager::getInstance()->set_effect(new Effect(CEffect::ENEMYBULLET, col->pos));
			break;
		case BBOUNCE:
		case BOUNCEDBOSSBULLET2:
			EffectManager::getInstance()->set_effect(new Effect(CEffect::BOUNCEBULLET, col->pos));
			break;
		default:
			break;
		}
		remove();
		break;
	case playerMelee:
		if (type == BOSSBULLET1 || type == BOSSBULLET3)
			return;
		if (selectedPlayer != knight)
			remove();
		else {	// 기사: 칼로 총알 반사
			dir = Player::getInstance()->GetMouseVector() * 10;
			angle = Player::getInstance()->GetAngle() * -1;
			col->layer = playerBullet;
			if (type == BOSSBULLET2)
				type = BOUNCEDBOSSBULLET2;
			else
				type = BBOUNCE;
		}
		break;
	default:
		break;
	}
}

// ---- DropItem ----

CImage DropItem::image[4];

DropItem::DropItem(int type, Vector2D<float> pos) : type(type), pos(pos), xDir(0)
{
	col = new Collider(Vector2D<float>(image[type].GetWidth(), image[type].GetHeight()));
	col->layer = dropitem;
	col->owner = this;
	col->pos = pos;
	col->damage = type;
	COLL.emplace_back(col);
}

void DropItem::init()
{
	image[DROP::DPISTOL].Load(L"resources/drop_pistol.png");
	image[DROP::DRIFLE].Load(L"resources/drop_rifle.png");
	image[DROP::DSHOTGUN].Load(L"resources/drop_shotgun.png");
	image[DROP::DROCKET].Load(L"resources/drop_rocket.png");
}

void DropItem::destroy()
{
	for (int i = 0; i < 4; i++)
		image[i].Destroy();
}

void DropItem::update()
{
	pos.y += std::sin(xDir / 3.14) / 3;
	xDir += 0.1;
	col->pos = pos;
}

void DropItem::Draw(HDC mDC)
{
	int width = image[type].GetWidth() * 2;
	int height = image[type].GetHeight() * 2;
	image[type].Draw(mDC, pos.x - width / 2, pos.y - height / 2, width, height);
}

void DropItem::handle_collision(int otherLayer, int damage)
{
	if (otherLayer == player || otherLayer == rolled_player || otherLayer == damaged_player) {
		if (!Player::getInstance()->GetIfWeaponFull(type + 1)) {
			Player::getInstance()->WeaponReload(type + 1);
			Player::getInstance()->SetWeapon(type + 1);
			SoundManager::getInstance()->play(type * 2 + 9);	// PISTOL_RELOAD, RIFLE_RELOAD, ...
			releaseCollider();
			std::erase(drops, this);
			deleteSet.insert(this);
		}
	}
}

void DropItem::Clear()
{
	while (!drops.empty())
	{
		delete (*drops.begin());
		drops.erase(drops.begin());
	}
}
