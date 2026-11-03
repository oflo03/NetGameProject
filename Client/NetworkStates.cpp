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
