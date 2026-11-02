#pragma once
#include <Windows.h>
#include <atlImage.h>
#include <vector>
#include <random>
#include "Vector2D.h"

struct Animation
{
	CImage resource;
	RECT size;
	int frame;
	float velocity;
};

// 가로로 frame장 이어붙인 스프라이트 시트를 읽고 한 장 크기를 계산
inline void LoadAnimation(Animation& a, const wchar_t* path, int frame, float velocity)
{
	a.resource.Load(path);
	a.frame = frame;
	a.velocity = velocity;
	a.size = { 0, 0, a.resource.GetWidth() / frame, a.resource.GetHeight() };
}

enum STATE
{
	STATE_IDLE, STATE_RUN, STATE_DAMAGED, STATE_DEAD, STATE_ROLL
};

enum DIRECTION
{
	FRONT, FRONT_RIGHT, FRONT_LEFT, BACK, BACK_RIGHT, BACK_LEFT
};

// ---- 전역 변수 (정의는 main.cpp) ----

// 윈도우 / 프레임
extern double frame_time;
extern double volume;
extern HDC mDC;
extern RECT screen;
extern Vector2D<float> monitorSize;
extern Vector2D<float> camSize;
extern Vector2D<float> camPos;
extern CImage cursor;
extern POINT mPoint;

// 랜덤
extern std::random_device rd;
extern std::default_random_engine dre;
extern std::uniform_int_distribution<int> uid;
extern std::uniform_int_distribution<int> rad;
extern std::uniform_int_distribution<int> ranTime;
extern std::uniform_int_distribution<int> ran;
extern std::uniform_int_distribution<int> randrop;

// 게임 월드
struct Collider;
class Bullet;
class DropItem;
extern std::vector<Collider*> COLL;
extern std::vector<Bullet*> Bullets;
extern std::vector<DropItem*> drops;
extern bool lookRange;	// X키: 충돌 범위 표시
extern bool beatable;	// Z키: 무적 토글
extern bool enemyclear;
