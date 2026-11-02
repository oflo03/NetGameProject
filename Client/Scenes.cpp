#include "Scenes.h"
#include "PlayState.h"
#include "Player.h"
#include "Boss.h"
#include "Map.h"
#include "Effect.h"
#include "Sound.h"

static HFONT MakeFont(int height, const wchar_t* face)
{
	return CreateFont(height, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET,
		OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY,
		DEFAULT_PITCH | FF_DONTCARE, face);
}

// ---- LogoState ----

LogoState::LogoState() : GameState()
{
	tuk.Load(L"resources/main_tuk.png");
	logo.Load(L"resources/main_logo.png");
	logoTime = 0;
}

void LogoState::update()
{
	logoTime += 0.01;
}

void LogoState::handle_events()
{
	if (GetAsyncKeyState(VK_ESCAPE)) {
		PostQuitMessage(0);
		return;
	}
	else if (GetAsyncKeyState('E') & 1)	// 스킵
	{
		if (logoTime < 1.5)
			logoTime = 1.5;
		else if (logoTime >= 1.5 && logoTime < 2)
			logoTime = 2;
		else if (logoTime >= 2 && logoTime < 3.5)
			logoTime = 3.5;
		else if (logoTime >= 3.5 && logoTime < 4)
			logoTime = 4;
	}
	else if (logoTime >= 4)
		change_state(new MenuState);
}

void LogoState::draw()
{
	if (logoTime < 2) {
		LONG alpha = 255 * (logoTime < 0.5 ? logoTime * 2 : logoTime < 1.5 ? 1 : (logoTime - 2) * -2);
		tuk.AlphaBlend(mDC, 0, 0, screen.right, screen.bottom, 0, 0, 1920, 1080, RGB(alpha, alpha, alpha));
	}
	else if (logoTime < 4) {
		LONG alpha = 255 * (logoTime < 2.5 ? (logoTime - 2) * 2 : logoTime < 3.5 ? 1 : (logoTime - 4) * -2);
		logo.AlphaBlend(mDC, 0, 0, screen.right, screen.bottom, 0, 0, 1920, 1080, RGB(alpha, alpha, alpha));
	}
}

// ---- MenuState ----

MenuState::MenuState() : string1(L"PLAY"), string2(L"OPTIONS"), string3(L"QUIT")
{
	title.Load(L"resources/main_title.png");
	font = MakeFont(60, L"Romulus");
	rect[0] = { 20,610,180,670 };
	rect[1] = { 20,690,220,750 };
	rect[2] = { 20,770,160,830 };
	for (int i = 0; i < 3; i++)
		isOn[i] = false;
	SoundManager::getInstance()->play(TitleState);
}

MenuState::~MenuState()
{
	DeleteObject(font);
	SoundManager::getInstance()->stop(TitleState);
}

void MenuState::update()
{
	SoundManager::getInstance()->update();
	for (int i = 0; i < 3; i++) {
		if (PtInRect(&rect[i], mPoint)) {
			if (!isOn[i])
				SoundManager::getInstance()->play(CURSORON);
			isOn[i] = true;
		}
		else isOn[i] = false;
	}
}

void MenuState::handle_events()
{
	if (GetAsyncKeyState(VK_LBUTTON) & 1) {
		if (isOn[0]) {
			SoundManager::getInstance()->play(BUTTONCLICK);
			change_state(new SelectState);
		}
		else if (isOn[1]) {
			SoundManager::getInstance()->play(BUTTONCLICK);
			push_state(new OptionState);
		}
		else if (isOn[2]) {
			SoundManager::getInstance()->play(BUTTONCLICK);
			PostQuitMessage(0);
			return;
		}
	}
}

void MenuState::draw()
{
	title.Draw(mDC, 0, 0, screen.right, screen.bottom, 0, 0, 1920, 1080);
	SelectObject(mDC, font);
	SetBkMode(mDC, TRANSPARENT);
	SetTextColor(mDC, isOn[0] ? RGB(255, 186, 0) : RGB(255, 255, 255));
	DrawText(mDC, string1, 5, &rect[0], DT_VCENTER | DT_SINGLELINE);
	SetTextColor(mDC, isOn[1] ? RGB(255, 186, 0) : RGB(255, 255, 255));
	DrawText(mDC, string2, 8, &rect[1], DT_VCENTER | DT_SINGLELINE);
	SetTextColor(mDC, isOn[2] ? RGB(255, 186, 0) : RGB(255, 255, 255));
	DrawText(mDC, string3, 9, &rect[2], DT_VCENTER | DT_SINGLELINE);
	cursor.Draw(mDC, mPoint.x - 20, mPoint.y - 30, 40, 40);
}

