#include "Common.h"
#include "Framework.h"
#include "NetworkManager.h"
#include "Scenes.h"

namespace {
	constexpr int gameWidth = 1920;
	constexpr int gameHeight = 1080;

	RECT GameViewport(const RECT& client)
	{
		const LONG width = client.right - client.left;
		const LONG height = client.bottom - client.top;
		LONG fittedWidth = width;
		LONG fittedHeight = height;
		if (static_cast<LONGLONG>(width) * gameHeight > static_cast<LONGLONG>(height) * gameWidth)
			fittedWidth = MulDiv(height, gameWidth, gameHeight);
		else
			fittedHeight = MulDiv(width, gameHeight, gameWidth);

		const LONG left = client.left + (width - fittedWidth) / 2;
		const LONG top = client.top + (height - fittedHeight) / 2;
		return { left, top, left + fittedWidth, top + fittedHeight };
	}

	POINT GameCursor(POINT point, const RECT& viewport)
	{
		const LONG width = viewport.right - viewport.left;
		const LONG height = viewport.bottom - viewport.top;
		if (width <= 0 || height <= 0)
			return { -1, -1 };

		// 여백의 좌표는 게임 화면 밖으로 유지한다.
		const auto x = static_cast<LONGLONG>(point.x) - viewport.left;
		const auto y = static_cast<LONGLONG>(point.y) - viewport.top;
		return { x < 0 ? -1 : static_cast<LONG>(x * gameWidth / width),
			y < 0 ? -1 : static_cast<LONG>(y * gameHeight / height) };
	}
}

// ---- 전역 변수 정의 (선언은 Common.h) ----
double frame_time;
double volume = 1.0;
HDC mDC;
RECT screen{ 0, 0, gameWidth, gameHeight };
Vector2D<float> monitorSize(gameWidth, gameHeight);	// 게임 내부 좌표계
Vector2D<float> camSize = monitorSize / 3;
Vector2D<float> camPos;
CImage cursor;
POINT mPoint;

std::random_device rd;
std::default_random_engine dre(rd());
std::uniform_int_distribution<int> uid(1, 10000);
std::uniform_int_distribution<int> rad(-180, 180);
std::uniform_int_distribution<int> ranTime(20, 70);
std::uniform_int_distribution<int> ran(0, 1);
std::uniform_int_distribution<int> randrop(0, 1);

std::vector<Collider*> COLL;
std::vector<Bullet*> Bullets;
std::vector<DropItem*> drops;
bool lookRange;
bool beatable = true;
bool enemyclear;

double GetFrameTime()
{
	static LARGE_INTEGER frequency;
	static bool initialized = false;
	static LARGE_INTEGER prevTime;

	if (!initialized)
	{
		QueryPerformanceFrequency(&frequency);
		QueryPerformanceCounter(&prevTime);
		initialized = TRUE;
	}

	LARGE_INTEGER currentTime;
	QueryPerformanceCounter(&currentTime);

	double frameTime = static_cast<double>(currentTime.QuadPart - prevTime.QuadPart) / frequency.QuadPart;
	prevTime = currentTime;

	return frameTime;
}

void GameTick(HWND hWnd)
{
	auto* network = NetworkManager::getInstance();
	network->RecvAll();
	network->ProcessPackets();

	POINT cursorPos{};
	RECT client{};
	if (GetCursorPos(&cursorPos) && ScreenToClient(hWnd, &cursorPos) && GetClientRect(hWnd, &client))
		mPoint = GameCursor(cursorPos, GameViewport(client));
	frame_time = GetFrameTime();
	stateStack.back()->handle_events();
	stateStack.back()->update();

	network->FlushSend();

	InvalidateRect(hWnd, NULL, false);
}

