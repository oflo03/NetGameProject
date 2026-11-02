#include "NetworkManager.h"

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
	return false;
}

bool NetworkManager::Connect(const char* serverIp)
{
	// [C1] Connect: 넌블로킹 소켓 생성, connect 시작
	return false;
}

NetworkManager::ConnectionState NetworkManager::CheckConnect()
{
	// [C1] CheckConnect: 연결 완료·진행 중·실패 확인 (3초 제한)
	return ConnectionState::CS_DISCONNECTED;
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
