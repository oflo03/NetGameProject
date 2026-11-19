#include "PlayState.h"
#include "NetworkManager.h"
#include <cmath>
#include <cstddef>
#include <cstring>
#include <unordered_set>

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

void PlayState::process_packet(const PacketHeader* packet)
{
	if (!packet || packet->size < sizeof(PacketHeader) || packet->size > MAX_PACKET_SIZE)
		return;

	// 고정 길이 패킷은 크기를 확인하고 복사한 뒤 전달
	const auto dispatch = [this, packet]<typename T>(void (PlayState::*handler)(const T&)) {
		if (packet->size != sizeof(T))
			return;
		T value{};
		std::memcpy(&value, packet, sizeof(value));
		(this->*handler)(value);
	};

	switch (packet->type) {
	case S2C_SNAPSHOT:
	{
		constexpr size_t baseSize = offsetof(S2C_SNAPSHOT_PACKET, enemies);
		if (packet->size < baseSize || packet->size > sizeof(S2C_SNAPSHOT_PACKET))
			return;
		S2C_SNAPSHOT_PACKET snapshot{};
		std::memcpy(&snapshot, packet, packet->size);
		if (snapshot.enemyCount > MAX_ENEMIES ||
			packet->size != baseSize + snapshot.enemyCount * sizeof(EnemySnap))
			return;
		OnSnapshot(snapshot);
		break;
	}
	case S2C_BULLET_SPAWN:
	{
		constexpr size_t baseSize = offsetof(S2C_BULLET_SPAWN_PACKET, bullets);
		if (packet->size < baseSize || packet->size > sizeof(S2C_BULLET_SPAWN_PACKET))
			return;
		S2C_BULLET_SPAWN_PACKET bullets{};
		std::memcpy(&bullets, packet, packet->size);
		if (bullets.count > MAX_SPAWN_BULLETS ||
			packet->size != baseSize + bullets.count * sizeof(BulletSpawn))
			return;
		OnBulletSpawn(bullets);
		break;
	}
	case S2C_BULLET_REMOVE: dispatch(&PlayState::OnBulletRemove); break;
	case S2C_ENEMY_SPAWN: dispatch(&PlayState::OnEnemySpawn); break;
	case S2C_ENEMY_DEAD: dispatch(&PlayState::OnEnemyDead); break;
	case S2C_PLAYER_EVENT: dispatch(&PlayState::OnPlayerEvent); break;
	case S2C_DROP: dispatch(&PlayState::OnDrop); break;
	case S2C_EFFECT: dispatch(&PlayState::OnEffect); break;
	case S2C_BOSS: dispatch(&PlayState::OnBoss); break;
	case S2C_PORTAL: dispatch(&PlayState::OnPortal); break;
	case S2C_STAGE_LOAD:
		if (packet->size == sizeof(S2C_STAGE_LOAD_PACKET)) {
			playerViews = {};
			enemyViews.clear();
			bossView = {};
			hasSnapshot = false;
			dispatch(&PlayState::OnStageLoad);
		}
		break;
	default:
		break;
	}
}

void PlayState::OnSnapshot(const S2C_SNAPSHOT_PACKET& packet)
{
	if (packet.enemyCount > MAX_ENEMIES)
		return;
	// 같은 틱이나 오래된 스냅샷으로 상태를 되돌리지 않음
	if (hasSnapshot) {
		const UINT32 distance = packet.tick - snapshotTick;
		if (distance == 0 || distance >= 0x80000000u)
			return;
	}
	const auto finitePosition = [](float x, float y) {
		return std::isfinite(x) && std::isfinite(y);
	};
	for (const auto& player : packet.players)
		if (player.used && (!finitePosition(player.x, player.y) || !std::isfinite(player.angle)))
			return;
	if (packet.boss.active && !finitePosition(packet.boss.x, packet.boss.y))
		return;
	if (packet.boss.active)
		for (int i = 0; i < 2; ++i)
			if (!finitePosition(packet.boss.handX[i], packet.boss.handY[i]))
				return;
	std::unordered_set<UINT16> ids;
	for (int i = 0; i < packet.enemyCount; ++i) {
		const auto& enemy = packet.enemies[i];
		if (!finitePosition(enemy.x, enemy.y) || !std::isfinite(enemy.angle) || !ids.insert(enemy.id).second)
			return;
	}

	const auto move = [](EntityView& view, float x, float y) {
		const Vector2D<float> position(x, y);
		view.previousPosition = view.hasPosition ? view.currentPosition : position;
		view.currentPosition = position;
		view.hasPosition = true;
	};
	for (int i = 0; i < MAX_PLAYERS; ++i) {
		auto& view = playerViews[i];
		if (!packet.players[i].used)
			view = {};
		else
			move(view, packet.players[i].x, packet.players[i].y);
		view.snapshot = packet.players[i];
	}
	for (auto it = enemyViews.begin(); it != enemyViews.end(); ) {
		if (!ids.contains(it->first))
			it = enemyViews.erase(it);
		else
			++it;
	}
	for (int i = 0; i < packet.enemyCount; ++i) {
		const auto& enemy = packet.enemies[i];
		auto& view = enemyViews[enemy.id];
		move(view, enemy.x, enemy.y);
		view.snapshot = enemy;
	}
	if (!packet.boss.active)
		bossView = {};
	else
		move(bossView, packet.boss.x, packet.boss.y);
	bossView.snapshot = packet.boss;
	snapshotTick = packet.tick;
	hasSnapshot = true;
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
