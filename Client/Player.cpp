#include "Player.h"
#include <string>
#include "Effect.h"
#include "Sound.h"

Vector2D<float> spawnpoint[4]{
	Vector2D<float>(224,256),
	Vector2D<float>(960,256),
	Vector2D<float>(224,540),
	Vector2D<float>(960,1000)
};

int selectedPlayer;

// ---- 플레이어 상태 (FSM) ----

class PlayerState
{
public:
	virtual ~PlayerState() = default;
	virtual void enter(Player& player) = 0;
	virtual PlayerState* handle_event(Player& player) = 0;
	virtual void update(Player& player) = 0;
};

class IdleState : public PlayerState
{
public:
	virtual PlayerState* handle_event(Player& player);
	virtual void update(Player& player) {}
	virtual void enter(Player& player) { player.SetCurState(STATE_IDLE); }
};

class RunState : public PlayerState
{
public:
	virtual PlayerState* handle_event(Player& player);
	virtual void update(Player& player);
	virtual void enter(Player& player) { player.SetCurState(STATE_RUN); }
};

class RollState : public PlayerState
{
private:
	float rollingTime;
	Vector2D<float> dir;
public:
	RollState(float dirX, float dirY) : dir(dirX, dirY) { rollingTime = 0; }
	virtual PlayerState* handle_event(Player& player);
	virtual void update(Player& player);
	virtual void enter(Player& player);
};

class DeadState : public PlayerState
{
public:
	virtual PlayerState* handle_event(Player& player) { return nullptr; }
	virtual void update(Player& player) {}
	virtual void enter(Player& player);
};

PlayerState* IdleState::handle_event(Player& player)
{
	float dirX = 0, dirY = 0;
	bool isMove = false;

	if (GetAsyncKeyState('D')) {
		dirX = 1;
		isMove = true;
	}
	else if (GetAsyncKeyState('A')) {
		dirX = -1;
		isMove = true;
	}
	if (GetAsyncKeyState('W')) {
		dirY = -1;
		isMove = true;
	}
	else if (GetAsyncKeyState('S')) {
		dirY = 1;
		isMove = true;
	}
	if (GetAsyncKeyState('1') & 0x8000 && player.GetWeapon() != SWORD)
		player.SetWeapon(SWORD);
	if (GetAsyncKeyState('2') & 0x8000 && player.GetWeapon() != PISTOL)
		player.SetWeapon(PISTOL);
	if (GetAsyncKeyState('3') & 0x8000 && player.GetWeapon() != RIFLE)
		player.SetWeapon(RIFLE);
	if (GetAsyncKeyState('4') & 0x8000 && player.GetWeapon() != SHOTGUN)
		player.SetWeapon(SHOTGUN);
	if (GetAsyncKeyState('5') & 0x8000 && player.GetWeapon() != ROCKET)
		player.SetWeapon(ROCKET);
	if (GetAsyncKeyState(VK_CONTROL) & 1)
		player.SetWeaponUp();
	if (GetAsyncKeyState(VK_TAB) & 1)
		player.SetWeaponDown();
	if (GetAsyncKeyState(VK_XBUTTON1) & 1)
		player.SetWeaponUp();
	if (GetAsyncKeyState(VK_XBUTTON2) & 1)
		player.SetWeaponDown();
	player.SetDirection();
	if (GetAsyncKeyState(VK_LBUTTON) & 0x8000)
		player.attack();
	GetAsyncKeyState(VK_RBUTTON);	// 구르기 입력이 Idle에서 쌓이지 않도록 비움
	if (isMove) {
		player.SetDir(Vector2D<float>(dirX, dirY));
		return new RunState();
	}
	return nullptr;
}

