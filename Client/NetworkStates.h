#pragma once
#include "Common.h"
#include "Framework.h"
#include "Protocol.h"
#include <string>

// 네트워크 화면 틀. 기존 게임 흐름에는 아직 연결하지 않음

class ConnectState : public GameState
{
private:
	std::string serverIp;
	std::wstring message;
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
	wchar_t nickname[NAME_LEN] = {};
	std::wstring message;
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
	S2C_ROOM_INFO_PACKET roomInfo = {};
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
