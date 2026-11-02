#pragma once
#include <cstdint>

// ---- 서버·클라이언트 공용 패킷 정의 ----
// 모든 패킷은 [size 2바이트][type 1바이트][내용]. size는 헤더를 포함한 전체 길이.
// 게임 수치의 종류 값(무기, 총알, 적, 이펙트 등)은 클라이언트 원작 enum 값을 그대로 쓴다.

constexpr uint16_t SERVER_PORT = 9000;
constexpr int MAX_PLAYERS = 3;
constexpr int NAME_LEN = 16;			// 닉네임 글자 수 (널 포함)
constexpr int MAX_PACKET_SIZE = 4096;	// size가 이보다 크면 잘못된 패킷
constexpr int MAX_ENEMIES = 64;			// 스냅샷 하나에 담는 적 수
constexpr int MAX_SPAWN_BULLETS = 64;	// 총알 생성 패킷 하나에 담는 총알 수 (보스 원형 탄막 40발)
constexpr int WEAPON_COUNT = 5;			// SWORD, PISTOL, RIFLE, SHOTGUN, ROCKET

enum PacketType : uint8_t
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

enum LoginResult : uint8_t
{
	LOGIN_OK, LOGIN_NAME_TAKEN, LOGIN_FULL, LOGIN_IN_GAME
};

// C2S_INPUT의 keys 비트
enum InputKey : uint8_t
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

enum PlayerEvent : uint8_t
{
	PE_HIT, PE_DEAD, PE_SWORD, PE_PICKUP, PE_REVIVE
};

enum BossPattern : uint8_t
{
	BOSS_IDLE, BOSS_A, BOSS_B, BOSS_C, BOSS_D, BOSS_SUMMON, BOSS_DEAD
};

enum GameResult : uint8_t
{
	RESULT_CLEAR, RESULT_WIPE
};

#pragma pack(push, 1)

struct PacketHeader
{
	uint16_t size;
	PacketType type;
};

// ---- Client → Server ----

struct C2S_LOGIN_PACKET : PacketHeader
{
	wchar_t name[NAME_LEN];
};

struct C2S_SELECT_CHAR_PACKET : PacketHeader
{
	uint8_t charType;	// playertype (marin 0, knight 1)
};

struct C2S_READY_PACKET : PacketHeader
{
	uint8_t ready;
};

struct C2S_START_PACKET : PacketHeader {};

// 33ms마다 전송. 구르기·무기 교체 비트는 지난 전송 이후 한 번이라도 눌렸으면 1
struct C2S_INPUT_PACKET : PacketHeader
{
	uint8_t keys;		// InputKey 비트
	int8_t weapon;		// 1~5키로 고른 무기 ID, 안 눌렀으면 -1
	float aimX, aimY;	// 마우스 조준 좌표 (월드)
};

struct C2S_INTERACT_PACKET : PacketHeader {};	// E키 (포탈)

// ---- Server → Client ----

struct S2C_LOGIN_RESULT_PACKET : PacketHeader
{
	uint8_t result;		// LoginResult
	uint8_t playerId;	// 0 ~ MAX_PLAYERS-1, 실패면 무시
};

struct RoomMember
{
	uint8_t used;
	uint8_t charType;
	uint8_t ready;
	wchar_t name[NAME_LEN];
};

struct S2C_ROOM_INFO_PACKET : PacketHeader
{
	uint8_t hostId;
	RoomMember members[MAX_PLAYERS];	// 인덱스 = playerId
};

struct StartPlayer
{
	uint8_t used;
	uint8_t charType;
	float x, y;
};

struct S2C_GAME_START_PACKET : PacketHeader
{
	StartPlayer players[MAX_PLAYERS];	// 인덱스 = playerId
};

struct S2C_STAGE_LOAD_PACKET : PacketHeader
{
	uint8_t stage;	// 1~3, 4는 보스 (컷신)
};

struct PlayerSnap
{
	uint8_t used;
	uint8_t state;		// STATE
	float x, y;
	float angle;		// 조준 각도
	int16_t hp;
	uint8_t weapon;		// 무기 ID
	uint8_t ammo[WEAPON_COUNT];
};

struct EnemySnap
{
	uint16_t id;
	uint8_t state;
	float x, y;
	float angle;
	int16_t hp;
};

struct BossSnap
{
	uint8_t active;
	float x, y;
	float handX[2], handY[2];
	uint8_t handType[2];
	int16_t hp;
};

// enemies는 enemyCount개만 보낸다: size = offsetof(S2C_SNAPSHOT_PACKET, enemies) + enemyCount * sizeof(EnemySnap)
struct S2C_SNAPSHOT_PACKET : PacketHeader
{
	uint32_t tick;
	PlayerSnap players[MAX_PLAYERS];	// 인덱스 = playerId
	BossSnap boss;
	uint8_t enemyCount;
	EnemySnap enemies[MAX_ENEMIES];
};

struct BulletSpawn
{
	uint16_t id;		// 서버가 발급, 한 바퀴 돌면 재사용
	uint8_t type;		// bullet_table
	uint8_t side;		// 플레이어 총알 / 적 총알
	float x, y;
	float dirX, dirY;	// 이동은 클라이언트가 같은 공식으로 재현
};

// bullets는 count개만 보낸다 (스냅샷과 같은 방식)
struct S2C_BULLET_SPAWN_PACKET : PacketHeader
{
	uint8_t count;
	BulletSpawn bullets[MAX_SPAWN_BULLETS];
};

struct S2C_BULLET_REMOVE_PACKET : PacketHeader
{
	uint16_t id;
	uint8_t hit;	// 1이면 소멸 위치에 피격 이펙트
	float x, y;
};

struct S2C_ENEMY_SPAWN_PACKET : PacketHeader
{
	uint16_t id;
	uint8_t type;	// enemytype
	float x, y;
};

struct S2C_ENEMY_DEAD_PACKET : PacketHeader
{
	uint16_t id;
};

struct S2C_PLAYER_EVENT_PACKET : PacketHeader
{
	uint8_t playerId;
	uint8_t event;	// PlayerEvent
	uint8_t value;	// PE_PICKUP: 얻은 아이템 (DROP)
};

struct S2C_DROP_PACKET : PacketHeader
{
	uint16_t id;
	uint8_t add;	// 1 생성, 0 제거
	uint8_t type;	// DROP
	float x, y;
};

struct S2C_EFFECT_PACKET : PacketHeader
{
	uint8_t type;	// CEffect
	float x, y;
};

struct S2C_BOSS_PACKET : PacketHeader
{
	uint8_t pattern;	// BossPattern
};

struct S2C_PORTAL_PACKET : PacketHeader
{
	float x, y;
};

struct S2C_GAME_END_PACKET : PacketHeader
{
	uint8_t result;	// GameResult
};

struct S2C_PLAYER_LEFT_PACKET : PacketHeader
{
	uint8_t playerId;
};

#pragma pack(pop)

static_assert(sizeof(S2C_SNAPSHOT_PACKET) <= MAX_PACKET_SIZE);
static_assert(sizeof(S2C_BULLET_SPAWN_PACKET) <= MAX_PACKET_SIZE);
