#include "EntityView.h"
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
	// [C6] update: 서버와 같은 공식으로 총알 이동 재현
}

void BulletView::draw(HDC dc)
{
	// [C9] draw: 총알 그리기
}

void DropView::draw(HDC dc)
{
	// [C9] draw: 드랍 아이템 그리기
}
