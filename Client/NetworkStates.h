#pragma once
#include "Common.h"
#include "Framework.h"
#include "Protocol.h"
#include <string>

// 네트워크 화면: 메뉴 ONLINE → ConnectState → LoginState → LobbyState → PlayState

class ConnectState : public GameState
{
private:
	CImage background;
	std::string serverIp;
	std::wstring message;
	bool connecting = false;
public:
	ConnectState();
	void on_char(wchar_t ch) override;
	void update() override;
	void handle_events() override;
	void draw() override;
};

class LoginState : public GameState
{
private:
	CImage background;
	wchar_t nickname[NAME_LEN] = {};
	std::wstring message;
	bool waiting = false;	// 로그인 결과를 기다리는 중
public:
	LoginState();
	void on_char(wchar_t ch) override;
	void SendLogin();
	void process_packet(const PacketHeader* packet) override;
	void update() override;
	void handle_events() override;
	void draw() override;
};

class LobbyState : public GameState
{
private:
	CImage background;
	CImage charImage[2];	// CHAR_MARIN, CHAR_KNIGHT 순서
	S2C_ROOM_INFO_PACKET roomInfo = {};
	bool hasRoomInfo = false;
	RECT memberRect[MAX_PLAYERS];
	RECT charButton[2];	// MARIN, KNIGHT
	RECT readyButton, startButton;

	const RoomMember* Me() const;	// 대기실 정보의 내 칸 (아직 못 받았으면 nullptr)
	bool IsHost() const;
public:
	LobbyState();
	void SendSelectChar(uint8_t charType);
	void SendReady(bool ready);
	void SendStart();
	void process_packet(const PacketHeader* packet) override;
	void update() override;
	void handle_events() override;
	void draw() override;
};

class ResultState : public GameState
{
private:
	GameResult result = RESULT_WIPE;
public:
	ResultState();
	void update() override;
	void handle_events() override;
	void draw() override;
};