PlayerState* RunState::handle_event(Player& player)
{
	float dirX = 0, dirY = 0;
	bool isMove = false;
	if (GetAsyncKeyState('D')) {
		dirX = 1;
		isMove = true;
	}
	else if (GetAsyncKeyState('A')) {
		dirX = -1;
		isMove = true;
	}
	else dirX = 0;
	if (GetAsyncKeyState('W')) {
		dirY = -1;
		isMove = true;
	}
	else if (GetAsyncKeyState('S')) {
		dirY = 1;
		isMove = true;
	}
	else dirY = 0;
	player.SetDirection();
	if (GetAsyncKeyState(VK_LBUTTON) & 0x8000)
		player.attack();
	if (GetAsyncKeyState(VK_RBUTTON) & 1) {
		if (dirX == 1 && dirY == 0)
			player.SetDirection(FRONT_RIGHT);
		else if (dirX == 1 && dirY == 1)
			player.SetDirection(FRONT_RIGHT);
		else if (dirX == 0 && dirY == 1)
			player.SetDirection(FRONT);
		else if (dirX == -1 && dirY == 1)
			player.SetDirection(FRONT_LEFT);
		else if (dirX == -1 && dirY == 0)
			player.SetDirection(FRONT_LEFT);
		else if (dirX == -1 && dirY == -1)
			player.SetDirection(BACK_LEFT);
		else if (dirX == 0 && dirY == -1)
			player.SetDirection(BACK);
		else if (dirX == 1 && dirY == -1)
			player.SetDirection(BACK_RIGHT);
		player.SetDir(Vector2D<float>(0, 0));
		return new RollState(dirX, dirY);
	}
	if (GetAsyncKeyState('1') & 0x8000 && player.GetWeapon() != SWORD) {
		player.SetWeapon(SWORD);
	}
	if (GetAsyncKeyState('2') & 0x8000 && player.GetWeapon() != PISTOL) {
		player.SetWeapon(PISTOL);
		if (SoundManager::getInstance()->isPlaying(PISTOL_RELOAD))
			SoundManager::getInstance()->play(PISTOL_RELOAD);
	}
	if (GetAsyncKeyState('3') & 0x8000 && player.GetWeapon() != RIFLE) {
		player.SetWeapon(RIFLE);
		if (SoundManager::getInstance()->isPlaying(RIFLE_RELOAD))
			SoundManager::getInstance()->play(RIFLE_RELOAD);
	}
	if (GetAsyncKeyState('4') & 0x8000 && player.GetWeapon() != SHOTGUN) {
		player.SetWeapon(SHOTGUN);
		if (SoundManager::getInstance()->isPlaying(SHOTGUN_RELOAD))
			SoundManager::getInstance()->play(SHOTGUN_RELOAD);
	}
	if (GetAsyncKeyState('5') & 0x8000 && player.GetWeapon() != ROCKET) {
		player.SetWeapon(ROCKET);
		if (SoundManager::getInstance()->isPlaying(ROCKET_RELOAD))
			SoundManager::getInstance()->play(ROCKET_RELOAD);
	}
	if (GetAsyncKeyState(VK_CONTROL) & 1)
		player.SetWeaponUp();
	if (GetAsyncKeyState(VK_TAB) & 1)
		player.SetWeaponDown();
	if (GetAsyncKeyState(VK_XBUTTON1) & 1)
		player.SetWeaponUp();
	if (GetAsyncKeyState(VK_XBUTTON2) & 1)
		player.SetWeaponDown();
	if (isMove) {
		player.SetDir(Vector2D<float>(dirX, dirY));
		if (!SoundManager::getInstance()->isPlaying(WALK))
			SoundManager::getInstance()->play(WALK);
		return nullptr;
	}
	return new IdleState();
}

void RunState::update(Player& player)
{
	if (player.GetDir().GetLenth() == 1) {
		player.SetPos(player.GetPos() + player.GetDir() * player.GetVelocity() * frame_time);
	}
	else {
		player.SetPos(player.GetPos() + player.GetDir() * (sqrt(2) / 2) * player.GetVelocity() * frame_time);
	}
}

PlayerState* RollState::handle_event(Player& player)
{
	if ((int)rollingTime == 9)
	{
		if (player.col->layer != damaged_player)
			player.col->layer = layertype::player;
		return new IdleState;
	}
	return nullptr;
}

void RollState::update(Player& player)
{
	if (player.col->layer != damaged_player)
		player.col->layer = rolled_player;
	rollingTime = (rollingTime + frame_time * 2 * 9);
	if ((int)rollingTime >= 6) return;
	if (dir.GetLenth() == 1) {
		player.SetPos(player.GetPos() + dir * player.GetVelocity() * 3 * frame_time);
	}
	else {
		player.SetPos(player.GetPos() + dir * (sqrt(2) / 2) * player.GetVelocity() * 3 * frame_time);
	}
}

