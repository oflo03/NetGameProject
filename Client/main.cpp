#include "Common.h"
#include "Framework.h"
#include "Scenes.h"

// ---- 전역 변수 정의 (선언은 Common.h) ----
double frame_time;
double volume = 1.0;
HDC mDC;
RECT screen;
Vector2D<float> monitorSize;
Vector2D<float> camSize;
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
	WndClass.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);
	WndClass.lpszMenuName = NULL;
	WndClass.lpszClassName = IpszClass;
	WndClass.hIconSm = LoadIcon(NULL, IDI_APPLICATION);
	RegisterClassExW(&WndClass);

	HMONITOR hMonitor = MonitorFromWindow(NULL, MONITOR_DEFAULTTOPRIMARY);
	MONITORINFO monitorInfo;
	monitorInfo.cbSize = sizeof(MONITORINFO);
	GetMonitorInfo(hMonitor, &monitorInfo);

	monitorSize = Vector2D<float>(monitorInfo.rcMonitor.right - monitorInfo.rcMonitor.left,
		monitorInfo.rcMonitor.bottom - monitorInfo.rcMonitor.top);
	camSize = monitorSize / 3;

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
		GetClientRect(hWnd, &screen);
		start_game(new LogoState());
		SetTimer(hWnd, 1, 10, NULL);
		ShowCursor(false);
		cursor.Load(L"resources/UI_Image_Cursor.png");
		break;
	case WM_PAINT:
	{
		hDC = BeginPaint(hWnd, &ps);
		HBITMAP hBitmap = CreateCompatibleBitmap(hDC, monitorSize.x, monitorSize.y);
		mDC = CreateCompatibleDC(hDC);
		SelectObject(mDC, hBitmap);
		stateStack.back()->draw();
		BitBlt(hDC, 0, 0, 1920, 1080, mDC, 0, 0, SRCCOPY);
		DeleteObject(hBitmap);
		DeleteDC(mDC);
		EndPaint(hWnd, &ps);
		break;
	}
	case WM_TIMER:
		if (wParam == 1) {
			GetCursorPos(&mPoint);
			frame_time = GetFrameTime();
			stateStack.back()->handle_events();
			stateStack.back()->update();
			InvalidateRect(hWnd, NULL, false);
		}
		break;
	case WM_DESTROY:
		end_game();
		PostQuitMessage(0);
		break;
	default:
		break;
	}
	return DefWindowProc(hWnd, iMessage, wParam, lParam);
}
