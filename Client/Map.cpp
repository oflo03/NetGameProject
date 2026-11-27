#include "Map.h"
#include <string>
#include "Framework.h"
#include "Scenes.h"
#include "Player.h"
#include "Enemy.h"
#include "Boss.h"

// ---- Tile / MapManager ----

std::ifstream& operator>>(std::ifstream& is, Tile& tile)
{
	if (!(is >> tile.type >> tile.tilePos))
		return is;
	switch (tile.type)
	{
	case wall_t:
		tile.col = new Collider(Vector2D<float>(TileSize, TileSize));
		tile.col->owner = &tile;
		tile.col->layer = wall;
		COLL.emplace_back(tile.col);
		break;
	case poke_water:
		tile.col = new Collider(Vector2D<float>(TileSize, TileSize));
		tile.col->owner = &tile;
		tile.col->layer = water;
		COLL.emplace_back(tile.col);
		break;
	default:
		break;
	}
	return is;
}

MapManager* MapManager::instance = nullptr;

MapManager::MapManager()
{
	currentStage = 1;
	grassImage.Load(L"resources/Tiles_Grass.png");
	wallImage.Load(L"resources/Tiles_Wall.png");
	Poke_grassImage.Load(L"resources/Tiles_PokemonGrass.png");
	Poke_waterImage.Load(L"resources/Tiles_PokemonWater.png");
}

MapManager* MapManager::getInstance()
{
	if (instance == nullptr)
		instance = new MapManager;
	return instance;
}

bool MapManager::LoadTileMap(int stage, bool createColliders)
{
	if (stage < 1 || stage > 4)
		return false;
	std::ifstream mapFile("resources/map" + std::to_string(stage) + ".txt");
	if (!mapFile)
		return false;
	if (!createColliders) {
		// 맵을 끝까지 읽은 뒤 현재 타일을 교체한다.
		struct TileData { int type; int index; };
		TileData tiles[MapHeight][MapWidth];
		for (auto& row : tiles)
			for (auto& tile : row)
				if (!(mapFile >> tile.type >> tile.index))
					return false;
		for (int i = 0; i < MapHeight; ++i)
			for (int j = 0; j < MapWidth; ++j) {
				Maps[i][j].type = tiles[i][j].type;
				Maps[i][j].tilePos = tiles[i][j].index;
			}
		currentStage = stage;
		return true;
	}
	for (int i = 0; i < MapHeight; ++i)
		for (int j = 0; j < MapWidth; ++j)
		{
			auto& tile = Maps[i][j];
			if (!(mapFile >> tile))
				return false;
			if (tile.type == wall_t || tile.type == poke_water)
				tile.col->pos = Vector2D<float>(j * PTS + TileSize, i * PTS + TileSize);
		}
	currentStage = stage;
	return true;
}

void MapManager::PrintMap(HDC mDC)
{
	for (int i = 0; i < MapHeight; ++i)
		for (int j = 0; j < MapWidth; ++j)
		{
			int tilePos = Maps[i][j].tilePos;
			switch (Maps[i][j].type)
			{
			case grass:
				grassImage.TransparentBlt(mDC, j * PTS, i * PTS, PTS, PTS,
					tilePos % GrassTileNum * TileSize, tilePos / GrassTileNum * TileSize, TileSize, TileSize, RGB(255, 255, 255));
				break;
			case wall_t:
				wallImage.TransparentBlt(mDC, j * PTS, i * PTS, PTS, PTS,
					tilePos % WallTileNum * TileSize, tilePos / WallTileNum * TileSize, TileSize, TileSize, RGB(255, 255, 255));
				break;
			case poke_grass:
				Poke_grassImage.TransparentBlt(mDC, j * PTS, i * PTS, PTS, PTS,
					tilePos % PokemonTileNum * PokemonTileSize, tilePos / PokemonTileNum * PokemonTileSize, PokemonTileSize, PokemonTileSize, RGB(255, 255, 255));
				break;
			case poke_water:
				Poke_waterImage.TransparentBlt(mDC, j * PTS, i * PTS, PTS, PTS,
					tilePos % PokemonTileNum * PokemonTileSize, tilePos / PokemonTileNum * PokemonTileSize, PokemonTileSize, PokemonTileSize, RGB(255, 255, 255));
				break;
			default:
				break;
			}
		}
}

// ---- Portal ----

Portal* Portal::instance = nullptr;