HINSTANCE g_hinst;
LPCTSTR IpszClass = L"Window Class Name";
LPCTSTR IpszWindowName = L"Drop The Gun";
LRESULT WndProc(HWND hWnd, UINT iMessage, WPARAM wParam, LPARAM lParam);

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevinstance, LPSTR lpszCmdParam, int nCmdShow)
{
	HWND hWnd;
	MSG Message;
	WNDCLASSEX WndClass;
	g_hinst = hInstance;

	WndClass.cbSize = sizeof(WndClass);
	WndClass.style = CS_HREDRAW | CS_VREDRAW;
	WndClass.lpfnWndProc = (WNDPROC)WndProc;
	WndClass.cbClsExtra = 0;
	WndClass.cbWndExtra = 0;
	WndClass.hInstance = hInstance;
	WndClass.hIcon = LoadIcon(NULL, IDI_APPLICATION);
	WndClass.hCursor = LoadCursor(NULL, NULL);
	WndClass.hbrBackground = nullptr;
	WndClass.lpszMenuName = NULL;
	WndClass.lpszClassName = IpszClass;
	WndClass.hIconSm = LoadIcon(NULL, IDI_APPLICATION);
	RegisterClassExW(&WndClass);

	HMONITOR hMonitor = MonitorFromWindow(NULL, MONITOR_DEFAULTTOPRIMARY);
	MONITORINFO monitorInfo;
	monitorInfo.cbSize = sizeof(MONITORINFO);
	GetMonitorInfo(hMonitor, &monitorInfo);

	hWnd = CreateWindow(IpszClass, IpszWindowName, WS_POPUP,
		monitorInfo.rcMonitor.left, monitorInfo.rcMonitor.top,
		monitorInfo.rcMonitor.right - monitorInfo.rcMonitor.left,
		monitorInfo.rcMonitor.bottom - monitorInfo.rcMonitor.top,
		NULL, (HMENU)NULL, hInstance, NULL);

	ShowWindow(hWnd, nCmdShow);
	UpdateWindow(hWnd);

	while (GetMessage(&Message, 0, 0, 0)) {
		TranslateMessage(&Message);
		DispatchMessage(&Message);
	}
	return Message.wParam;
}

LRESULT WndProc(HWND hWnd, UINT iMessage, WPARAM wParam, LPARAM lParam)
{
	PAINTSTRUCT ps;
	HDC hDC;
	switch (iMessage)
	{
	case WM_CREATE:
		NetworkManager::getInstance()->Init();
		start_game(new LogoState());
		SetTimer(hWnd, 1, 10, NULL);
		ShowCursor(false);
		cursor.Load(L"resources/UI_Image_Cursor.png");
		break;
	case WM_ERASEBKGND:
		return 1;	// WM_PAINT에서 완성된 화면과 여백을 그린다.
	case WM_PAINT:
	{
		hDC = BeginPaint(hWnd, &ps);
		RECT client{};
		GetClientRect(hWnd, &client);
		const RECT viewport = GameViewport(client);
		const auto black = static_cast<HBRUSH>(GetStockObject(BLACK_BRUSH));
		if (!stateStack.empty() && viewport.right > viewport.left && viewport.bottom > viewport.top) {
			const HBITMAP bitmap = CreateCompatibleBitmap(hDC, gameWidth, gameHeight);
			mDC = CreateCompatibleDC(hDC);
			if (bitmap && mDC) {
				const auto previousBitmap = SelectObject(mDC, bitmap);
				PatBlt(mDC, 0, 0, gameWidth, gameHeight, BLACKNESS);
				stateStack.back()->draw();
				StretchBlt(hDC, viewport.left, viewport.top, viewport.right - viewport.left,
					viewport.bottom - viewport.top, mDC, 0, 0, gameWidth, gameHeight, SRCCOPY);
				SelectObject(mDC, previousBitmap);
			}
			if (mDC)
				DeleteDC(mDC);
			if (bitmap)
				DeleteObject(bitmap);
			mDC = nullptr;

			if (viewport.left != client.left || viewport.right != client.right) {
				const RECT left{ client.left, client.top, viewport.left, client.bottom };
				const RECT right{ viewport.right, client.top, client.right, client.bottom };
				FillRect(hDC, &left, black);
				FillRect(hDC, &right, black);
			} else if (viewport.top != client.top || viewport.bottom != client.bottom) {
				const RECT top{ client.left, client.top, client.right, viewport.top };
				const RECT bottom{ client.left, viewport.bottom, client.right, client.bottom };
				FillRect(hDC, &top, black);
				FillRect(hDC, &bottom, black);
			}
		} else {
			FillRect(hDC, &client, black);
		}
		EndPaint(hWnd, &ps);
		return 0;
	}
	case WM_CHAR:
		// 문자 입력을 현재 화면에 전달 (서버 IP, 닉네임)
		if (!stateStack.empty())
			stateStack.back()->on_char(static_cast<wchar_t>(wParam));
		break;
	case WM_TIMER:
		if (wParam == 1) {
			GameTick(hWnd);
		}
		break;
	case WM_DESTROY:
		end_game();
		NetworkManager::getInstance()->Cleanup();
		NetworkManager::release();
		PostQuitMessage(0);
		break;
	default:
		break;
	}
	return DefWindowProc(hWnd, iMessage, wParam, lParam);
}
