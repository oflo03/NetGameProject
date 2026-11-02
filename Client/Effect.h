#pragma once
#include <memory>
#include "Common.h"

enum CEffect {
	PISTOLBULLET,
	RIFLEBULLET,
	SHOTGUNBULLET,
	ENEMYBULLET,
	BOUNCEBULLET,
	SWORDATTACK,
	BOMBSHE1,
	BOMBSHE2,
	BATDIE,
	BOMBSHEDIE,
	SHOTGUNDIE,
	DAMAGED,
	PATTERNA,
	PATTERNB,
	PATTERNC,
	PATTERND,
	EXPLOSION,
	BOSSHIT,
	TELEPORT,
	EFFECT_AMOUNT
};

class Effect {
public:
	static Animation effect[EFFECT_AMOUNT];
	int type;
	float time;
	Vector2D<float> pos;
	Effect(const CEffect& type, Vector2D<float> pos) : type(type), pos(pos), time(0) {}
	static void init();
	static void destroy();
	void Draw(HDC mDC);
	void update();
	bool TimeOver() const {
		return ((int)time == effect[type].frame);
	}
};

class EffectManager {
private:
	std::vector<std::unique_ptr<Effect>> effect;
	static EffectManager* instance;
	EffectManager() { Effect::init(); }
	~EffectManager() {
		effect.clear();
		instance = nullptr;
		Effect::destroy();
	}
public:
	static EffectManager* getInstance();
	void Draw(HDC mDC);
	void update();
	void set_effect(Effect* e);
	void delete_effect();
	void clear();
};
