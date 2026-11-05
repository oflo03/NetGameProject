#pragma once
#include <WinSock2.h>
#include <atomic>
#include <mutex>
#include <vector>
#include "Protocol.h"

constexpr int MAX_SESSIONS = 8;	// 로그인 전 접속 포함. 로그인은 MAX_PLAYERS명까지

// 락 순서: room.lock → sendLock / inputLock (반대로 잡지 않는다)

// 게임 로직 스레드가 TakeInput으로 꺼내 가는 입력
struct InputState
{
	uint8_t keys = 0;			// InputKey 비트
	int8_t weapon = -1;			// 1~5키로 고른 무기 ID, 없으면 -1
	float aimX = 0, aimY = 0;	// 조준 좌표 (월드)
	bool interact = false;		// C2S_INTERACT를 받았으면 true
};

// 접속 하나. 소켓과 수신 버퍼는 자기 클라이언트 스레드만 쓰고,
// 송신 큐와 입력 버퍼는 각자의 lock, 나머지는 room.lock을 잡고 쓴다
struct Session
{
	bool used = false;					// 슬롯 사용 중
	std::atomic<bool> connected = false;	// 클라이언트 스레드 루프 조건
	SOCKET sock = INVALID_SOCKET;
	int playerId = -1;					// 로그인 전 -1, 로그인하면 0 ~ MAX_PLAYERS-1
	wchar_t name[NAME_LEN] = {};
	uint8_t charType = 0;				// playertype (marin 0, knight 1)
	bool ready = false;
	char recvBuffer[MAX_PACKET_SIZE * 4] = {};	// 덜 받은 패킷 + 새로 받은 데이터
	int recvLength = 0;

	// 송신 큐. 여러 스레드가 EnqueueSend로 넣고 자기 클라이언트 스레드가 FlushSend로 보낸다
	std::mutex sendLock;
	std::vector<char> sendQueue;

	// 입력 버퍼. 클라이언트 스레드가 HandleInput으로 쓰고 게임 로직 스레드가 TakeInput으로 꺼낸다
	std::mutex inputLock;
	InputState input;
};

enum RoomState : uint8_t
{
	ROOM_LOBBY, ROOM_IN_GAME
};

// 세션·대기실 정보. 모든 스레드가 lock을 잡고 쓴다
struct Room
{
	std::mutex lock;
	Session sessions[MAX_SESSIONS];
	RoomState state = ROOM_LOBBY;
	int hostId = -1;					// 방장 playerId
	bool startRequested = false;		// S4가 표시하고 S9가 확인
	std::vector<uint8_t> leftPlayers;	// 게임 중 이탈한 playerId. S8이 넣고 게임 로직 스레드가 꺼냄
};

extern Room room;
