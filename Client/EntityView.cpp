#include "EntityView.h"
#include "GameData.h"
#include "Map.h"
#include "Weapon.h"
#include <algorithm>
#include <cmath>
#include <string>

namespace {
	struct PlayerSprites
	{
		Animation body[STATE_ROLL + 1][6]{};
		CImage hand;

		explicit PlayerSprites(CharType type)
		{
			const std::wstring base = type == CHAR_KNIGHT ? L"resources/knight" : L"resources/Marin";
			const wchar_t* directions[] = { L"front", L"front_right", L"front_left",
				L"back", L"back_right", L"back_left" };
			for (int d = 0; d < 6; ++d) {
				const std::wstring suffix = std::wstring(L"_") + directions[d] + L".png";
				LoadAnimation(body[STATE_IDLE][d], (base + L"_idle" + suffix).c_str(), 4, 1.5f);
				LoadAnimation(body[STATE_RUN][d], (base + L"_run" + suffix).c_str(), 6, 1.5f);
				LoadAnimation(body[STATE_ROLL][d], (base + L"_roll" + suffix).c_str(), 9, 2.0f);
				const bool left = type == CHAR_MARIN && (d == FRONT_LEFT || d == BACK_LEFT);
				LoadAnimation(body[STATE_DEAD][d], (base + (left ? L"_dead_left.png" : L"_dead.png")).c_str(),
					type == CHAR_MARIN ? 15 : 12, 0.5f);
			}
			hand.Load((base + L"_hand.png").c_str());
		}
	};

	PlayerSprites& GetPlayerSprites(CharType type)
	{
		if (type == CHAR_KNIGHT) {
			static PlayerSprites knight(CHAR_KNIGHT);
			return knight;
		}
		static PlayerSprites marin(CHAR_MARIN);
		return marin;
	}

	struct BulletSprites
	{
		Animation sheets[BROCKET + 1]{};

		BulletSprites()
		{
			LoadAnimation(sheets[BPISTOL], L"resources/Bullet_Pistol.png", 4, 3);
			LoadAnimation(sheets[BRIFLE], L"resources/Bullet_Rifle.png", 4, 3);
			LoadAnimation(sheets[BSHOTGUN], L"resources/Bullet_Shotgun.png", 4, 3);
			LoadAnimation(sheets[BENEMY], L"resources/enemy_bullet3.png", 4, 3);
			LoadAnimation(sheets[BBOUNCE], L"resources/bounced_bullet.png", 4, 3);
			LoadAnimation(sheets[BOSSBULLET1], L"resources/boss_bullet1.png", 4, 3);
			LoadAnimation(sheets[BOSSBULLET2], L"resources/boss_bullet2.png", 1, 0);
			LoadAnimation(sheets[BOSSBULLET3], L"resources/boss_bullet3.png", 4, 2);
			LoadAnimation(sheets[BOUNCEDBOSSBULLET2], L"resources/boss_bullet2_bounced.png", 1, 0);
			LoadAnimation(sheets[BROCKET], L"resources/Bullet_Rocket.png", 5, 3);
		}
	};

	struct EnemySprites
	{
		Animation spawn{};
		Animation gunner[3][4][4]{};
		Animation bat[2]{};
		Animation banshee[4][3]{};

