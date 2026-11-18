#include "NetworkManager.h"
#include <WS2tcpip.h>

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
	// [C4] RecvAll: recv를 WSAEWOULDBLOCK까지 반복
}

void NetworkManager::ProcessPackets()
{
	// [C4] ProcessPackets: 버퍼를 패킷 단위로 잘라 현재 화면에 전달
}

void NetworkManager::Send(const void* packet, int size)
{
	// [C8] Send: 패킷을 송신 버퍼에 추가
}

void NetworkManager::FlushSend()
{
	// [C8] FlushSend: 송신 버퍼를 send, 못 보낸 데이터는 다음 틱에
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
