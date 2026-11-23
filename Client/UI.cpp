#include "UI.h"
#include "Player.h"
#include "Boss.h"
#include "Map.h"
#include "GameData.h"
#include "Protocol.h"
#include <algorithm>

#define HeartSize 90

UI* UI::instance = nullptr;

UI::UI()
{
	heart.Load(L"resources/UI_Image_Heart.png");
	weapons.Load(L"resources/UI_Image_Weapons.png");
	bossHp.Load(L"resources/boss_hp.png");
	bossHpFrame.Load(L"resources/boss_hp_frame.png");
	bossHpFrame2.Load(L"resources/boss_hp_frame2.png");
	ammoTextBox = RECT(monitorSize.x - 250, 170, monitorSize.x - 10, 200);
	romulus = CreateFont(48, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET,
		OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY,
		DEFAULT_PITCH | FF_DONTCARE, L"Romulus");
}

UI::~UI()
{
	DeleteObject(romulus);
}

UI* UI::getInstance()
{
	if (instance == nullptr)
		instance = new UI;
	return instance;
}

void UI::release()
{
	if (instance != nullptr)
		delete instance;
	instance = nullptr;
}

void UI::draw(HDC mDC)
{
	auto* pp = Player::getInstance();
	int hp = pp->GetHP();
	for (int i = 0; i < hp / 2; ++i)
		heart.Draw(mDC, 10 + i * HeartSize, 10, HeartSize, HeartSize, 0, 0, 15, 15);
	if (hp % 2)
		heart.Draw(mDC, 10 + hp / 2 * HeartSize, 10, HeartSize, HeartSize, 15, 0, 15, 15);
	weapons.AlphaBlend(mDC, monitorSize.x - 250, 10, 240, 160, 0, 0, 48, 32, RGB(200, 200, 200));
	weapons.Draw(mDC, monitorSize.x - 250, 10, 240, 160, 48 + pp->GetWeapon() * 48, 0, 48, 32);
	std::vector<Weapon*>& w = pp->getWeapons();
	for (int i = 0, h = 0; i < w.size(); i++) {
		if (w[i]->GetCurAmmo() != 0 && i != pp->GetWeapon()) {
			weapons.AlphaBlend(mDC, monitorSize.x - 160, 220 + h * 70, 140, 90, 48 + i * 48, 0, 48, 32, RGB(200, 200, 200));
			h++;
		}
	}
	SelectObject(mDC, romulus);
	SetBkMode(mDC, TRANSPARENT);
	SetTextColor(mDC, RGB(255, 255, 255));
	if (pp->GetWeapon())
		swprintf_s(ammoText, 8, L"%2d / %2d", pp->GetWeaponCurAmmo(), pp->GetWeaponMaxAmmo());
	else
		swprintf_s(ammoText, 8, L" ∞ / ∞ ");
	DrawText(mDC, ammoText, 7, &ammoTextBox, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
	if (LevelManager::getInstance()->GetStage() == 4) {
		if (Boss::getInstance()->getHP() > 0) {
			float decreased = 1.0f - (float)Boss::getInstance()->getHP() / BOSSHP;
			int srclenth = (decreased * (bossHp.GetWidth() - 2)) / 2;
			int destlenth = (decreased * (monitorSize.x - 600)) / 2;
			bossHp.Draw(mDC, 300 + destlenth, monitorSize.y - 150, monitorSize.x - 600 - destlenth * 2, 140, srclenth, 0, bossHp.GetWidth() - srclenth * 2, bossHp.GetHeight());
			bossHpFrame.Draw(mDC, 300, monitorSize.y - 150, monitorSize.x - 600, 140, 0, 0, bossHpFrame.GetWidth(), bossHp.GetHeight());
			bossHpFrame2.Draw(mDC, 300, monitorSize.y - 150, monitorSize.x - 600, 140, 0, 0, bossHpFrame.GetWidth(), bossHp.GetHeight());
		}
	}
	cursor.Draw(mDC, mPoint.x - 20, mPoint.y - 30, 40, 40);
}

void UI::draw(HDC mDC, const PlayerSnap& player)
{
	const int width = screen.right - screen.left;
	const int size = (std::min)(HeartSize, width / 14);
	if (!player.used || size <= 0)
		return;

	const int hp = (std::min)(static_cast<int>(player.hp), GameData::Players[CHAR_MARIN].hp);
	for (int i = 0; i < (hp + 1) / 2; ++i) {
		const int sourceX = hp - i * 2 == 1 ? 15 : 0;
		heart.Draw(mDC, screen.left + 10 + i * size, screen.top + 10,
			size, size, sourceX, 0, 15, 15);
	}
	if (player.weapon >= WEAPON_COUNT)
		return;

	const int iconWidth = (std::min)(240, width / 4);
	const int iconHeight = iconWidth * 2 / 3;
	const int right = screen.right - 10;
	const int x = right - iconWidth;
	const int y = screen.top + 10;
	weapons.Draw(mDC, x, y, iconWidth, iconHeight, 0, 0, 48, 32);
	weapons.Draw(mDC, x, y, iconWidth, iconHeight, 48 + player.weapon * 48, 0, 48, 32);

	wchar_t ammo[16]{};
	if (player.weapon == WEAPON_SWORD)
		wcscpy_s(ammo, L"∞ / ∞");
	else
		swprintf_s(ammo, L"%d / %d", player.ammo[player.weapon], GameData::Weapons[player.weapon].maxAmmo);
	const int saved = SaveDC(mDC);
	SelectObject(mDC, romulus);
	SetBkMode(mDC, TRANSPARENT);
	SetTextColor(mDC, RGB(255, 255, 255));
	RECT label{ x, y + iconHeight, right, y + iconHeight + 40 };
	DrawTextW(mDC, ammo, -1, &label, DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_NOCLIP);
	RestoreDC(mDC, saved);

	const int smallWidth = iconWidth * 7 / 12;
	const int smallHeight = smallWidth * 2 / 3;
	int rowY = label.bottom;
	for (int i = WEAPON_PISTOL; i < WEAPON_COUNT; ++i) {
		if (i == player.weapon || player.ammo[i] == 0)
			continue;
		weapons.Draw(mDC, right - smallWidth, rowY, smallWidth, smallHeight,
			48 + i * 48, 0, 48, 32);
		rowY += smallHeight;
	}
}