void RollState::enter(Player& player)
{
	player.SetCurState(STATE_ROLL);
	SoundManager::getInstance()->play(ROLLING);
}

void DeadState::enter(Player& player)
{
	SoundManager::getInstance()->stop(SOUND::MainState);
	SoundManager::getInstance()->play(SOUND::GAME_OVER);
	player.SetCurState(STATE_DEAD);
}

// ---- Player ----

Player* Player::instance = nullptr;

Player* Player::getInstance()
{
	if (instance == nullptr)
		instance = new Player(selectedPlayer, spawnpoint[0].x, spawnpoint[0].y);
	return instance;
}

void Player::release()
{
	if (instance != nullptr)
		delete instance;
	instance = nullptr;
}

Player::Player(int type, float x, float y) : type(type), pos(x, y), dir(0, 0), frame(0), damageCnt(0), angle(90), direction(FRONT),
	state(nullptr), selectedWeapon(SWORD), velocity(200), hp(6), curstate(STATE_IDLE)
{
	// resources/{Marin|knight}_{idle|run|roll}_{방향}.png
	static const wchar_t* const names[] = { L"Marin", L"knight" };
	static const wchar_t* const dirs[] = { L"front", L"front_right", L"front_left", L"back", L"back_right", L"back_left" };
	std::wstring base = std::wstring(L"resources/") + names[type];
	for (int d = 0; d < 6; d++) {
		LoadAnimation(animation[STATE_IDLE][d], (base + L"_idle_" + dirs[d] + L".png").c_str(), 4, 1.5);
		LoadAnimation(animation[STATE_RUN][d], (base + L"_run_" + dirs[d] + L".png").c_str(), 6, 1.5);
		LoadAnimation(animation[STATE_ROLL][d], (base + L"_roll_" + dirs[d] + L".png").c_str(), 9, 2);
		bool left = type == marin && (d == FRONT_LEFT || d == BACK_LEFT);
		LoadAnimation(animation[STATE_DEAD][d], (base + (left ? L"_dead_left.png" : L"_dead.png")).c_str(), type == marin ? 15 : 12, 0.5);
	}
	hand.Load((base + L"_hand.png").c_str());
	shadow[0].Load(L"resources/shadow.png");
	shadow[1].Load(L"resources/shadow2.png");
	if (type == marin)
		hp = 10;
	state = new IdleState;
	col = new Collider(Vector2D<float>(animation[STATE_IDLE][FRONT].size.right, animation[STATE_IDLE][FRONT].size.bottom));
	col->owner = this;
	col->layer = player;
	col->pos = pos;
	col->damage = 5;
	COLL.emplace_back(col);
	myWeapons.emplace_back(new Sword);
	myWeapons.emplace_back(new Pistol);
	myWeapons.emplace_back(new Rifle);
	myWeapons.emplace_back(new Shotgun);
	myWeapons.emplace_back(new Rocket);
}

Player::~Player()
{
	for (auto w : myWeapons)
		delete w;
	delete state;
}

void Player::draw_character(HDC mDC)
{
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
	bool isRolling = col->layer == rolled_player;
	Animation& anim = animation[curstate][direction];
	float yDest = pos.y - (anim.size.bottom - 20) * 2;
	bool blink = col->layer == damaged_player && !((int)(damageCnt / 0.5) % 2);	// 피격 무적 중 깜빡임
	bool showHand = !isRolling && curstate != STATE_DEAD;
	if (curstate != STATE_DEAD)
		shadow[isRolling].Draw(mDC, pos.x - shadow[isRolling].GetWidth(), pos.y + col->size.y - 2 - shadow[isRolling].GetHeight(), shadow[isRolling].GetWidth() * 2, shadow[isRolling].GetHeight() * 2);
	// 앞을 볼 땐 몸 위에 손, 뒤를 볼 땐 몸 아래에 손
	bool front = direction == FRONT || direction == FRONT_RIGHT || direction == FRONT_LEFT;
	if (!front && showHand) {
		myWeapons[selectedWeapon]->draw_weapon(mDC, handPos, mPos);
		hand.Draw(mDC, handPos.x - hand.GetWidth(), handPos.y - hand.GetHeight(), hand.GetWidth() * 2, hand.GetHeight() * 2);
	}
	if (!blink)
		anim.resource.Draw(mDC, pos.x - anim.size.right, yDest - 20, anim.size.right * 2, anim.size.bottom * 2,
			(int)frame * anim.size.right, 0, anim.size.right, anim.size.bottom);
	if (front && showHand) {
		myWeapons[selectedWeapon]->draw_weapon(mDC, handPos, mPos);
		hand.Draw(mDC, handPos.x - hand.GetWidth(), handPos.y - hand.GetHeight(), hand.GetWidth() * 2, hand.GetHeight() * 2);
	}
}

