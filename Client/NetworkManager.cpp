#include "NetworkManager.h"
#include <WS2tcpip.h>
#include "Framework.h"
#include <algorithm>
#include <climits>
#include <cstring>

NetworkManager* NetworkManager::instance = nullptr;

NetworkManager::NetworkManager()
{
}

NetworkManager* NetworkManager::getInstance()
{
	if (instance == nullptr)
		instance = new NetworkManager;
	return instance;
}

void NetworkManager::release()
{
	delete instance;
	instance = nullptr;
}

bool NetworkManager::Init()
{
	// [C1] Init: Winsock 초기화
	WSADATA wsa;
	return WSAStartup(MAKEWORD(2, 2), &wsa) == 0;
}

bool NetworkManager::Connect(const char* serverIp)
{
	// [C1] Connect: 넌블로킹 소켓 생성, connect 시작
	sockaddr_in addr{};
	addr.sin_family = AF_INET;
	addr.sin_port = htons(SERVER_PORT);
	if (inet_pton(AF_INET, serverIp, &addr.sin_addr) != 1)
		return false;

	if (sock != INVALID_SOCKET)
		closesocket(sock);
	sock = socket(AF_INET, SOCK_STREAM, 0);
	if (sock == INVALID_SOCKET)
		return false;
	u_long nonBlocking = 1;
	BOOL noDelay = TRUE;	// 작은 패킷을 모아 두지 않고 바로 보냄 (Nagle 끔)
	ioctlsocket(sock, FIONBIO, &nonBlocking);
	setsockopt(sock, IPPROTO_TCP, TCP_NODELAY, (const char*)&noDelay, sizeof(noDelay));

	if (connect(sock, (sockaddr*)&addr, sizeof(addr)) == SOCKET_ERROR && WSAGetLastError() != WSAEWOULDBLOCK) {
		closesocket(sock);
		sock = INVALID_SOCKET;
		return false;
	}
	connectionState = ConnectionState::CS_CONNECTING;
	connectStartTime = GetTickCount64();
	return true;
}

NetworkManager::ConnectionState NetworkManager::CheckConnect()
{
	// [C1] CheckConnect: 연결 완료·진행 중·실패 확인 (3초 제한)
	if (connectionState != ConnectionState::CS_CONNECTING)
		return connectionState;

	// 넌블로킹 connect는 성공하면 쓰기 가능, 실패하면 예외 집합에 뜬다
	fd_set writeSet, errorSet;
	FD_ZERO(&writeSet);
	FD_ZERO(&errorSet);
	FD_SET(sock, &writeSet);
	FD_SET(sock, &errorSet);
	timeval noWait{};
	bool failed = select(0, nullptr, &writeSet, &errorSet, &noWait) == SOCKET_ERROR || FD_ISSET(sock, &errorSet);

	if (!failed && FD_ISSET(sock, &writeSet))
		connectionState = ConnectionState::CS_CONNECTED;
	else if (failed || GetTickCount64() - connectStartTime >= 3000) {
		closesocket(sock);
		sock = INVALID_SOCKET;
		connectionState = ConnectionState::CS_FAILED;
	}
	return connectionState;
}

void NetworkManager::RecvAll()
{
	if (sock == INVALID_SOCKET || connectionState != ConnectionState::CS_CONNECTED || receiveEnded)
		return;

	// 버퍼가 차면 이번 틱에 조립하고 다음 틱에 이어서 읽음
	while (recvLength < RECV_BUFFER_SIZE) {
		const int received = recv(sock, recvBuffer + recvLength, RECV_BUFFER_SIZE - recvLength, 0);
		if (received > 0) {
			recvLength += received;
			continue;
		}
		if (received == 0 || WSAGetLastError() != WSAEWOULDBLOCK)
			receiveEnded = true;
		break;
	}
}

void NetworkManager::ProcessPackets()
{
	constexpr int headerSize = sizeof(PacketHeader);
	int consumed = 0;
	while (recvLength - consumed >= headerSize) {
		const char* bytes = recvBuffer + consumed;
		PacketHeader header{};
		std::memcpy(&header, bytes, headerSize);
		if (header.size < headerSize || header.size > MAX_PACKET_SIZE) {
			CloseTransport();
			return;
		}
		if (header.size > recvLength - consumed)
			break;

		consumed += header.size;
		if (!stateStack.empty())
			stateStack.back()->process_packet(reinterpret_cast<const PacketHeader*>(bytes));
	}

	if (receiveEnded) {
		// 종료 직전에 받은 완성 패킷까지 전달한 뒤 정리
		CloseTransport();
		return;
	}
	if (consumed > 0) {
		recvLength -= consumed;
		std::memmove(recvBuffer, recvBuffer + consumed, recvLength);
	}
}

void NetworkManager::Send(const void* packet, int size)
{
	if (!packet || size < static_cast<int>(sizeof(PacketHeader)) || size > MAX_PACKET_SIZE ||
		sock == INVALID_SOCKET || connectionState != ConnectionState::CS_CONNECTED)
		return;

	const auto* bytes = static_cast<const char*>(packet);
	sendBuffer.insert(sendBuffer.end(), bytes, bytes + size);
}

void NetworkManager::FlushSend()
{
	if (sock == INVALID_SOCKET || connectionState != ConnectionState::CS_CONNECTED || sendBuffer.empty())
		return;

	const int length = static_cast<int>((std::min)(sendBuffer.size(), static_cast<size_t>(INT_MAX)));
	const int sent = send(sock, sendBuffer.data(), length, 0);
	if (sent > 0)
		sendBuffer.erase(sendBuffer.begin(), sendBuffer.begin() + sent);
	else if (sent == 0 || WSAGetLastError() != WSAEWOULDBLOCK)
		CloseTransport();
}

void NetworkManager::CloseTransport()
{
	if (sock != INVALID_SOCKET)
		closesocket(sock);
	sock = INVALID_SOCKET;
	connectionState = ConnectionState::CS_DISCONNECTED;
	recvLength = 0;
	receiveEnded = false;
	sendBuffer.clear();
	OnDisconnected();
}

void NetworkManager::Disconnect()
{
	// [C11] Disconnect: 소켓 닫기
}

void NetworkManager::OnDisconnected()
{
	// [C11] OnDisconnected: 연결이 끊기면 타이틀로 이동
}

void NetworkManager::Cleanup()
{
	// [C11] Cleanup: 프로그램 종료 시 WSACleanup
}
