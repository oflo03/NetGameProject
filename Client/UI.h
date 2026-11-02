#pragma once
#include "Common.h"

class Player;

// 플레이 중 HUD (체력, 무기, 탄약, 보스 체력바, 커서)
class UI
{
protected:
	static UI* instance;
	CImage heart, weapons, bossHp, bossHpFrame, bossHpFrame2;
	Player* pp;
	RECT ammoTextBox;
	HFONT romulus;
	TCHAR ammoText[8];
	UI();
	~UI();
public:
	static UI* getInstance();
	static void release();
	void draw(HDC mDC);
};
