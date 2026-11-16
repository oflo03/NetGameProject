#include <WinSock2.h>
#include <atomic>
#include <cstdio>
#include <mutex>
#include <thread>
#include "ClientThread.h"
#include "LogicThread.h"

Room room;
SOCKET listenSock = INVALID_SOCKET;
std::atomic<bool> serverRunning = true;

void ShutdownServer()
{
	// [S1] ShutdownServer: listen 소켓 닫기, WSACleanup
	if (listenSock != INVALID_SOCKET) {
		closesocket(listenSock);
		listenSock = INVALID_SOCKET;
	}
	WSACleanup();
}

bool InitServer()
{
	// [S1] InitServer: WSAStartup, listen 소켓 생성 (SERVER_PORT), 넌블로킹 설정
	WSADATA wsa;
	if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
		printf("WSAStartup 실패\n");
		return false;
	}

	listenSock = socket(AF_INET, SOCK_STREAM, 0);
	sockaddr_in addr{};
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = htonl(INADDR_ANY);
	addr.sin_port = htons(SERVER_PORT);
	u_long nonBlocking = 1;
	if (listenSock == INVALID_SOCKET
		|| bind(listenSock, (sockaddr*)&addr, sizeof(addr)) == SOCKET_ERROR
		|| listen(listenSock, SOMAXCONN) == SOCKET_ERROR
		|| ioctlsocket(listenSock, FIONBIO, &nonBlocking) == SOCKET_ERROR) {
		printf("listen 소켓 준비 실패 (%d)\n", WSAGetLastError());
		ShutdownServer();
		return false;
	}

	printf("서버 시작 (포트 %d)\n", SERVER_PORT);
	return true;
}

// Ctrl+C나 콘솔 닫기로 서버 종료
BOOL WINAPI OnConsoleCtrl(DWORD ctrlType)
{
	serverRunning = false;
	return TRUE;
}

// 모든 세션의 connected를 끄고 클라이언트 스레드가 ReleaseSession까지 마칠 때까지 대기
void WaitClientThreads()
{
	while (true) {
		{
			std::lock_guard<std::mutex> lock(room.lock);
			bool anyUsed = false;
			for (Session& s : room.sessions) {
				if (s.used) {
					s.connected = false;
					anyUsed = true;
				}
			}
			if (!anyUsed)
				return;
		}
		Sleep(1);
	}
}

Session* RegisterSession(SOCKET sock)
{
	// [S2] RegisterSession: room.lock을 잡고 빈 슬롯에 세션 등록 (used, connected = true, sock), 슬롯이 없으면 nullptr
	std::lock_guard<std::mutex> lock(room.lock);
	for (Session& s : room.sessions) {
		if (!s.used) {
			s.used = true;
			s.connected = true;
			s.sock = sock;
			return &s;
		}
	}
	return nullptr;
}

void OnAccept(SOCKET sock)
{
	// [S2] OnAccept: 새 소켓 넌블로킹 설정, RegisterSession, 클라이언트 스레드 생성 (등록 실패 시 소켓 닫기)
	u_long nonBlocking = 1;
	BOOL noDelay = TRUE;	// 작은 패킷을 모아 두지 않고 바로 보냄 (Nagle 끔)
	ioctlsocket(sock, FIONBIO, &nonBlocking);
	setsockopt(sock, IPPROTO_TCP, TCP_NODELAY, (const char*)&noDelay, sizeof(noDelay));

	Session* s = RegisterSession(sock);
	if (s == nullptr) {
		printf("빈 슬롯 없음, 접속 거부\n");
		closesocket(sock);
		return;
	}
	printf("접속 (슬롯 %d)\n", int(s - room.sessions));
	std::thread(ClientThreadMain, s).detach();	// 끝은 WaitClientThreads가 used로 확인
}

void AcceptLoop()
{
	// [S2] AcceptLoop: accept, WSAEWOULDBLOCK이면 넘어가고 새 소켓이면 OnAccept
	while (serverRunning) {
		SOCKET sock = accept(listenSock, nullptr, nullptr);
		if (sock != INVALID_SOCKET) {
			OnAccept(sock);
			continue;
		}
		if (WSAGetLastError() != WSAEWOULDBLOCK)
			printf("accept 실패 (%d)\n", WSAGetLastError());
		Sleep(1);
	}
}

int main()
{
	SetConsoleOutputCP(CP_UTF8);
	if (!InitServer())
		return 1;
	SetConsoleCtrlHandler(OnConsoleCtrl, TRUE);

	std::thread logicThread(LogicThreadMain);
	AcceptLoop();

	// 종료 순서: 게임 로직 스레드 → 클라이언트 스레드 → 소켓 정리
	logicThread.join();
	WaitClientThreads();
	ShutdownServer();
	return 0;
}
