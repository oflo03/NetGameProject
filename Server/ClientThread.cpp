#include "ClientThread.h"

// ---- S5 패킷 수신 ----

void ClientThreadMain(Session* s)
{
	while (s->connected) {
		FlushSend(s);
		Sleep(1);
		RecvAll(s);
		ProcessPackets(s);
	}
	// [S8] OnDisconnect: 세션 정리, 남은 클라이언트에 알림
}

void RecvAll(Session* s)
{
	// [S5] RecvAll: recv를 WSAEWOULDBLOCK까지 반복해 recvBuffer 뒤에 붙임, 0이나 오류면 connected = false
}

void ProcessPackets(Session* s)
{
	// [S5] ProcessPackets: recvBuffer를 size 단위로 잘라 DispatchPacket 호출, 남은 조각은 앞으로 당김
	// size가 헤더보다 작거나 MAX_PACKET_SIZE보다 크면 connected = false
}

void DispatchPacket(Session* s, const PacketHeader* packet)
{
	// [S5] DispatchPacket: packet->type별 처리 함수 호출 (S3 로그인, S4 대기실, S6 입력)
}

// ---- S6 입력 처리 ----

void HandleInput(Session* s, const C2S_INPUT_PACKET* packet)
{
	// [S6] HandleInput: 게임 중인지 확인, inputLock을 잡고 input 갱신 (구르기·무기 교체 비트는 TakeInput 전까지 유지)
}

void HandleInteract(Session* s)
{
	// [S6] HandleInteract: 게임 중인지 확인, inputLock을 잡고 input.interact = true
}

InputState TakeInput(int playerId)
{
	// [S6] TakeInput: room.lock → 해당 세션 inputLock 순서로 잡고 input을 꺼냄, 한 번만 쓰는 비트(구르기·무기 교체·interact)는 비움
	return {};
}

// ---- S7 패킷 송신 ----

void EnqueueSend(Session* s, const void* packet, int size)
{
	// [S7] EnqueueSend: sendLock을 잡고 sendQueue 뒤에 패킷 바이트 추가
}

void Broadcast(const void* packet, int size)
{
	// [S7] Broadcast: room.lock을 잡고 로그인한 모든 세션에 EnqueueSend
}

void FlushSend(Session* s)
{
	// [S7] FlushSend: sendLock을 잡고 sendQueue를 send, 보낸 만큼 앞에서 지움 (WSAEWOULDBLOCK이면 다음 루프에), 오류면 connected = false
}
