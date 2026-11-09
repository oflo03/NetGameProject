#pragma once
#include "Session.h"

// ---- 클라이언트 스레드 (접속 하나당 하나) ----

// [S3] 로그인 처리. IsNameTaken과 CountLoggedIn은 room.lock을 잡은 채로 부른다
void HandleLogin(Session* s, const C2S_LOGIN_PACKET* packet);
bool IsNameTaken(const wchar_t* name);
int CountLoggedIn();
void SendLoginResult(Session* s, uint8_t result, uint8_t playerId);

// [S4] 대기실 처리. PassHost는 room.lock을 잡은 채로, BroadcastRoomInfo는 room.lock을 풀고 부른다
void HandleSelectChar(Session* s, const C2S_SELECT_CHAR_PACKET* packet);
void HandleReady(Session* s, const C2S_READY_PACKET* packet);
void HandleStart(Session* s);
void BroadcastRoomInfo();
void PassHost();

// [S5] 패킷 수신
void ClientThreadMain(Session* s);
void RecvAll(Session* s);
void ProcessPackets(Session* s);
void DispatchPacket(Session* s, const PacketHeader* packet);

// [S6] 입력 처리. TakeInput은 게임 로직 스레드가 부른다
void HandleInput(Session* s, const C2S_INPUT_PACKET* packet);
void HandleInteract(Session* s);
InputState TakeInput(int playerId);

// [S7] 패킷 송신. EnqueueSend와 Broadcast는 어느 스레드에서나 부를 수 있다
void EnqueueSend(Session* s, const void* packet, int size);
void Broadcast(const void* packet, int size);	// room.lock을 직접 잡으므로 room.lock을 잡은 채로 부르지 않는다
void FlushSend(Session* s);