// ---- SelectState ----

SelectState::SelectState()
{
	const int imagesize = 2;
	Player::release();
	background.Load(L"resources/main_selchar_back.png");
	image1.Load(L"resources/marin_front.png");
	image2.Load(L"resources/knight_front.png");
	BT1 = RECT(screen.right / 4 - image1.GetWidth() * imagesize, screen.bottom / 2 - image1.GetHeight() * imagesize,
		screen.right / 4 + image1.GetWidth() * imagesize, screen.bottom / 2 + image1.GetHeight() * imagesize);
	BT2 = RECT(screen.right / 4 * 3 - image2.GetWidth() * imagesize, screen.bottom / 2 - image2.GetHeight() * imagesize,
		screen.right / 4 * 3 + image2.GetWidth() * imagesize, screen.bottom / 2 + image2.GetHeight() * imagesize);
	TB1 = RECT(BT1.left - 150, BT1.top - 40, BT1.right + 150, BT1.top);
	TB2 = RECT(BT2.left - 150, BT2.top - 40, BT2.right + 150, BT2.top);
	TB3 = RECT(BT1.left - 150, BT1.bottom, BT1.right + 150, BT1.bottom + 40);
	TB4 = RECT(BT2.left - 150, BT2.bottom, BT2.right + 150, BT2.bottom + 40);
	font = MakeFont(48, L"Romulus");
	mouseOn[0] = mouseOn[1] = false;
	SoundManager::getInstance()->play(SELECTSTATE);
}

SelectState::~SelectState()
{
	DeleteObject(font);
	SoundManager::getInstance()->stop(SELECTSTATE);
}

void SelectState::update()
{
	SoundManager::getInstance()->update();
}

void SelectState::handle_events()
{
	if (PtInRect(&BT1, mPoint)) {
		if (mouseOn[0] == false) {
			SoundManager::getInstance()->play(CURSORON);
			mouseOn[0] = true;
		}
	}
	else
		mouseOn[0] = false;
	if (PtInRect(&BT2, mPoint)) {
		if (mouseOn[1] == false) {
			SoundManager::getInstance()->play(CURSORON);
			mouseOn[1] = true;
		}
	}
	else
		mouseOn[1] = false;
	if (GetAsyncKeyState(VK_ESCAPE) & 1) {
		change_state(new MenuState());
		return;
	}
	else if (GetAsyncKeyState(VK_LBUTTON) & 1) {
		if (PtInRect(&BT1, mPoint)) {
			selectedPlayer = marin;
			SoundManager::getInstance()->play(BUTTONCLICK);
			change_state(new PlayState());
		}
		else if (PtInRect(&BT2, mPoint)) {
			selectedPlayer = knight;
			SoundManager::getInstance()->play(BUTTONCLICK);
			change_state(new PlayState());
		}
	}
}

