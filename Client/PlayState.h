#pragma once
#include "Framework.h"

class Player;
class UI;
class EnemyManager;
class EffectManager;
class LevelManager;
class SoundManager;
class MapManager;
class Portal;
class Boss;

// 실제 게임 플레이 화면. 매 프레임 handle_events -> update -> draw
class PlayState : public GameState
{
public:
	PlayState();
	~PlayState();
	virtual void update();
	virtual void handle_events();
	virtual void draw();
private:
	Player* player;
	UI* ui;
	EnemyManager* enemyManager;
	EffectManager* effectManager;
	LevelManager* levelManager;
	SoundManager* soundManager;
	MapManager* mapManager;
	Portal* portal;
	Boss* boss;
};
