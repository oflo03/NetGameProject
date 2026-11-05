#include "LocalPlayState.h"
#include "Player.h"
#include "Enemy.h"
#include "Boss.h"
#include "Map.h"
#include "Effect.h"
#include "Sound.h"
#include "UI.h"
#include "Scenes.h"

LocalPlayState::LocalPlayState() : GameState(), player(Player::getInstance()), enemyManager(EnemyManager::getInstance()), ui(UI::getInstance()), effectManager(EffectManager::getInstance())
, soundManager(SoundManager::getInstance()), mapManager(MapManager::getInstance()), portal(Portal::getInstance()), boss(nullptr), levelManager(LevelManager::getInstance())
{
	Bullet::init();
	DropItem::init();
	for (int i = 1; i < 5; ++i)	// 칼 빼고 총은 빈 채로 시작
		player->SetCurAmmoZero(i);
	soundManager->play(MainState);
}

LocalPlayState::~LocalPlayState()
{
	enemyManager->Clear();
	Bullets.clear();
	Bullet::destroy();
	DropItem::destroy();
	Collider::Clear();
	DropItem::Clear();
	LevelManager::release();
	UI::release();
	Portal::release();
	Boss::release();
	effectManager->clear();
	soundManager->stop(MainState);
	soundManager->stop(BOSS);
}

void LocalPlayState::update()
{
	soundManager->update();
	levelManager->update();
	if (levelManager->GetStage() == 4 && boss == nullptr)
		boss = Boss::getInstance();
	if (boss != nullptr && boss->getHP() <= 0) {
		boss = nullptr;
	}
	for (auto& d : drops)
		d->update();
	player->update();
	enemyManager->update();
	if (boss != nullptr)
		boss->update();
	if (enemyclear)
		portal->update();
	for (auto& B : Bullets)
		B->update();
	effectManager->update();
	RECT bounds = { -100,-100,2000,1000 };	// 맵 밖으로 나간 총알은 벽에 맞은 것으로 처리
	for (auto& B : Bullets) {
		if (!PtInRect(&bounds, POINT(B->col->pos.x, B->col->pos.y))) {
			collisionMsg.emplace(CollisionMessage(B->col->owner, wall, 0));
		}
	}
	ColliderUpdate();
	effectManager->delete_effect();
}

void LocalPlayState::handle_events()
{
	if (GetAsyncKeyState(VK_ESCAPE) & 1) {
		push_state(new OptionState);
		return;
	}
	// ---- 치트키 ----
	else if (GetAsyncKeyState('X') & 1) {
		lookRange = !lookRange;
	}
	else if (GetAsyncKeyState('Z') & 1) {
		beatable = !beatable;
	}
	else if (GetAsyncKeyState('N') & 1) {	// 다음 스테이지
		if (levelManager->GetStage() < 4)
			levelManager->loadNextStage();
	}
	else if (GetAsyncKeyState('R') & 1) {	// 탄약 충전
		for (int i = 0; i < 4; i++)
			player->WeaponReload(i + 1);
	}
	else if (GetAsyncKeyState('K') & 1) {	// 적 전부 제거
		for (int i = 0; i < COLL.size();)
			if (COLL[i]->layer == enemy)
			{
				delete COLL[i];
				COLL.erase(COLL.begin() + i);
			}
			else
				++i;
		EnemyManager::getInstance()->Clear();
	}
	// ---- 상태 전환 ----
	else if (player->GetHP() <= 0) {
		change_state(new GameOverState);
		return;
	}
	else if (boss != nullptr && boss->getHP() <= 0) {
		push_state(new BossDieScene);
		for (int i = 0; i < COLL.size();)
			if (COLL[i]->layer == enemy || COLL[i]->layer == enemyBullet || COLL[i]->layer == playerBullet)
			{
				delete COLL[i];
				COLL.erase(COLL.begin() + i);
			}
			else
				++i;
		EnemyManager::getInstance()->Clear();
		Bullets.clear();
	}
	enemyManager->handle_event();
	player->handle_event();
	if (boss != nullptr)
		boss->handle_event();
	if (enemyclear)
		portal->handle_event();
}

void LocalPlayState::draw()
{
	HDC mapDC = CreateCompatibleDC(mDC);
	HBITMAP mapbitmap = CreateCompatibleBitmap(mDC, 1920, 1080);
	SelectObject(mapDC, mapbitmap);
	mapManager->PrintMap(mapDC);
	for (auto& d : drops)
		d->Draw(mapDC);
	enemyManager->draw(mapDC);
	if (enemyclear)
		if (player->GetPos().y - portal->GetPos().y > 0) {
			portal->Draw(mapDC);
			player->draw_character(mapDC);
		}
		else {
			player->draw_character(mapDC);
			portal->Draw(mapDC);
		}
	else
		player->draw_character(mapDC);
	if (boss != nullptr)
		boss->draw(mapDC);
	for (auto& B : Bullets)
		B->draw_bullet(mapDC);
	if (lookRange)
		for (auto& c : COLL)
			c->draw_range(mapDC);
	effectManager->Draw(mapDC);
	StretchBlt(mDC, 0, 0, screen.right, screen.bottom,
		mapDC, camPos.x - camSize.x, camPos.y - camSize.y, camSize.x * 2, camSize.y * 2, SRCCOPY);
	DeleteObject(mapbitmap);
	DeleteDC(mapDC);
	ui->draw(mDC);
}
