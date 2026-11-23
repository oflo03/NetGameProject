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
	// [C5] update: 적 애니메이션 갱신
}

void EnemyView::draw(HDC dc)
{
	// [C9] draw: 적 그리기
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
	// [C9] draw: 드랍 아이템 그리기
}
