#include "ClientThread.h"
#include <cstdio>
#include <cstring>
#include <cmath>
#include <cwchar>

// ---- S3 로그인 처리 ----

void HandleLogin(Session* s, const C2S_LOGIN_PACKET* packet)
{
	if (packet->size != sizeof(C2S_LOGIN_PACKET))
		return;	// 크기가 다르면 연결 종료는 11/27 작업, 여기서는 읽지 않고 무시
	wchar_t name[NAME_LEN];
	std::memcpy(name, packet->name, sizeof(name));
	name[NAME_LEN - 1] = L'\0';	// 클라가 널 문자를 안 넣어 보내도 끝을 막음
	if (name[0] == L'\0')
		return;

	LoginResult result = LOGIN_OK;
	int playerId = -1;
	{
		std::lock_guard<std::mutex> lock(room.lock);
		if (s->playerId >= 0)
			return;	// 이미 로그인한 세션
		// [S3] 게임 진행 중 거절(LOGIN_IN_GAME)은 11/27 작업
		if (CountLoggedIn() >= MAX_PLAYERS)
			result = LOGIN_FULL;
		else if (IsNameTaken(name))
			result = LOGIN_NAME_TAKEN;
		else {
			// 비어 있는 가장 작은 playerId 발급
			bool taken[MAX_PLAYERS] = {};
			for (const Session& other : room.sessions)
				if (other.used && other.playerId >= 0)
					taken[other.playerId] = true;
			playerId = 0;
			while (taken[playerId])
				++playerId;

			s->playerId = playerId;
			std::memcpy(s->name, name, sizeof(name));
			s->charType = CHAR_MARIN;
			s->ready = READY_NO;
			if (room.hostId < 0)
				room.hostId = playerId;	// 첫 로그인이면 방장
		}
	}

	SendLoginResult(s, result, playerId < 0 ? 0 : (UINT8)playerId);
	if (result == LOGIN_OK) {
		printf("로그인 (슬롯 %d, 플레이어 %d)\n", int(s - room.sessions), playerId);
		BroadcastRoomInfo();
	}
}

bool IsNameTaken(const wchar_t* name)
{
	for (const Session& s : room.sessions)
		if (s.used && s.playerId >= 0 && std::wcscmp(s.name, name) == 0)
			return true;
	return false;
}

int CountLoggedIn()
{
	int count = 0;
	for (const Session& s : room.sessions)
		if (s.used && s.playerId >= 0)
			++count;
	return count;
}

void SendLoginResult(Session* s, LoginResult result, UINT8 playerId)
{
	S2C_LOGIN_RESULT_PACKET packet{};
	packet.size = sizeof(packet);
	packet.type = S2C_LOGIN_RESULT;
	packet.result = result;
	packet.playerId = playerId;
	EnqueueSend(s, &packet, sizeof(packet));
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
	while (s->recvLength < (int)sizeof(s->recvBuffer)) {
		int received = recv(s->sock, s->recvBuffer + s->recvLength, (int)sizeof(s->recvBuffer) - s->recvLength, 0);
		if (received > 0) {
			s->recvLength += received;
			continue;
		}
		if (received == SOCKET_ERROR && WSAGetLastError() == WSAEWOULDBLOCK)
			return;
		s->connected = false;	// 0이면 상대가 끊음
		return;
	}
}

void ProcessPackets(Session* s)
{
	// [S5] ProcessPackets: recvBuffer를 size 단위로 잘라 DispatchPacket 호출, 남은 조각은 앞으로 당김
	// size가 헤더보다 작거나 MAX_PACKET_SIZE보다 크면 connected = false
	int offset = 0;
	while (s->connected && s->recvLength - offset >= (int)sizeof(PacketHeader)) {
		const PacketHeader* packet = (const PacketHeader*)(s->recvBuffer + offset);
		if (packet->size < sizeof(PacketHeader) || packet->size > MAX_PACKET_SIZE) {
			s->connected = false;
			return;
		}
		if (s->recvLength - offset < packet->size)
			break;
		DispatchPacket(s, packet);
		offset += packet->size;
	}
	memmove(s->recvBuffer, s->recvBuffer + offset, s->recvLength - offset);
	s->recvLength -= offset;
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
		s->connected = false;	// 모르는 패킷 종류
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
	const char* bytes = (const char*)packet;
	std::lock_guard<std::mutex> lock(s->sendLock);
	s->sendQueue.insert(s->sendQueue.end(), bytes, bytes + size);
}

void Broadcast(const void* packet, int size)
{
	// [S7] Broadcast: room.lock을 잡고 로그인한 모든 세션에 EnqueueSend
	std::lock_guard<std::mutex> lock(room.lock);
	for (Session& s : room.sessions) {
		if (s.used && s.playerId >= 0)
			EnqueueSend(&s, packet, size);
	}
}

void FlushSend(Session* s)
{
	// [S7] FlushSend: sendLock을 잡고 sendQueue를 send, 보낸 만큼 앞에서 지움 (WSAEWOULDBLOCK이면 다음 루프에), 오류면 connected = false
	std::lock_guard<std::mutex> lock(s->sendLock);
	int sentTotal = 0;
	while (sentTotal < (int)s->sendQueue.size()) {
		int sent = send(s->sock, s->sendQueue.data() + sentTotal, (int)s->sendQueue.size() - sentTotal, 0);
		if (sent == SOCKET_ERROR) {
			if (WSAGetLastError() != WSAEWOULDBLOCK)
				s->connected = false;
			break;
		}
		sentTotal += sent;
	}
	s->sendQueue.erase(s->sendQueue.begin(), s->sendQueue.begin() + sentTotal);
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
