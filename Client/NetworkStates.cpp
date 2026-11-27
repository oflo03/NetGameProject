#include "NetworkStates.h"
#include <cstring>
#include <cwchar>
#include "NetworkManager.h"
#include "PlayState.h"
#include "Scenes.h"
#include "Sound.h"

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

	void DrawTextIn(const wchar_t* text, HFONT font, COLORREF color, RECT rect)
	{
		SelectObject(mDC, font);
		SetTextColor(mDC, color);
		DrawText(mDC, text, -1, &rect, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
	}

	// 화면 가로 가운데 한 줄
	void DrawLine(const wchar_t* text, HFONT font, COLORREF color, int top, int height)
	{
		DrawTextIn(text, font, color, { 0, top, screen.right, top + height });
	}

	// 3픽셀 테두리
	void DrawFrame(RECT rect, COLORREF color)
	{
		HBRUSH brush = CreateSolidBrush(color);
		for (int i = 0; i < 3; ++i) {
			FrameRect(mDC, &rect, brush);
			InflateRect(&rect, -1, -1);
		}
		DeleteObject(brush);
	}

	// 선택됐으면 노란색, 마우스가 올라가 있으면 흰색, 아니면 회색
	void DrawButton(const RECT& rect, const wchar_t* text, bool selected)
	{
		const COLORREF color = selected ? yellow : PtInRect(&rect, mPoint) ? white : gray;
		DrawFrame(rect, color);
		DrawTextIn(text, TitleFont(), color, rect);
	}

	// 서버 IP·닉네임 입력 화면 (제목, 입력 칸, 안내 문구)
	void DrawInputScreen(CImage& background, const wchar_t* title, const std::wstring& text, const std::wstring& message)
	{
		background.AlphaBlend(mDC, 0, 0, screen.right, screen.bottom, 0, 0, 1920, 1080, 60);
		SetBkMode(mDC, TRANSPARENT);
		const int top = screen.bottom / 2 - 160;
		DrawLine(title, TitleFont(), white, top, 80);
		const RECT box{ screen.right / 2 - 360, top + 110, screen.right / 2 + 360, top + 180 };
		DrawFrame(box, white);
		DrawTextIn((text + L"_").c_str(), TextFont(), white, box);
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
	background.Load(backgroundImage);
	charImage[CHAR_MARIN].Load(L"resources/marin_front.png");
	charImage[CHAR_KNIGHT].Load(L"resources/knight_front.png");

	// 멤버 카드 3장을 가로로, 그 아래 버튼 4개
	constexpr int cardWidth = 400, cardHeight = 420, cardGap = 60, cardTop = 200;
	const int cardLeft = (screen.right - (cardWidth * MAX_PLAYERS + cardGap * (MAX_PLAYERS - 1))) / 2;
	for (int i = 0; i < MAX_PLAYERS; ++i) {
		const int left = cardLeft + i * (cardWidth + cardGap);
		memberRect[i] = { left, cardTop, left + cardWidth, cardTop + cardHeight };
	}
	constexpr int buttonWidth = 260, buttonHeight = 70, buttonGap = 40, buttonTop = cardTop + cardHeight + 60;
	RECT* buttons[] = { &charButton[CHAR_MARIN], &charButton[CHAR_KNIGHT], &readyButton, &startButton };
	const int buttonLeft = (screen.right - (buttonWidth * 4 + buttonGap * 3)) / 2;
	for (int i = 0; i < 4; ++i) {
		const int left = buttonLeft + i * (buttonWidth + buttonGap);
		*buttons[i] = { left, buttonTop, left + buttonWidth, buttonTop + buttonHeight };
	}
}

const RoomMember* LobbyState::Me() const
{
	const int id = NetworkManager::getInstance()->playerId;
	if (!hasRoomInfo || id < 0 || id >= MAX_PLAYERS || !roomInfo.members[id].used)
		return nullptr;
	return &roomInfo.members[id];
}

bool LobbyState::IsHost() const
{
	return hasRoomInfo && roomInfo.hostId == NetworkManager::getInstance()->playerId;
}

void LobbyState::SendSelectChar(uint8_t charType)
{
	C2S_SELECT_CHAR_PACKET packet{};
	packet.size = sizeof(packet);
	packet.type = C2S_SELECT_CHAR;
	packet.charType = static_cast<CharType>(charType);
	NetworkManager::getInstance()->Send(&packet, sizeof(packet));
}

void LobbyState::SendReady(bool ready)
{
	C2S_READY_PACKET packet{};
	packet.size = sizeof(packet);
	packet.type = C2S_READY;
	packet.ready = ready ? READY_YES : READY_NO;
	NetworkManager::getInstance()->Send(&packet, sizeof(packet));
}

void LobbyState::SendStart()
{
	C2S_START_PACKET packet{};
	packet.size = sizeof(packet);
	packet.type = C2S_START;
	NetworkManager::getInstance()->Send(&packet, sizeof(packet));
}

void LobbyState::process_packet(const PacketHeader* packet)
{
	if (packet->type == S2C_ROOM_INFO && packet->size == sizeof(S2C_ROOM_INFO_PACKET)) {
		std::memcpy(&roomInfo, packet, sizeof(roomInfo));
		for (RoomMember& member : roomInfo.members)
			member.name[NAME_LEN - 1] = L'\0';
		hasRoomInfo = true;
	}
	else if (packet->type == S2C_GAME_START && packet->size == sizeof(S2C_GAME_START_PACKET)) {
		// 내 ID와 대기실 정보(캐릭터, 닉네임)를 PlayState가 복사한 뒤 이 화면이 지워진다
		change_state(new PlayState(NetworkManager::getInstance()->playerId, roomInfo));
	}
}

void LobbyState::update()
{
	SoundManager::getInstance()->update();
}

void LobbyState::handle_events()
{
	if (GetAsyncKeyState(VK_ESCAPE) & 1) {
		BackToMenu();
		return;
	}
	if (!(GetAsyncKeyState(VK_LBUTTON) & 1))
		return;
	const RoomMember* me = Me();
	if (me == nullptr)
		return;

	bool clicked = true;
	if (PtInRect(&charButton[CHAR_MARIN], mPoint))
		SendSelectChar(CHAR_MARIN);
	else if (PtInRect(&charButton[CHAR_KNIGHT], mPoint))
		SendSelectChar(CHAR_KNIGHT);
	else if (PtInRect(&readyButton, mPoint))
		SendReady(me->ready != READY_YES);
	else if (IsHost() && PtInRect(&startButton, mPoint))
		SendStart();
	else
		clicked = false;
	if (clicked)
		SoundManager::getInstance()->play(BUTTONCLICK);
}

void LobbyState::draw()
{
	background.AlphaBlend(mDC, 0, 0, screen.right, screen.bottom, 0, 0, 1920, 1080, 60);
	SetBkMode(mDC, TRANSPARENT);
	DrawLine(L"LOBBY", TitleFont(), white, 80, 80);

	const int myId = NetworkManager::getInstance()->playerId;
	for (int i = 0; i < MAX_PLAYERS; ++i) {
		const RECT& card = memberRect[i];
		const RoomMember& member = roomInfo.members[i];
		if (!hasRoomInfo || !member.used) {
			DrawFrame(card, RGB(90, 90, 90));
			DrawTextIn(L"EMPTY", TitleFont(), RGB(90, 90, 90), card);
			continue;
		}
		DrawFrame(card, i == myId ? yellow : white);
		DrawTextIn(member.name, TextFont(), white, { card.left, card.top + 20, card.right, card.top + 80 });
		if (roomInfo.hostId == i)
			DrawTextIn(L"방장", TextFont(), yellow, { card.left, card.top + 80, card.right, card.top + 120 });

		const bool knight = member.charType == CHAR_KNIGHT;
		CImage& image = charImage[knight ? CHAR_KNIGHT : CHAR_MARIN];
		if (!image.IsNull()) {
			const int height = 190, width = image.GetWidth() * height / image.GetHeight();
			const int centerX = (card.left + card.right) / 2;
			image.AlphaBlend(mDC, centerX - width / 2, card.top + 130, width, height, 0, 0, image.GetWidth(), image.GetHeight());
		}
		DrawTextIn(knight ? L"KNIGHT" : L"MARIN", TextFont(), white, { card.left, card.top + 325, card.right, card.top + 365 });
		const bool ready = member.ready == READY_YES;
		DrawTextIn(ready ? L"READY" : L"NOT READY", TextFont(), ready ? yellow : gray, { card.left, card.top + 365, card.right, card.top + 405 });
	}

	const RoomMember* me = Me();
	DrawButton(charButton[CHAR_MARIN], L"MARIN", me && me->charType == CHAR_MARIN);
	DrawButton(charButton[CHAR_KNIGHT], L"KNIGHT", me && me->charType == CHAR_KNIGHT);
	DrawButton(readyButton, L"READY", me && me->ready == READY_YES);
	if (IsHost())
		DrawButton(startButton, L"START", false);
	DrawLine(IsHost() ? L"모두 레디하면 START로 시작   Esc 메뉴로" : L"방장이 시작하면 게임이 시작됩니다   Esc 메뉴로",
		TextFont(), gray, readyButton.bottom + 40, 60);
	cursor.Draw(mDC, mPoint.x - 20, mPoint.y - 30, 40, 40);
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
