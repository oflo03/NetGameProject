#include "NetworkStates.h"
#include <cstring>
#include <cwchar>
#include "NetworkManager.h"
#include "Scenes.h"

namespace {
	const wchar_t* backgroundImage = L"resources/main_selchar_back.png";
	constexpr COLORREF white = RGB(255, 255, 255);
	constexpr COLORREF yellow = RGB(255, 186, 0);	// 메뉴 강조색
	constexpr COLORREF gray = RGB(160, 160, 160);

	HFONT TitleFont()
	{
		static HFONT font = MakeFont(60, L"Romulus");
		return font;
	}

	HFONT TextFont()
	{
		static HFONT font = MakeFont(40, L"dalmoori");
		return font;
	}

	void DrawLine(const wchar_t* text, HFONT font, COLORREF color, int top, int height)
	{
		RECT rect{ 0, top, screen.right, top + height };
		SelectObject(mDC, font);
		SetTextColor(mDC, color);
		DrawText(mDC, text, -1, &rect, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
	}

	// 서버 IP·닉네임 입력 화면 (제목, 입력 칸, 안내 문구)
	void DrawInputScreen(CImage& background, const wchar_t* title, const std::wstring& text, const std::wstring& message)
	{
		background.AlphaBlend(mDC, 0, 0, screen.right, screen.bottom, 0, 0, 1920, 1080, 60);
		SetBkMode(mDC, TRANSPARENT);
		const int top = screen.bottom / 2 - 160;
		DrawLine(title, TitleFont(), white, top, 80);
		RECT box{ screen.right / 2 - 360, top + 110, screen.right / 2 + 360, top + 180 };
		FrameRect(mDC, &box, (HBRUSH)GetStockObject(WHITE_BRUSH));
		DrawLine((text + L"_").c_str(), TextFont(), white, box.top, box.bottom - box.top);
		DrawLine(message.c_str(), TextFont(), yellow, top + 200, 60);
		DrawLine(L"Enter 확인   Esc 메뉴로", TextFont(), gray, top + 280, 60);
		cursor.Draw(mDC, mPoint.x - 20, mPoint.y - 30, 40, 40);
	}

	// 연결을 끊고 메뉴로 (change_state가 현재 화면을 지우므로 호출 뒤 멤버를 건드리지 않는다)
	void BackToMenu()
	{
		NetworkManager::getInstance()->Disconnect();
		change_state(new MenuState);
	}
}

// ---- ConnectState ----

ConnectState::ConnectState() : serverIp("127.0.0.1")
{
	background.Load(backgroundImage);
}

void ConnectState::on_char(wchar_t ch)
{
	if (connecting)
		return;
	if (ch == L'\r') {
		if (NetworkManager::getInstance()->Connect(serverIp.c_str())) {
			connecting = true;
			message = L"접속 중...";
		}
		else
			message = L"IP 주소 형식이 잘못되었습니다";
	}
	else if (ch == L'\b') {
		if (!serverIp.empty())
			serverIp.pop_back();
	}
	else if (((ch >= L'0' && ch <= L'9') || ch == L'.') && serverIp.size() < 15)
		serverIp += static_cast<char>(ch);
}

void ConnectState::update()
{
	if (!connecting)
		return;
	switch (NetworkManager::getInstance()->CheckConnect()) {
	case NetworkManager::ConnectionState::CS_CONNECTED:
		change_state(new LoginState);
		return;
	case NetworkManager::ConnectionState::CS_FAILED:
		connecting = false;
		message = L"접속 실패: 서버 IP와 실행 여부를 확인하세요";
		break;
	default:
		break;
	}
}

void ConnectState::handle_events()
{
	if (GetAsyncKeyState(VK_ESCAPE) & 1)
		BackToMenu();
}

void ConnectState::draw()
{
	DrawInputScreen(background, L"SERVER IP", std::wstring(serverIp.begin(), serverIp.end()), message);
}

// ---- LoginState ----

LoginState::LoginState()
{
	background.Load(backgroundImage);
}

void LoginState::on_char(wchar_t ch)
{
	if (waiting)
		return;
	const size_t length = std::wcslen(nickname);
	if (ch == L'\r') {
		if (length > 0) {
			SendLogin();
			waiting = true;
			message = L"로그인 중...";
		}
	}
	else if (ch == L'\b') {
		if (length > 0)
			nickname[length - 1] = L'\0';
	}
	else if (ch >= L' ' && ch != 0x7F && length < NAME_LEN - 1)
		nickname[length] = ch;	// 배열이 0으로 초기화돼 있어 끝은 이미 널 문자
}

void LoginState::SendLogin()
{
	C2S_LOGIN_PACKET packet{};
	packet.size = sizeof(packet);
	packet.type = C2S_LOGIN;
	std::memcpy(packet.name, nickname, sizeof(packet.name));
	NetworkManager::getInstance()->Send(&packet, sizeof(packet));
}

void LoginState::process_packet(const PacketHeader* packet)
{
	if (packet->type != S2C_LOGIN_RESULT || packet->size != sizeof(S2C_LOGIN_RESULT_PACKET))
		return;
	S2C_LOGIN_RESULT_PACKET result;
	std::memcpy(&result, packet, sizeof(result));
	waiting = false;
	switch (result.result) {
	case LOGIN_OK:
		if (result.playerId >= MAX_PLAYERS)
			break;
		NetworkManager::getInstance()->playerId = result.playerId;
		change_state(new LobbyState);	// 이 화면은 지워지니 바로 반환
		return;
	case LOGIN_NAME_TAKEN:
		message = L"이미 사용 중인 닉네임입니다";
		return;
	case LOGIN_FULL:
		message = L"방이 가득 찼습니다 (최대 3명)";
		return;
	case LOGIN_IN_GAME:
		message = L"게임이 진행 중입니다";
		return;
	}
	message = L"로그인에 실패했습니다";
}

void LoginState::update()
{
}

void LoginState::handle_events()
{
	if (GetAsyncKeyState(VK_ESCAPE) & 1)
		BackToMenu();
}

void LoginState::draw()
{
	DrawInputScreen(background, L"NICKNAME", nickname, message);
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
