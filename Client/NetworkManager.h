#pragma once
#include <WinSock2.h>
#include <vector>
#include "Protocol.h"

class NetworkManager
{
public:
	enum class ConnectionState
	{
		CS_DISCONNECTED, CS_CONNECTING, CS_CONNECTED, CS_FAILED
	};

private:
	static NetworkManager* instance;
	static constexpr int RECV_BUFFER_SIZE = MAX_PACKET_SIZE * 16;	// 여러 패킷 누적 수신 (65536바이트)
	SOCKET sock = INVALID_SOCKET;
	char recvBuffer[RECV_BUFFER_SIZE] = {};
	int recvLength = 0;
	std::vector<char> sendBuffer;
	ConnectionState connectionState = ConnectionState::CS_DISCONNECTED;
	ULONGLONG connectStartTime = 0;
	NetworkManager();

public:
	static NetworkManager* getInstance();
	static void release();
	bool Init();
	bool Connect(const char* serverIp);
	ConnectionState CheckConnect();
	void RecvAll();
	void ProcessPackets();
	void Send(const void* packet, int size);
	void FlushSend();
	void Disconnect();
	void OnDisconnected();
	void Cleanup();
};