		EnemySprites()
		{
			LoadAnimation(spawn, L"resources/Enemy_Spawn.png", 17, 0.5f);
			// 앞, 오른쪽, 왼쪽, 뒤 순서
			const wchar_t* const names[3][4][4] = {
				{
					{ L"pistol_right", L"pistol_right", L"pistol_left", L"pistol_back" },
					{ L"pistol_run_front", L"pistol_run_right", L"pistol_run_left", L"pistol_run_back" },
					{ L"pistol_damaged_front", L"pistol_damaged_right", L"pistol_damaged_left", L"pistol_damaged_front" },
					{ L"pistol_dead_front", L"pistol_dead_front", L"pistol_dead_front", L"pistol_dead_back" },
				},
				{
					{ L"rifle_right", L"rifle_right", L"rifle_left", L"rifle_back" },
					{ L"rifle_run_front", L"rifle_run_front_right", L"rifle_run_front_left", L"rifle_run_back" },
					{ L"rifle_damaged_front", L"rifle_damaged_right", L"rifle_damaged_left", L"rifle_damaged_front" },
					{ L"rifle_dead_front", L"rifle_dead_front", L"rifle_dead_front", L"rifle_dead_back" },
				},
				{
					{ L"shotgun_front", L"shotgun_front", L"shotgun_front", L"shotgun_back" },
					{ L"shotgun_run_front", L"shotgun_run_right", L"shotgun_run_left", L"shotgun_run_back" },
					{ L"shotgun_damaged_front", L"shotgun_damaged_right", L"shotgun_damaged_left", L"shotgun_damaged_front" },
					{ L"shotgun_dead", L"shotgun_dead", L"shotgun_dead", L"shotgun_dead" },
				},
			};
			for (int type = 0; type < 3; ++type) {
				const int frames[] = { 2, 6, 2, type == ENEMY_SHOTGUNMAN ? 3 : 5 };
				for (int state = 0; state < 4; ++state) {
					for (int dir = 0; dir < 4; ++dir) {
						const auto path = std::wstring(L"resources/enemy_") + names[type][state][dir] + L".png";
						LoadAnimation(gunner[type][state][dir], path.c_str(), frames[state],
							state == STATE_DAMAGED ? 3.0f : 2.0f);
					}
				}
			}
			LoadAnimation(bat[0], L"resources/bat.png", 6, 2);
			LoadAnimation(bat[1], L"resources/bat_damaged.png", 4, 2);
			LoadAnimation(banshee[STATE_IDLE][FRONT], L"resources/Bombshe_front.png", 4, 2);
			LoadAnimation(banshee[STATE_IDLE][FRONT_RIGHT], L"resources/Bombshe_right.png", 4, 2);
			LoadAnimation(banshee[STATE_IDLE][FRONT_LEFT], L"resources/Bombshe_left.png", 4, 2);
			LoadAnimation(banshee[STATE_RUN][FRONT], L"resources/Bombshe_attack.png", 5, 2);
			LoadAnimation(banshee[STATE_DAMAGED][FRONT], L"resources/Bombshe_damaged.png", 1, 8);
			LoadAnimation(banshee[STATE_DEAD][FRONT], L"resources/Bombshe_dead.png", 2, 3);
		}
	};

	struct DropSprites
	{
		CImage items[DROCKET + 1];

		DropSprites()
		{
			items[DPISTOL].Load(L"resources/drop_pistol.png");
			items[DRIFLE].Load(L"resources/drop_rifle.png");
			items[DSHOTGUN].Load(L"resources/drop_shotgun.png");
			items[DROCKET].Load(L"resources/drop_rocket.png");
		}
	};
}

Vector2D<float> EntityView::Interpolate(float alpha) const
{
	if (!std::isfinite(alpha))
		alpha = 0;
	alpha = std::clamp(alpha, 0.0f, 1.0f);
	return previousPosition * (1 - alpha) + currentPosition * alpha;
}

void PlayerView::update()
{
	if (!snapshot.used || !hasPosition)
		return;

	renderPosition = Interpolate(interpolation);
	const float angle = snapshot.angle;
	if (angle >= -20 && angle <= 60) direction = FRONT_RIGHT;
	else if (angle > 60 && angle <= 120) direction = FRONT;
	else if (angle > 120 || angle < -160) direction = FRONT_LEFT;
	else if (angle < -120) direction = BACK_LEFT;
	else if (angle < -60) direction = BACK;
	else direction = BACK_RIGHT;

	int state = snapshot.state;
	if (snapshot.hp == 0 || state == STATE_DEAD)
		state = STATE_DEAD;
	else if (state != STATE_RUN && state != STATE_ROLL)
		state = STATE_IDLE;

	const Animation& anim = GetPlayerSprites(charType).body[state][direction];
	if (animationState != state) {
		animationState = state;
		animationFrame = 0.0f;
	}
	animationFrame += static_cast<float>(frame_time) * anim.velocity * anim.frame;
	if (animationFrame >= anim.frame)
		animationFrame = state == STATE_DEAD ? static_cast<float>(anim.frame - 1) : 0.0f;
}

