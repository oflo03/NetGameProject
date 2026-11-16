#pragma once
#include <WinSock2.h>
#include <atomic>
#include <mutex>
#include <vector>
#include "Protocol.h"

constexpr int MAX_SESSIONS = 8; // 로그인 전 접속 포함

// 락 순서: room.lock → sendLock / inputLock (반대로 잡지 않는다)

// 게임 로직 스레드가 TakeInput으로 꺼내 갈 입력
struct InputState
{
	InputKey keys{};
	WeaponType weapon = WEAPON_KEEP;
	float aimX = 0, aimY = 0;
	bool interact = false;
};

// 접속 하나
// 소켓·수신 버퍼는 자기 클라이언트 스레드만, 송신 큐·입력 버퍼는 각자의 lock, 나머지는 room.lock
struct Session
{
	bool used = false;
	std::atomic<bool> connected = false;
	SOCKET sock = INVALID_SOCKET;
	int playerId = -1; // 로그인 전 -1
	wchar_t name[NAME_LEN] = {};
	CharType charType = CHAR_MARIN;
	ReadyState ready = READY_NO;
	char recvBuffer[MAX_PACKET_SIZE * 4] = {};
	int recvLength = 0;

	// 아무 스레드나 EnqueueSend로 넣고 자기 클라이언트 스레드가 FlushSend로 보낸다
	std::mutex sendLock;
	std::vector<char> sendQueue;

	// 클라이언트 스레드가 HandleInput으로 넣고 게임 로직 스레드가 TakeInput으로 꺼낸다
	std::mutex inputLock;
	InputState input;
};

enum RoomState : UINT8
{
	ROOM_LOBBY, ROOM_IN_GAME
};

// 세션·대기실 정보
struct Room
{
	std::mutex lock;
	Session sessions[MAX_SESSIONS];
	RoomState state = ROOM_LOBBY;
	int hostId = -1;
	bool startRequested = false; // S4가 켜고 S9가 확인
	std::vector<UINT8> leftPlayers; // 게임 중 나간 playerId (S8이 넣고 게임 로직 스레드가 꺼냄)
};

extern Room room;