void SelectState::draw()
{
	background.AlphaBlend(mDC, 0, 0, screen.right, screen.bottom, 0, 0, 1920, 1080, RGB(30, 30, 30));

	SelectObject(mDC, font);
	SetBkMode(mDC, TRANSPARENT);
	SetTextColor(mDC, RGB(255, 255, 255));
	DrawText(mDC, L"MARIN", 5, &TB1, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
	if (PtInRect(&BT1, mPoint)) {
		image1.AlphaBlend(mDC, BT1.left, BT1.top, BT1.right - BT1.left, BT1.bottom - BT1.top,
			0, 0, image1.GetWidth(), image1.GetHeight(), RGB(255, 255, 255));
		DrawText(mDC, L"GET MORE HP", 11, &TB3, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
	}
	else image1.AlphaBlend(mDC, BT1.left, BT1.top, BT1.right - BT1.left, BT1.bottom - BT1.top,
		0, 0, image1.GetWidth(), image1.GetHeight(), RGB(150, 150, 150));

	DrawText(mDC, L"KNIGHT", 6, &TB2, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
	if (PtInRect(&BT2, mPoint)) {
		image2.AlphaBlend(mDC, BT2.left, BT2.top, BT2.right - BT2.left, BT2.bottom - BT2.top,
			0, 0, image2.GetWidth(), image2.GetHeight(), RGB(255, 255, 255));
		DrawText(mDC, L"SWORD REFLETS BULLET", 20, &TB4, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
	}
	else image2.AlphaBlend(mDC, BT2.left, BT2.top, BT2.right - BT2.left, BT2.bottom - BT2.top,
		0, 0, image2.GetWidth(), image2.GetHeight(), RGB(150, 150, 150));
	cursor.Draw(mDC, mPoint.x - 20, mPoint.y - 30, 40, 40);
}

// ---- OptionState (ESC로 여는 일시정지 메뉴) ----

OptionState::OptionState() : string1(L"*menu"), string2(L"*control"), string3(L"*continue")
{
	black.Load(L"resources/black.png");
	background.Load(L"resources/OPTION.png");
	control.Load(L"resources/control.png");
	font = MakeFont(screen.right / 25, L"Romulus");
	rect[0] = { 320,570,520,620 };
	rect[1] = { 600,570,860,620 };
	rect[2] = { 920,570,1250,620 };
	for (int i = 0; i < 3; i++)
		isOn[i] = false;
	controlOn = false;
}

OptionState::~OptionState()
{
	DeleteObject(font);
}

void OptionState::update()
{
	SoundManager::getInstance()->update();
	for (int i = 0; i < 3; i++) {
		if (PtInRect(&rect[i], mPoint)) {
			if (!isOn[i])
				SoundManager::getInstance()->play(CURSORON);
			isOn[i] = true;
		}
		else isOn[i] = false;
	}
}

void OptionState::handle_events()
{
	if (GetAsyncKeyState(VK_ESCAPE) & 1) {
		if (controlOn)
			controlOn = false;
		else
			pop_state();
	}
	else if (GetAsyncKeyState(VK_LBUTTON) && !controlOn) {
		RECT temp = { 690, 378, 1110,420 };
		RECT temp2 = { (700 + 200 * volume) - 6, 368, (700 + 200 * volume) + 6, 420 };
		if (isOn[0]) {
			SoundManager::getInstance()->play(BUTTONCLICK);
			end_game();
			push_state(new MenuState);
		}
		else if (isOn[1]) {
			SoundManager::getInstance()->stop(CURSORON);
			if (!SoundManager::getInstance()->isPlaying(BUTTONCLICK)) {
				SoundManager::getInstance()->play(BUTTONCLICK);
			}
			controlOn = true;
		}
		else if (isOn[2]) {
			SoundManager::getInstance()->play(BUTTONCLICK);
			pop_state();
		}
		else if ((PtInRect(&temp, mPoint) || PtInRect(&temp2, mPoint)) && mPoint.x >= temp.left) {
			volume = (mPoint.x - temp.left) / 200.0;
			SoundManager::getInstance()->volumeSet(volume);
		}
	}
}

void OptionState::draw()
{
	(*(stateStack.end() - 2))->draw();	// 뒤에 깔린 화면 먼저
	black.AlphaBlend(mDC, 0, 0, screen.right, screen.bottom, 0, 0, black.GetWidth(), black.GetHeight(), 130);
	background.Draw(mDC, 200, 200, screen.right - 400, screen.bottom - 400, 0, 0, background.GetWidth(), background.GetHeight());
	SelectObject(mDC, font);
	SetBkMode(mDC, TRANSPARENT);
	SetTextColor(mDC, RGB(255, 255, 255));
	TextOut(mDC, 650, 340, L"-", 1);
	TextOut(mDC, 1120, 347, L"+", 1);
	SetTextColor(mDC, isOn[0] ? RGB(255, 186, 0) : RGB(255, 255, 255));
	DrawText(mDC, string1, 5, &rect[0], DT_CENTER | DT_VCENTER | DT_SINGLELINE);
	SetTextColor(mDC, isOn[1] ? RGB(255, 186, 0) : RGB(255, 255, 255));
	DrawText(mDC, string2, 8, &rect[1], DT_CENTER | DT_VCENTER | DT_SINGLELINE);
	SetTextColor(mDC, isOn[2] ? RGB(255, 186, 0) : RGB(255, 255, 255));
	DrawText(mDC, string3, 9, &rect[2], DT_CENTER | DT_VCENTER | DT_SINGLELINE);
	// 볼륨 바
	HBRUSH bar = CreateSolidBrush(RGB(125, 125, 125));
	HBRUSH brush = (HBRUSH)SelectObject(mDC, bar);
	Rectangle(mDC, 700, 380, 1100, 390);
	DeleteObject(bar);
	SelectObject(mDC, brush);
	Rectangle(mDC, 700, 378, 700 + 200 * volume, 392);
	Rectangle(mDC, (700 + 200 * volume) - 6, 368, (700 + 200 * volume) + 6, 402);
	if (controlOn)
		control.Draw(mDC, 200, 100, screen.right - 400, screen.bottom - 200, 0, 0, control.GetWidth(), control.GetHeight());
	cursor.Draw(mDC, mPoint.x - 20, mPoint.y - 30, 40, 40);
}

// ---- GameOverState ----

GameOverState::GameOverState()
{
	time = 0;
	dir = 0;
	alpha = 0;
	back.Load(L"resources/GameOver.png");
	black.Load(L"resources/black.png");
	rom = MakeFont(80, L"Romulus");
}

GameOverState::~GameOverState()
{
	DeleteObject(rom);
}

void GameOverState::update()
{
	Player::getInstance()->update();
	if (time <= 100) {	// 카메라 흔들기
		double range = (dir % 2 == 0) ? (100.0 / time) : -(100.0 / time);
		camPos.x = Player::getInstance()->GetPos().x + range;
		if (time % 10 == 0)
			dir++;
		time++;
	}
	else if (time >= 100) {
		if ((camPos - Player::getInstance()->GetPos()).GetLenth() > 3)
			camPos += (Player::getInstance()->GetPos() - camPos).Normalize() * 2;
		else
			time++;
	}
}

void GameOverState::handle_events()
{
	if (GetAsyncKeyState(VK_ESCAPE) & 1) {
		end_game();
		push_state(new MenuState);
	}
}

void GameOverState::draw()
{
	HDC mapDC = CreateCompatibleDC(mDC);
	HBITMAP mapbitmap = CreateCompatibleBitmap(mDC, 1920, 1080);
	SelectObject(mapDC, mapbitmap);
	MapManager::getInstance()->PrintMap(mapDC);
	if (time >= 100) {
		if (alpha < 255) alpha += 5;
		black.AlphaBlend(mapDC, 0, 0, 1920, 1080, 0, 0, black.GetWidth(), black.GetHeight(), alpha);
	}
	Player::getInstance()->draw_character(mapDC);
	StretchBlt(mDC, 0, 0, screen.right, screen.bottom,
		mapDC, camPos.x - camSize.x, camPos.y - camSize.y, camSize.x * 2, camSize.y * 2, SRCCOPY);
	if (alpha >= 255) {
		SelectObject(mDC, rom);
		SetBkMode(mDC, TRANSPARENT);
		SetTextColor(mDC, RGB(255, 255, 255));
		back.Draw(mDC, 470, 120);
		TextOut(mDC, 600, screen.bottom - 220, L"Press ESC", 9);
	}
	DeleteObject(mapbitmap);
	DeleteDC(mapDC);
}

// ---- EndingState ----

EndingState::EndingState()
{
	back.Load(L"resources/Ending.png");
	rom = MakeFont(80, L"Romulus");
	SoundManager::getInstance()->play(END);
}

EndingState::~EndingState()
{
	DeleteObject(rom);
	SoundManager::getInstance()->stop(END);
}

void EndingState::handle_events()
{
	if (GetAsyncKeyState(VK_ESCAPE) & 1) {
		end_game();
		push_state(new MenuState);
	}
}

void EndingState::draw()
{
	back.Draw(mDC, 0, 0, screen.right, screen.bottom);
	SelectObject(mDC, rom);
	SetBkMode(mDC, TRANSPARENT);
	SetTextColor(mDC, RGB(255, 255, 255));
	back.Draw(mDC, 470, 120);
	TextOut(mDC, 600, screen.bottom - 220, L"Press ESC", 9);
}

// ---- BossScene ----

BossScene::BossScene()
{
	scene.Load(L"resources/cutScene.png");
	time = 0;
	rect = { screen.right,100,screen.right * 2 + 20,800 };
	camPos = Player::getInstance()->GetPos();
	SoundManager::getInstance()->stop(MainState);
	SoundManager::getInstance()->play(BOSS);
}

BossScene::~BossScene()
{
	camPos = Player::getInstance()->GetPos();
}

void BossScene::update()
{
	if ((camPos - Boss::getInstance()->pos).GetLenth() > 3)
		camPos += (Boss::getInstance()->pos - camPos).Normalize() * 3;
	else {
		if (rect.left > 0)
			OffsetRect(&rect, -70, 0);
		else
			time++;
	}
	SoundManager::getInstance()->update();
}

void BossScene::handle_events()
{
	if (time == 70)
		pop_state();
	if (time == 1)
	{
		SoundManager::getInstance()->play(BOSSSCENE);
	}
}

void BossScene::draw()
{
	HDC mapDC = CreateCompatibleDC(mDC);
	HBITMAP mapbitmap = CreateCompatibleBitmap(mDC, 1920, 1080);
	SelectObject(mapDC, mapbitmap);
	MapManager::getInstance()->PrintMap(mapDC);
	Player::getInstance()->draw_character(mapDC);
	Boss::getInstance()->draw(mapDC);
	StretchBlt(mDC, 0, 0, screen.right, screen.bottom,
		mapDC, camPos.x - camSize.x, camPos.y - camSize.y, camSize.x * 2, camSize.y * 2, SRCCOPY);
	scene.Draw(mDC, rect);
	DeleteObject(mapbitmap);
	DeleteDC(mapDC);
}

// ---- BossDieScene ----

BossDieScene::BossDieScene() : boss(Boss::getInstance()), player(Player::getInstance()), time(0), dir(0)
{
	r1 = { 0,-200,screen.right,0 };
	r2 = { 0,screen.bottom,screen.right,screen.bottom + 200 };
	EffectManager::getInstance()->set_effect(new Effect(BOSSHIT, boss->pos));
	SoundManager::getInstance()->stop(BOSS);
	rom = MakeFont(50, L"dalmoori");
}

BossDieScene::~BossDieScene()
{
	camPos = player->GetPos();
	DeleteObject(rom);
}

void BossDieScene::update()
{
	if (time <= 100) {
		double range = (dir % 2 == 0) ? (150.0 / time) : -(150.0 / time);
		camPos.x = player->GetPos().x + range;
		if (time % 5 == 0)
			dir++;
		time++;
	}
	else if (time >= 100) {
		if ((camPos - boss->pos).GetLenth() > 4)
			camPos += (boss->pos - camPos).Normalize() * 4;
		else if ((camPos - boss->pos).GetLenth() <= 4 && (camPos - boss->pos).GetLenth() > 2)
			camPos += (boss->pos - camPos).Normalize();
	}
	if ((camPos - boss->pos).GetLenth() < 2) {
		if (r1.top < 0) {
			OffsetRect(&r1, 0, 20);
			OffsetRect(&r2, 0, -20);
		}
		boss->update();
		time++;
	}

	EffectManager::getInstance()->update();
	EffectManager::getInstance()->delete_effect();
	SoundManager::getInstance()->update();
}

void BossDieScene::handle_events()
{
	boss->handle_event();
	if (time == 420)
		pop_state();
}

void BossDieScene::draw()
{
	HDC mapDC = CreateCompatibleDC(mDC);
	HBITMAP mapbitmap = CreateCompatibleBitmap(mDC, 1920, 1080);
	SelectObject(mapDC, mapbitmap);
	SelectObject(mDC, rom);
	MapManager::getInstance()->PrintMap(mapDC);
	player->draw_character(mapDC);
	if (time <= 380)
		boss->draw(mapDC);
	EffectManager::getInstance()->Draw(mapDC);
	StretchBlt(mDC, 0, 0, screen.right, screen.bottom,
		mapDC, camPos.x - camSize.x, camPos.y - camSize.y, camSize.x * 2, camSize.y * 2, SRCCOPY);
	FillRect(mDC, &r1, (HBRUSH)GetStockObject(BLACK_BRUSH));
	FillRect(mDC, &r2, (HBRUSH)GetStockObject(BLACK_BRUSH));
	SetBkMode(mDC, TRANSPARENT);
	SetTextColor(mDC, RGB(255, 255, 255));
	if (time >= 150 && time <= 350) {
		TCHAR CT1[] = L"크윽..! 오늘은 이쯤에서 물러나주지...";
		TextOut(mDC, 350, screen.bottom - 130, CT1, 23);
	}
	DeleteObject(mapbitmap);
	DeleteDC(mapDC);
}