void PlayerView::draw(HDC dc)
{
	if (!snapshot.used || !hasPosition)
		return;

	auto& sprites = GetPlayerSprites(charType);
	auto& anim = sprites.body[animationState][direction];
	if (anim.resource.IsNull())
		return;

	const int x = static_cast<int>(renderPosition.x);
	const int y = static_cast<int>(renderPosition.y);
	const int width = anim.size.right;
	const int height = anim.size.bottom;
	const int top = y - height * 2 + 20;
	const bool showHand = animationState != STATE_ROLL && animationState != STATE_DEAD;
	const bool front = direction == FRONT || direction == FRONT_LEFT || direction == FRONT_RIGHT;
	const auto drawHand = [&] {
		static Sword sword;
		static Pistol pistol;
		static Rifle rifle;
		static Shotgun shotgun;
		static Rocket rocket;
		Weapon* const weapons[] = { &sword, &pistol, &rifle, &shotgun, &rocket };
		const float radians = snapshot.angle * static_cast<float>(M_PI / 180.0);
		const Vector2D<float> center(renderPosition.x + (std::abs(snapshot.angle) < 90 ? 18 : -17),
			renderPosition.y + 8);
		if (snapshot.weapon < WEAPON_COUNT) {
			const Vector2D<float> aim = center + Vector2D<float>(std::cos(radians), std::sin(radians)) * 100.0f;
			weapons[snapshot.weapon]->draw_weapon(dc, center, aim);
		}
		if (!sprites.hand.IsNull()) {
			const int handWidth = sprites.hand.GetWidth();
			const int handHeight = sprites.hand.GetHeight();
			sprites.hand.Draw(dc, static_cast<int>(center.x) - handWidth,
				static_cast<int>(center.y) - handHeight, handWidth * 2, handHeight * 2);
		}
	};
	if (showHand && !front)
		drawHand();
	anim.resource.Draw(dc, x - width, top, width * 2, height * 2,
		static_cast<int>(animationFrame) * width, 0, width, height);
	if (showHand && front)
		drawHand();

	if (nickname[0]) {
		const int oldMode = SetBkMode(dc, TRANSPARENT);
		const COLORREF oldColor = SetTextColor(dc, RGB(255, 255, 255));
		RECT label{ x - 130, top - 25, x + 130, top };
		DrawTextW(dc, nickname, -1, &label, DT_CENTER | DT_SINGLELINE | DT_NOPREFIX);
		SetTextColor(dc, oldColor);
		SetBkMode(dc, oldMode);
	}
}

void EnemyView::update()
{
	if (!hasPosition || !hasType || finished)
		return;
	if (!dying)
		renderPosition = Interpolate(interpolation);

	static const EnemySprites sprites;
	const float elapsed = static_cast<float>(frame_time);
	if (!dying && spawnTime > 0) {
		spawnTime = (std::max)(0.0f, spawnTime - elapsed);
		if (spawnTime > 0) {
			animation = &sprites.spawn;
			frame = sprites.spawn.frame * (1 - spawnTime / GameData::EnemySpawnSeconds);
			return;
		}
	}

	const float angle = snapshot.angle;
	const int direction = angle >= 60 && angle <= 120 ? FRONT :
		angle >= -120 && angle <= -60 ? BACK : std::abs(angle) < 90 ? FRONT_RIGHT : FRONT_LEFT;
	const int state = dying ? STATE_DEAD : snapshot.state < STATE_DEAD ? snapshot.state : STATE_IDLE;
	if (type == ENEMY_BAT)
		animation = &sprites.bat[dying || state != STATE_IDLE ? 1 : 0];
	else if (type == ENEMY_BOMBSHE)
		animation = &sprites.banshee[state][state == STATE_IDLE && direction != BACK ? direction : FRONT];
	else
		animation = &sprites.gunner[type][state][direction];

	if (animationState != state) {
		animationState = state;
		frame = 0.0f;
	} else {
		frame += elapsed * animation->velocity * animation->frame;
	}
	if (dying)
		finished = frame >= animation->frame;
	else if (frame >= animation->frame)
		frame = std::fmod(frame, static_cast<float>(animation->frame));
}

