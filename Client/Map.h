#pragma once
#include <fstream>
#include "Collision.h"

// ---- 타일맵 ----

#define TileSize 32
#define PokemonTileSize 16
#define PTS (TileSize * 2)	// 화면에 그리는 타일 크기
#define GrassTileNum 8
#define WallTileNum 16
#define PokemonTileNum 6
#define MapWidth 30
#define MapHeight 17

enum tileType
{
	none, grass, wall_t, poke_grass, poke_water
};

class Tile : public Master
{
private:
	friend std::ifstream& operator>>(std::ifstream& is, Tile& tile);
public:
	int type;
	int tilePos;
	virtual void handle_collision(int otherLayer, int damage) {};
};

class MapManager
{
private:
	static MapManager* instance;
	CImage grassImage;
	CImage wallImage;
	CImage Poke_grassImage;
	CImage Poke_waterImage;
	Tile Maps[MapHeight][MapWidth];
	int currentStage;
	MapManager();
public:
	static MapManager* getInstance();
	void LoadTileMap(int stage);	// resources/map{stage}.txt
	void PrintMap(HDC mDC);
};

// ---- 포탈 (스테이지 클리어 후 E키로 다음 스테이지) ----

class Portal
{
protected:
	static Portal* instance;
	Animation portal, E, P;
	float pframe, eframe;
	Vector2D<float> pos;
	Portal(float x, float y);
public:
	Vector2D<float> GetPos() { return pos; };
	static Portal* getInstance();
	static void release();
	void SetPos(Vector2D<float> point) { pos = point; }
	void update();
	void handle_event();
	void Draw(HDC mDC);
};

// ---- 스테이지 진행: stage 1~4 (4 = 보스), 각 stage는 phase 1~3 웨이브 ----

class LevelManager
{
protected:
	int stage, phase;
	static LevelManager* instance;
public:
	LevelManager();
	static LevelManager* getInstance();
	static void release();
	void update();
	void loadNextStage();
	int GetStage() { return stage; }
};
