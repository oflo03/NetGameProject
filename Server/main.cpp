#include <WinSock2.h>
#include <atomic>
#include <mutex>
#include <thread>
#include "ClientThread.h"
#include "LogicThread.h"

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
	return nullptr;
}

void OnAccept(SOCKET sock)
{
	// [S2] OnAccept: 새 소켓 넌블로킹 설정
	Session* s = RegisterSession(sock);
	if (s == nullptr) {
		closesocket(sock);
		return;
	}
	std::thread(ClientThreadMain, s).detach();	// 끝은 WaitClientThreads가 used로 확인
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
	SetConsoleCtrlHandler(OnConsoleCtrl, TRUE);

	std::thread logicThread(LogicThreadMain);
	AcceptLoop();

	// 종료 순서: 게임 로직 스레드 → 클라이언트 스레드 → 소켓 정리
	logicThread.join();
	WaitClientThreads();
	ShutdownServer();
	return 0;
}
