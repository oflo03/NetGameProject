#include <WinSock2.h>
#include <atomic>
#include "Protocol.h"
#include "Session.h"

Room room;
SOCKET listenSock = INVALID_SOCKET;
std::atomic<bool> serverRunning = true;

bool InitServer()
{
	// [S1] InitServer: WSAStartup, listen 소켓 생성 (SERVER_PORT), 넌블로킹 설정
	return false;
}

void ShutdownServer()
{
	// [S1] ShutdownServer: listen 소켓 닫기, WSACleanup
}

Session* RegisterSession(SOCKET sock)
{
	// [S2] RegisterSession: room.lock을 잡고 빈 슬롯에 세션 등록, 슬롯이 없으면 nullptr
	return nullptr;
}

void OnAccept(SOCKET sock)
{
	// [S2] OnAccept: 새 소켓 넌블로킹 설정, RegisterSession, 클라이언트 스레드 생성 (등록 실패 시 소켓 닫기)
}

void AcceptLoop()
{
	while (serverRunning) {
		// [S2] AcceptLoop: accept, WSAEWOULDBLOCK이면 넘어가고 새 소켓이면 OnAccept
		Sleep(1);
	}
}

int main()
{
	if (!InitServer())
		return 1;
	// [S1] 게임 로직 스레드 생성
	AcceptLoop();
	// [S1] 게임 로직 스레드 종료 대기
	ShutdownServer();
	return 0;
}