Portal::Portal(float x, float y) :pos(x, y), pframe(0), eframe(0)
{
	LoadAnimation(portal, L"resources/Portal.png", 4, 1);
	LoadAnimation(E, L"resources/Portal_E-Key.png", 2, 1);
	LoadAnimation(P, L"resources/Portal_P-Key.png", 2, 1);
}

Portal* Portal::getInstance()
{
	if (instance == nullptr)
		instance = new Portal(0, 0);
	return instance;
}

void Portal::release()
{
	if (instance != nullptr)
		delete instance;
	instance = nullptr;
}

void Portal::update()
{
	pframe = pframe + frame_time * portal.velocity * portal.frame;
	if (pframe >= portal.frame)pframe = 0;
	eframe = eframe + frame_time * E.velocity * E.frame;
	if (eframe >= E.frame)eframe = 0;
}

void Portal::handle_event()
{
	if ((GetAsyncKeyState('E') & 0x8000) && (Player::getInstance()->GetPos() - pos).GetLenth() < UseRange)
		LevelManager::getInstance()->loadNextStage();
}

void Portal::Draw(HDC mDC)
{
	const bool showHint = (camPos - pos).GetLenth() < UseRange;
	const auto hintPosition = showHint ? Player::getInstance()->GetPos() : pos;
	Draw(mDC, showHint ? &hintPosition : nullptr);
}

void Portal::Draw(HDC mDC, const Vector2D<float>* hintPosition)
{
	portal.resource.Draw(mDC, pos.x - portal.size.right, pos.y - portal.size.bottom - 50, portal.size.right * 2, portal.size.bottom * 2,
		(int)pframe * portal.size.right, 0, portal.size.right, portal.size.bottom);
	// 화면 밖이면 가장자리에 방향 표시
	if (!(pos - camPos < camSize))
		P.resource.Draw(mDC, camPos.x + (pos - camPos).Normalize().x * camSize.x / 1.1 - P.size.right / 2,
			camPos.y + (pos - camPos).Normalize().y * camSize.y / 1.1 - P.size.bottom / 2, P.size.right, P.size.bottom,
			(int)eframe * P.size.right, 0, P.size.right, P.size.bottom);
	if (hintPosition)
		E.resource.Draw(mDC, hintPosition->x - E.size.right / 2, hintPosition->y - E.size.bottom - 30, E.size.right, E.size.bottom,
			(int)eframe * E.size.right, 0, E.size.right, E.size.bottom);
}

// ---- LevelManager ----

Vector2D<float> portalpoint[4]{
	Vector2D<float>(1633,256),
	Vector2D<float>(960,900),
	Vector2D<float>(960,540),
	Vector2D<float>(960,540)
};

LevelManager* LevelManager::instance = nullptr;

LevelManager::LevelManager() :stage(1), phase(1)
{
	MapManager::getInstance()->LoadTileMap(1);
	EnemyManager::getInstance()->SetEnemy(1);
}

LevelManager* LevelManager::getInstance()
{
	if (instance == nullptr)
		instance = new LevelManager();
	return instance;
}

void LevelManager::release()
{
	if (instance != nullptr)
		delete instance;
	instance = nullptr;
}

void LevelManager::update()
{
	if (EnemyManager::getInstance()->isclear()) {
		if (phase == 3) {
			Portal::getInstance()->SetPos(portalpoint[stage - 1]);
			enemyclear = true;
		}
		else if (stage != 4)
		{
			++phase;
			EnemyManager::getInstance()->SetEnemy((stage - 1) * 3 + phase);
		}
		else if (Boss::getInstance()->getHP() <= 0) {
			Portal::getInstance()->SetPos(portalpoint[stage - 1]);
			enemyclear = true;
		}
	}
	else enemyclear = false;
}

void LevelManager::loadNextStage()
{
	++stage;
	phase = 1;
	Collider::Clear();
	DropItem::Clear();
	EnemyManager::getInstance()->Clear();
	Bullets.clear();
	if (stage == 5) {
		change_state(new EndingState);
		return;
	}
	Player::getInstance()->SetPos(spawnpoint[stage - 1]);
	if (stage == 4)
		push_state(new BossScene);
	COLL.emplace_back(Player::getInstance()->col);
	MapManager::getInstance()->LoadTileMap(stage);
	EnemyManager::getInstance()->SetEnemy((stage - 1) * 3 + phase);
	enemyclear = false;
}
