#include "ClientThread.h"
#include <cmath>

// ---- S3 로그인 처리 ----

void HandleLogin(Session* s, const C2S_LOGIN_PACKET* packet)
{
	// [S3] HandleLogin: 이미 로그인한 세션이면 무시, name 끝을 널 문자로 막음
	// room.lock을 잡고 게임 중 → 인원 → 닉네임 중복 순서로 확인, 통과하면 빈 playerId 발급 (첫 로그인이면 방장)
	// 락을 푼 뒤 SendLoginResult, 성공이면 BroadcastRoomInfo
}

bool IsNameTaken(const wchar_t* name)
{
	// [S3] IsNameTaken: 로그인한 세션 중 같은 닉네임이 있는지 확인
	return false;
}

int CountLoggedIn()
{
	// [S3] CountLoggedIn: 로그인한 세션 수 (playerId >= 0)
	return 0;
}

void SendLoginResult(Session* s, LoginResult result, UINT8 playerId)
{
	// [S3] SendLoginResult: S2C_LOGIN_RESULT를 EnqueueSend
}

// ---- S4 대기실 처리 ----

void HandleSelectChar(Session* s, const C2S_SELECT_CHAR_PACKET* packet)
{
	// [S4] HandleSelectChar: room.lock을 잡고 로그인 전이거나 게임 중이거나 charType이 CHAR_MARIN·CHAR_KNIGHT가 아니면 무시, charType 갱신
	// 락을 푼 뒤 BroadcastRoomInfo
}

void HandleReady(Session* s, const C2S_READY_PACKET* packet)
{
	// [S4] HandleReady: room.lock을 잡고 로그인 전이거나 게임 중이면 무시, ready 갱신
	// 락을 푼 뒤 BroadcastRoomInfo
}

void HandleStart(Session* s)
{
	// [S4] HandleStart: room.lock을 잡고 방장인지, 로그인한 사람 전원이 레디인지 확인, 맞으면 startRequested = true
}

void BroadcastRoomInfo()
{
	// [S4] BroadcastRoomInfo: room.lock을 잡고 S2C_ROOM_INFO 작성, 락을 푼 뒤 Broadcast
}

void PassHost()
{
	// [S4] PassHost: 남은 로그인 세션 중 가장 작은 playerId를 hostId로, 없으면 -1
}

// ---- S5 패킷 수신 ----

void ClientThreadMain(Session* s)
{
	while (s->connected) {
		FlushSend(s);
		Sleep(1);
		RecvAll(s);
		ProcessPackets(s);
	}
	OnDisconnect(s);
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
	// [S5] DispatchPacket: packet->size가 종류별 구조체 크기와 같은지 확인, 다르면 connected = false
	switch (packet->type)
	{
	case C2S_LOGIN:
		HandleLogin(s, static_cast<const C2S_LOGIN_PACKET*>(packet));
		break;
	case C2S_SELECT_CHAR:
		HandleSelectChar(s, static_cast<const C2S_SELECT_CHAR_PACKET*>(packet));
		break;
	case C2S_READY:
		HandleReady(s, static_cast<const C2S_READY_PACKET*>(packet));
		break;
	case C2S_START:
		HandleStart(s);
		break;
	case C2S_INPUT:
		HandleInput(s, static_cast<const C2S_INPUT_PACKET*>(packet));
		break;
	case C2S_INTERACT:
		HandleInteract(s);
		break;
	default:
		// [S5] 모르는 패킷 종류면 connected = false
		break;
	}
}

// ---- S6 입력 처리 ----

namespace {
	constexpr UINT8 actionMask = KEY_ROLL | KEY_WEAPON_PREV | KEY_WEAPON_NEXT;

	// room.lock을 잡은 상태에서 확인
	bool CanAcceptInput(const Session& session)
	{
		return room.state == ROOM_IN_GAME && session.used && session.connected &&
			session.playerId >= 0 && session.playerId < MAX_PLAYERS;
	}
}

void HandleInput(Session* s, const C2S_INPUT_PACKET* packet)
{
	if (!s || !packet || packet->size != sizeof(C2S_INPUT_PACKET) ||
		!std::isfinite(packet->aimX) || !std::isfinite(packet->aimY))
		return;
	if (packet->weapon != WEAPON_KEEP && packet->weapon > WEAPON_ROCKET)
		return;

	std::lock_guard roomGuard(room.lock);
	if (!CanAcceptInput(*s))
		return;
	std::lock_guard inputGuard(s->inputLock);
	auto& input = s->input;
	input.keys = static_cast<InputKey>(packet->keys | (input.keys & actionMask));
	if (packet->weapon != WEAPON_KEEP)
		input.weapon = packet->weapon;
	input.aimX = packet->aimX;
	input.aimY = packet->aimY;
}

void HandleInteract(Session* s)
{
	std::lock_guard roomGuard(room.lock);
	if (!s || !CanAcceptInput(*s))
		return;
	std::lock_guard inputGuard(s->inputLock);
	s->input.interact = true;
}

InputState TakeInput(int playerId)
{
	if (playerId < 0 || playerId >= MAX_PLAYERS)
		return {};

	std::lock_guard roomGuard(room.lock);
	for (auto& session : room.sessions) {
		if (session.playerId != playerId || !CanAcceptInput(session))
			continue;
		std::lock_guard inputGuard(session.inputLock);
		const InputState input = session.input;
		session.input.keys = static_cast<InputKey>(input.keys & ~actionMask);
		session.input.weapon = WEAPON_KEEP;
		session.input.interact = false;
		return input;
	}
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

// ---- S8 접속 종료 처리 ----

void OnDisconnect(Session* s)
{
	// [S8] OnDisconnect: room.lock을 잡고 로그인한 세션이면 게임 중 이탈 기록(leftPlayers), playerId = -1, 방장이었으면 PassHost
	// 락을 푼 뒤 남은 클라이언트에 알림 (게임 중 S2C_PLAYER_LEFT, 대기실 BroadcastRoomInfo), 마지막에 ReleaseSession
}

void ReleaseSession(Session* s)
{
	// [S8] ReleaseSession: closesocket, room.lock을 잡고 슬롯을 필드별로 초기화 (sendQueue, input, recvLength 포함), 마지막에 used = false
}
