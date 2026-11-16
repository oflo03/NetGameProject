#pragma once
#include <basetsd.h>	// UINT8·UINT16·UINT32 (Windows.h 없이 include해도 되게)

// 패킷 구조
// [size 2바이트][type 1바이트][내용]

// size는 헤더를 포함
// 네트워크 바이트 정렬로 변환하지 않는걸로 통일

constexpr UINT16 SERVER_PORT = 9000;
constexpr int MAX_PLAYERS = 3;
constexpr int NAME_LEN = 16; // NULL 포함
constexpr int MAX_PACKET_SIZE = 4096;
constexpr int MAX_ENEMIES = 64;
constexpr int MAX_SPAWN_BULLETS = 64;
constexpr int WEAPON_COUNT = 5;

enum PacketType : UINT8
{
	// Client → Server
	C2S_LOGIN = 1,
	C2S_SELECT_CHAR,
	C2S_READY,
	C2S_START,
	C2S_INPUT,
	C2S_INTERACT,

	// Server → Client
	S2C_LOGIN_RESULT = 100,
	S2C_ROOM_INFO,
	S2C_GAME_START,
	S2C_STAGE_LOAD,
	S2C_SNAPSHOT,
	S2C_BULLET_SPAWN,
	S2C_BULLET_REMOVE,
	S2C_ENEMY_SPAWN,
	S2C_ENEMY_DEAD,
	S2C_PLAYER_EVENT,
	S2C_DROP,
	S2C_EFFECT,
	S2C_BOSS,
	S2C_PORTAL,
	S2C_GAME_END,
	S2C_PLAYER_LEFT,
};

enum CharType : UINT8
{
	CHAR_MARIN, CHAR_KNIGHT
};

enum EnemyType : UINT8
{
	ENEMY_PISTOLMAN, ENEMY_RIFLEMAN, ENEMY_SHOTGUNMAN, ENEMY_BAT, ENEMY_BOMBSHE
};

enum WeaponType : UINT8
{
	WEAPON_SWORD, WEAPON_PISTOL, WEAPON_RIFLE, WEAPON_SHOTGUN, WEAPON_ROCKET,
	WEAPON_KEEP = 0xFF // C2S_INPUT에서 1~5키 안 누름
};

enum LoginResult : UINT8
{
	LOGIN_OK, LOGIN_NAME_TAKEN, LOGIN_FULL, LOGIN_IN_GAME
};

enum ReadyState : UINT8
{
	READY_NO, READY_YES
};

// C2S_INPUT의 keys 비트
enum InputKey : UINT8
{
	KEY_UP = 1 << 0,
	KEY_DOWN = 1 << 1,
	KEY_LEFT = 1 << 2,
	KEY_RIGHT = 1 << 3,
	KEY_ATTACK = 1 << 4,		// 좌클릭 누르는 동안
	KEY_ROLL = 1 << 5,			// 우클릭
	KEY_WEAPON_PREV = 1 << 6,	// Ctrl, 마우스 X1
	KEY_WEAPON_NEXT = 1 << 7,	// Tab, 마우스 X2
};

enum PlayerEvent : UINT8
{
	PE_HIT, PE_DEAD, PE_SWORD, PE_PICKUP, PE_REVIVE
};

enum BossPattern : UINT8
{
	BOSS_IDLE, BOSS_A, BOSS_B, BOSS_C, BOSS_D, BOSS_SUMMON, BOSS_DEAD
};

enum GameResult : UINT8
{
	RESULT_CLEAR, RESULT_WIPE
};

#pragma pack(push, 1)

struct PacketHeader
{
	UINT16 size;
	PacketType type;
};

// ---- Client → Server ----

// 저 이름 이거 쓸래요
struct C2S_LOGIN_PACKET : PacketHeader
{
	wchar_t name[NAME_LEN];
};

// 캐릭터 이거 할게요
struct C2S_SELECT_CHAR_PACKET : PacketHeader
{
	CharType charType;
};

// 준비 완료 했어요
struct C2S_READY_PACKET : PacketHeader
{
	ReadyState ready;
};

