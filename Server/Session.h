#pragma once
#include <WinSock2.h>
#include <atomic>
#include <mutex>
#include <vector>
#include "Protocol.h"

constexpr int MAX_SESSIONS = 8;	// 로그인 전 접속 포함. 로그인은 MAX_PLAYERS명까지

// 접속 하나. 소켓과 수신 버퍼는 자기 클라이언트 스레드만 쓰고, 나머지는 room.lock을 잡고 쓴다
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
