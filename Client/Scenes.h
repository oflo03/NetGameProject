#pragma once
#include "Common.h"
#include "Framework.h"

// LocalPlayState 외의 화면들 (로고, 메뉴, 캐릭터 선택, 옵션, 게임오버, 엔딩, 보스 컷신)
// 흐름: Logo -> Menu -> Select -> LocalPlayState -> (BossScene / BossDieScene) -> Ending / GameOver

class Boss;
class Player;

class LogoState : public GameState
{
private:
	CImage tuk, logo;
	double logoTime;
public:
	LogoState();
	virtual void update();
	virtual void handle_events();
	virtual void draw();
};

class MenuState : public GameState
{
private:
	CImage title;
	HFONT font;
	TCHAR string1[10];
	TCHAR string2[10];
	TCHAR string3[10];
	RECT rect[3];
	bool isOn[3];
public:
	MenuState();
	~MenuState();
	virtual void update();
	virtual void handle_events();
	virtual void draw();
};

class SelectState : public GameState
{
private:
	CImage background, image1, image2;
	HFONT font;
	RECT BT1, BT2;				// 캐릭터 버튼
	RECT TB1, TB2, TB3, TB4;	// 이름 / 설명 텍스트 박스
	bool mouseOn[2];
public:
	SelectState();
	~SelectState();
	virtual void update();
	virtual void handle_events();
	virtual void draw();
};

class OptionState : public GameState
{
private:
	CImage background;
	CImage black;
	CImage control;
	HFONT font;
	TCHAR string1[10];
	TCHAR string2[10];
	TCHAR string3[10];
	RECT rect[3];
	bool isOn[3];
	bool controlOn;
public:
	OptionState();
	~OptionState();
	virtual void update();
	virtual void handle_events();
	virtual void draw();
};

class GameOverState : public GameState
{
protected:
	CImage back;
	CImage black;
	HFONT rom;
	int time;
	int dir;
	LONG alpha;
public:
	GameOverState();
	~GameOverState();
	void update();
	void handle_events();
	void draw();
};

class EndingState : public GameState
{
protected:
	CImage back;
	HFONT rom;
public:
	EndingState();
	~EndingState();
	void update() {}
	void handle_events();
	void draw();
};

// 보스 등장 컷신
class BossScene : public GameState
{
protected:
	CImage scene;
	float time;
	RECT rect;
public:
	BossScene();
	~BossScene();
	virtual void update();
	virtual void handle_events();
	virtual void draw();
};

// 보스 처치 컷신
class BossDieScene : public GameState
{
private:
	Boss* boss;
	Player* player;
	int time;
	int dir;
	RECT r1;
	RECT r2;
	HFONT rom;
public:
	BossDieScene();
	~BossDieScene();
	virtual void update();
	virtual void handle_events();
	virtual void draw();
};
