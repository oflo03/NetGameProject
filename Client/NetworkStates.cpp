#include "NetworkStates.h"

// ---- ConnectState ----

ConnectState::ConnectState()
{
	// [C1] ConnectState: 서버 접속 화면 초기화
}

void ConnectState::on_char(wchar_t ch)
{
	// [C1] on_char: 서버 IP 입력, Enter 시 접속 시작
}

void ConnectState::update()
{
	// [C1] update: 연결되면 로그인 화면으로, 실패하면 메시지 표시
}

void ConnectState::handle_events()
{
	// [C1] handle_events: 서버 접속 화면 입력 처리
}

void ConnectState::draw()
{
	// [C1] draw: 서버 IP와 접속 상태 그리기
}

// ---- LoginState ----

LoginState::LoginState()
{
	// [C2] LoginState: 로그인 화면 초기화
}

void LoginState::on_char(wchar_t ch)
{
	// [C2] on_char: 닉네임 입력, Enter 시 로그인 요청
}

void LoginState::SendLogin()
{
	// [C2] SendLogin: C2S_LOGIN 전송
}

void LoginState::process_packet(const PacketHeader* packet)
{
	// [C2] process_packet: 로그인 결과 처리, 성공 시 대기실로, 실패 시 사유 표시
}

void LoginState::update()
{
	// [C2] update: 로그인 화면 갱신
}

void LoginState::handle_events()
{
	// [C2] handle_events: 로그인 화면 입력 처리
}

void LoginState::draw()
{
	// [C2] draw: 로그인 화면 그리기
}

// ---- LobbyState ----

LobbyState::LobbyState()
{
	// [C3] LobbyState: 대기실 화면 초기화
}

void LobbyState::SendSelectChar(uint8_t charType)
{
	// [C3] SendSelectChar: 캐릭터 선택 요청 전송
}

void LobbyState::SendReady(bool ready)
{
	// [C3] SendReady: 레디 상태 변경 요청 전송
}

void LobbyState::SendStart()
{
	// [C3] SendStart: 방장의 게임 시작 요청 전송
}

void LobbyState::process_packet(const PacketHeader* packet)
{
	// [C3] process_packet: 대기실 정보 갱신, 게임 시작 시 화면 전환
}

void LobbyState::update()
{
	// [C3] update: 대기실 화면 갱신
}

void LobbyState::handle_events()
{
	// [C3] handle_events: 캐릭터·레디·시작 버튼 처리
}

void LobbyState::draw()
{
	// [C3] draw: 멤버 목록과 버튼 그리기
}

// ---- ResultState ----

ResultState::ResultState()
{
	// [C10] ResultState: 결과 화면 초기화
}

void ResultState::update()
{
	// [C10] update: 결과 화면 갱신
}

void ResultState::handle_events()
{
	// [C10] handle_events: 확인 시 대기실로 복귀
}

void ResultState::draw()
{
	// [C10] draw: 클리어 / 게임오버 결과 표시
}
