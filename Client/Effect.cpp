#include "Effect.h"
#include <algorithm>

// ---- Effect ----

Animation Effect::effect[EFFECT_AMOUNT];

void Effect::init()
{
	LoadAnimation(effect[PISTOLBULLET], L"resources/Bullet_Pistol_effect.png", 4, 3);
	LoadAnimation(effect[RIFLEBULLET], L"resources/Bullet_Rifle_effect.png", 6, 3);
	LoadAnimation(effect[SHOTGUNBULLET], L"resources/Bullet_Shotgun_effect.png", 6, 3);
	LoadAnimation(effect[ENEMYBULLET], L"resources/enemy_bullet_effect.png", 5, 3);
	LoadAnimation(effect[BOUNCEBULLET], L"resources/Bullet_bounce_effect.png", 5, 3);
	LoadAnimation(effect[SWORDATTACK], L"resources/melee_effect.png", 3, 4);
	LoadAnimation(effect[BOMBSHE1], L"resources/banshe_effect.png", 13, 1);
	LoadAnimation(effect[BOMBSHE2], L"resources/banshe_effect2.png", 3, 2.5);
	LoadAnimation(effect[BATDIE], L"resources/bat_explosion.png", 5, 3);
	LoadAnimation(effect[BOMBSHEDIE], L"resources/Bombshe_explode.png", 5, 3);
	LoadAnimation(effect[SHOTGUNDIE], L"resources/enemy_shotgun_explode.png", 4, 4);
	LoadAnimation(effect[DAMAGED], L"resources/player_damaged.png", 5, 4);
	LoadAnimation(effect[PATTERNA], L"resources/boss_patternA.png", 8, 3);
	LoadAnimation(effect[PATTERNB], L"resources/boss_patternB.png", 7, 4);
	LoadAnimation(effect[PATTERNC], L"resources/boss_patternC.png", 9, 2);
	LoadAnimation(effect[PATTERND], L"resources/boss_patternD.png", 6, 2);
	LoadAnimation(effect[EXPLOSION], L"resources/rocket_explosion.png", 18, 2);
	LoadAnimation(effect[BOSSHIT], L"resources/boss_hit.png", 8, 1);
	LoadAnimation(effect[TELEPORT], L"resources/teleport.png", 17, 4);
}

void Effect::destroy()
{
	for (int i = 0; i < EFFECT_AMOUNT; i++)
		effect[i].resource.Destroy();
}

void Effect::Draw(HDC mDC) {
	effect[type].resource.Draw(mDC, pos.x - effect[type].size.right, pos.y - effect[type].size.bottom, effect[type].size.right * 2, effect[type].size.bottom * 2,
		(int)time * effect[type].size.right, 0, effect[type].size.right, effect[type].size.bottom);
}

void Effect::update() {
	time = (time + frame_time * effect[type].velocity * effect[type].frame);
}

// ---- EffectManager ----

EffectManager* EffectManager::instance = nullptr;

EffectManager* EffectManager::getInstance()
{
	if (instance == nullptr)
		instance = new EffectManager;
	return instance;
}

void EffectManager::Draw(HDC mDC)
{
	for (auto& e : effect)
		e->Draw(mDC);
}

void EffectManager::update()
{
	for (auto& e : effect)
		e->update();
}

void EffectManager::set_effect(Effect* e)
{
	effect.emplace_back(e);
}

void EffectManager::delete_effect()
{
	std::erase_if(effect, [](const std::unique_ptr<Effect>& e) { return e->TimeOver(); });
}

void EffectManager::clear()
{
	effect.clear();
}
