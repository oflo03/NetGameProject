#include "PlayState.h"
#include "NetworkManager.h"

namespace {
	constexpr UINT8 actionMask = KEY_ROLL | KEY_WEAPON_PREV | KEY_WEAPON_NEXT;
}

void PlayState::update()
{
	// [C5] update: 서버에서 받은 상태로 화면 갱신
}

void PlayState::handle_events()
{
	SampleInput();
	SendInput();
}

void PlayState::draw()
{
	// [C9] draw: 서버 상태를 바탕으로 화면 그리기
}

void PlayState::OnBulletSpawn(const S2C_BULLET_SPAWN_PACKET& packet)
{
	// [C6] OnBulletSpawn: 총알 생성 연출
}

void PlayState::OnBulletRemove(const S2C_BULLET_REMOVE_PACKET& packet)
{
	// [C6] OnBulletRemove: 총알 제거와 피격 연출
}

void PlayState::OnEnemySpawn(const S2C_ENEMY_SPAWN_PACKET& packet)
{
	// [C6] OnEnemySpawn: 적 등장 연출
}

void PlayState::OnEnemyDead(const S2C_ENEMY_DEAD_PACKET& packet)
{
	// [C6] OnEnemyDead: 적 사망 연출
}

void PlayState::OnPlayerEvent(const S2C_PLAYER_EVENT_PACKET& packet)
{
	// [C6] OnPlayerEvent: 플레이어 이벤트 연출
}

void PlayState::OnDrop(const S2C_DROP_PACKET& packet)
{
	// [C6] OnDrop: 드랍 아이템 생성·제거
}

void PlayState::OnEffect(const S2C_EFFECT_PACKET& packet)
{
	// [C6] OnEffect: 이펙트와 사운드 재생
}

void PlayState::OnBoss(const S2C_BOSS_PACKET& packet)
{
	// [C6] OnBoss: 보스 패턴 연출
}

void PlayState::OnPortal(const S2C_PORTAL_PACKET& packet)
{
	// [C6] OnPortal: 포탈 표시
}

void PlayState::OnStageLoad(const S2C_STAGE_LOAD_PACKET& packet)
{
	// [C6] OnStageLoad: 맵 교체와 보스 컷신
}

void PlayState::SampleInput()
{
	UINT8 keys = input.keys & actionMask;
	input.keys = static_cast<InputKey>(keys);
	const HWND window = GetActiveWindow();
	if (!window)
		return;

	const auto held = [](int key) { return (GetAsyncKeyState(key) & 0x8000) != 0; };
	if (held('W')) keys |= KEY_UP;
	if (held('S')) keys |= KEY_DOWN;
	if (held('A')) keys |= KEY_LEFT;
	if (held('D')) keys |= KEY_RIGHT;
	if (held(VK_LBUTTON)) keys |= KEY_ATTACK;

	UINT8 actions = 0;
	if (held(VK_RBUTTON)) actions |= KEY_ROLL;
	if (held(VK_CONTROL) || held(VK_XBUTTON1)) actions |= KEY_WEAPON_PREV;
	if (held(VK_TAB) || held(VK_XBUTTON2)) actions |= KEY_WEAPON_NEXT;
	input.keys = static_cast<InputKey>(keys | (actions & ~previousActions));
	previousActions = actions;

	UINT8 weapons = 0;
	for (int i = 0; i < WEAPON_COUNT; ++i) {
		const auto bit = static_cast<UINT8>(1 << i);
		if (!held('1' + i))
			continue;
		weapons |= bit;
		if (!(previousWeapons & bit))
			input.weapon = static_cast<WeaponType>(i);
	}
	previousWeapons = weapons;
	const bool interact = held('E');
	interactPending |= interact && !interactHeld;
	interactHeld = interact;

	POINT point = mPoint;
	if (ScreenToClient(window, &point)) {
		const auto aim = ScreenToWorld(point);
		input.aimX = aim.x;
		input.aimY = aim.y;
	}
}

Vector2D<float> PlayState::ScreenToWorld(POINT point) const
{
	const LONG width = screen.right - screen.left;
	const LONG height = screen.bottom - screen.top;
	if (width <= 0 || height <= 0)
		return camPos;
	return {
		camPos.x + (static_cast<float>(point.x - screen.left) / width * 2 - 1) * camSize.x,
		camPos.y + (static_cast<float>(point.y - screen.top) / height * 2 - 1) * camSize.y
	};
}

void PlayState::SendInput()
{
	const auto now = GetTickCount64();
	if (now - lastInputSend < 33)
		return;
	lastInputSend = now;

	auto* network = NetworkManager::getInstance();
	network->Send(&input, sizeof(input));
	input.keys = static_cast<InputKey>(input.keys & ~actionMask);
	input.weapon = WEAPON_KEEP;

	if (interactPending) {
		const C2S_INTERACT_PACKET packet{{sizeof(C2S_INTERACT_PACKET), C2S_INTERACT}};
		network->Send(&packet, sizeof(packet));
		interactPending = false;
	}
}