void EnemyView::draw(HDC dc)
{
	if (!animation || finished || animation->resource.IsNull())
		return;
	const int width = animation->size.right;
	const int height = animation->size.bottom;
	const int x = static_cast<int>(renderPosition.x) - width;
	const int y = static_cast<int>(renderPosition.y) - (spawnTime > 0 && !dying ? height : height * 2 - 20);
	const int sourceX = std::clamp(static_cast<int>(frame), 0, animation->frame - 1) * width;
	animation->resource.Draw(dc, x, y, width * 2, height * 2, sourceX, 0, width, height);
}

void BossView::update()
{
	// [C5] update: 보스 애니메이션 갱신
}

void BossView::draw(HDC dc)
{
	// [C9] draw: 보스 본체와 손 그리기
}

void BulletView::update()
{
	age += static_cast<float>(frame_time);
	const Vector2D<float> start(spawn.x, spawn.y);
	if (spawn.type == BOSSBULLET1) {
		const float degrees = (std::min)(GameData::OrbitBulletDegreesPerSecond * age, 600.0f);
		position = (start - direction).Rotate(degrees) + direction;
	} else {
		position = start + direction * (GameData::Bullets[spawn.type].speedPerSecond * age);
	}
}

void BulletView::draw(HDC dc)
{
	if (!(position.x >= -PTS && position.x <= MapWidth * PTS + PTS &&
		position.y >= -PTS && position.y <= MapHeight * PTS + PTS))
		return;
	static BulletSprites sprites;
	auto& anim = sprites.sheets[spawn.type];
	if (anim.resource.IsNull())
		return;
	const int width = anim.size.right;
	const int height = anim.size.bottom;
	const int frame = static_cast<int>(std::fmod(age * anim.velocity * anim.frame, anim.frame));
	const int x = static_cast<int>(position.x);
	const int y = static_cast<int>(position.y);
	if (spawn.type == BOSSBULLET1 || spawn.type == BOSSBULLET3) {
		anim.resource.Draw(dc, x - width, y - height, width * 2, height * 2,
			frame * width, 0, width, height);
		return;
	}

	CImage rotated;
	if (!rotated.Create(width, height, 32))
		return;
	for (int py = 0; py < height; ++py) {
		for (int px = 0; px < width; ++px) {
			const float dx = px - width * 0.5f;
			const float dy = py - height * 0.5f;
			const float sx = dx * direction.x + dy * direction.y + width * 0.5f;
			const float sy = dy * direction.x - dx * direction.y + height * 0.5f;
			auto* pixel = static_cast<DWORD*>(rotated.GetPixelAddress(px, py));
			*pixel = 0;
			if (sx >= 0 && sx < width && sy >= 0 && sy < height) {
				const auto* source = static_cast<const DWORD*>(anim.resource.GetPixelAddress(
					frame * width + static_cast<int>(sx), static_cast<int>(sy)));
				*pixel = *source;
			}
		}
	}
	rotated.TransparentBlt(dc, x - width / 2, y - height / 2, width, height, RGB(0, 0, 0));
}

void DropView::draw(HDC dc)
{
	static const DropSprites sprites;
	const auto& image = sprites.items[drop.type];
	if (!image.IsNull()) {
		const int width = image.GetWidth();
		const int height = image.GetHeight();
		image.Draw(dc, static_cast<int>(drop.x) - width, static_cast<int>(drop.y) - height, width * 2, height * 2);
	}
}