// 시작이나 하쇼
struct C2S_START_PACKET : PacketHeader {};

// 저 이거이거 눌렀어요
struct C2S_INPUT_PACKET : PacketHeader
{
	InputKey keys;
	WeaponType weapon;
	float aimX, aimY;
};

// 포탈 탔어요
struct C2S_INTERACT_PACKET : PacketHeader {};

// ---- Server → Client ----

// 로그인 결과 알려줄게요
struct S2C_LOGIN_RESULT_PACKET : PacketHeader
{
	LoginResult result;
	UINT8 playerId;
};

struct RoomMember
{
	UINT8 used;
	CharType charType;
	UINT8 ready;
	wchar_t name[NAME_LEN];
};

// 니 방에 이것들 있어요
struct S2C_ROOM_INFO_PACKET : PacketHeader
{
	UINT8 hostId; // 방장이 나가면 남은 사람 중 가장 작은 ID
	RoomMember members[MAX_PLAYERS];
};

struct StartPlayer
{
	UINT8 used;
	CharType charType;
	float x, y;
};

// 게임 시작할게요
struct S2C_GAME_START_PACKET : PacketHeader
{
	StartPlayer players[MAX_PLAYERS];
};

// 스테이지 로드하세요
struct S2C_STAGE_LOAD_PACKET : PacketHeader
{
	UINT8 stage;	// 1~3, 4는 보스 (컷신)
};

struct PlayerSnap
{
	UINT8 used;
	UINT8 state;
	float x, y;
	float angle;
	UINT16 hp;
	WeaponType weapon;
	UINT8 ammo[WEAPON_COUNT];
};

struct EnemySnap
{
	UINT16 id;
	UINT8 state;
	float x, y;
	float angle;
	UINT16 hp;
};

struct BossSnap
{
	UINT8 active;
	float x, y;
	float handX[2], handY[2];
	UINT8 handType[2];
	UINT16 hp;
};

// 월드 스냅샷
struct S2C_SNAPSHOT_PACKET : PacketHeader
{
	UINT32 tick;
	PlayerSnap players[MAX_PLAYERS];
	BossSnap boss;
	UINT8 enemyCount;
	EnemySnap enemies[MAX_ENEMIES];
};

struct BulletSpawn
{
	UINT16 id;
	UINT8 type;
	UINT8 side;
	float x, y;
	float dirX, dirY;
};

// 총알 생겼어요
struct S2C_BULLET_SPAWN_PACKET : PacketHeader
{
	UINT8 count;
	BulletSpawn bullets[MAX_SPAWN_BULLETS];
};

// 총알 사라졌어요
struct S2C_BULLET_REMOVE_PACKET : PacketHeader
{
	UINT16 id;
	UINT8 hit;
	float x, y;
};

struct S2C_ENEMY_SPAWN_PACKET : PacketHeader
{
	UINT16 id;
	EnemyType type;
	float x, y;
};

struct S2C_ENEMY_DEAD_PACKET : PacketHeader
{
	UINT16 id;
};

struct S2C_PLAYER_EVENT_PACKET : PacketHeader
{
	UINT8 playerId;
	PlayerEvent event;
	UINT8 value;
};

struct S2C_DROP_PACKET : PacketHeader
{
	UINT16 id;
	UINT8 add;
	UINT8 type;
	float x, y;
};

struct S2C_EFFECT_PACKET : PacketHeader
{
	UINT8 type;
	float x, y;
};

struct S2C_BOSS_PACKET : PacketHeader
{
	BossPattern pattern;
};

struct S2C_PORTAL_PACKET : PacketHeader
{
	float x, y;
};

struct S2C_GAME_END_PACKET : PacketHeader
{
	GameResult result;
};

struct S2C_PLAYER_LEFT_PACKET : PacketHeader
{
	UINT8 playerId;
};

#pragma pack(pop)

static_assert(sizeof(S2C_SNAPSHOT_PACKET) <= MAX_PACKET_SIZE);
static_assert(sizeof(S2C_BULLET_SPAWN_PACKET) <= MAX_PACKET_SIZE);