void Player::handle_event()
{
	PlayerState* temp = this->state->handle_event(*this);
	if (temp != nullptr) {
		delete this->state;
		this->state = temp;
		this->state->enter(*this);
		frame = 0;
	}
}

void Player::update()
{
	POINT temp;
	GetCursorPos(&temp);
	mPos.x = temp.x;
	mPos.y = temp.y;
	mPos -= monitorSize / 2;
	mPos /= 2;
	mPos += camPos;
	lastPos = pos;
	state->update(*this);
	Animation& anim = animation[curstate][direction];
	frame = (frame + frame_time * anim.velocity * anim.frame);
	if (frame >= anim.frame)
		frame = (curstate == STATE_DEAD) ? anim.frame - 1 : 0;	// 죽음 모션은 마지막 프레임 유지
	if (damageCnt > 0) damageCnt -= 0.1;
	if (col->layer == damaged_player && (int)damageCnt == 0) col->layer = player;
	myWeapons[selectedWeapon]->update();
	if (myWeapons[selectedWeapon]->GetShotTime() == 0 && myWeapons[selectedWeapon]->IsRunOut())
		SetWeapon(SWORD);
	col->pos = pos;
	if (curstate != STATE_DEAD)
		camPos = pos + (mPos - pos) / 4;
	if (myWeapons[selectedWeapon]->IsReBound()) {
		Vector2D<float> t = mPos - pos;
		t.Normalize();
		t.Rotate(180);
		camPos += t * rebound[selectedWeapon] * (myWeapons[selectedWeapon]->GetCurTime() % 2 ? 1 : -1);
	}
	if (damageCnt > 8) {
		Vector2D<float> t = mPos - pos;
		t.Normalize();
		t.Rotate(180);
		camPos += t * 10 * ((int)(damageCnt / 0.5) % 2 ? 1 : -1);
	}
}

void Player::SetDirection()
{
	angle = std::atan2(mPos.y - (pos.y), mPos.x - pos.x) * (180.0f / M_PI);
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

void Player::hit()
{
	--hp;
	if (hp > 0) {
		SoundManager::getInstance()->play(PLAYER_HURT);
		pos -= dir;
		col->layer = damaged_player;
		damageCnt = 10.0;
	}
	else {
		SoundManager::getInstance()->play(PLAYER_DEATH);
		delete this->state;
		this->state = new DeadState();
		this->state->enter(*this);
		frame = 0;
	}
}

void Player::handle_collision(int otherLayer, int damage)
{
	switch (otherLayer)
	{
	case wall:
	case water:
		if (!isWallCollision(Vector2D<float>(pos.x, lastPos.y), col->size))
			pos.y = lastPos.y;
		else if (!isWallCollision(Vector2D<float>(lastPos.x, pos.y), col->size))
			pos.x = lastPos.x;
		else
			pos = lastPos;
		lastPos = pos;
		col->pos = pos;
		break;
	case enemy:
		if (col->layer == player && beatable)
			hit();
		else
			pos = lastPos;
		lastPos = pos;
		col->pos = pos;
		break;
	case enemyBullet:
		EffectManager::getInstance()->set_effect(new Effect(CEffect::DAMAGED, col->pos));
		if (col->layer == player && beatable)
			hit();
		break;
	default:
		break;
	}
}
