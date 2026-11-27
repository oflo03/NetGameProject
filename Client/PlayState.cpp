#include "PlayState.h"
#include "GameData.h"
#include "Map.h"
#include "NetworkManager.h"
#include "Sound.h"
#include "UI.h"
#include "Weapon.h"
#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstring>
#include <unordered_set>

namespace {
	constexpr UINT8 actionMask = KEY_ROLL | KEY_WEAPON_PREV | KEY_WEAPON_NEXT;
	constexpr int worldWidth = MapWidth * PTS;
	constexpr int worldHeight = MapHeight * PTS;

	bool IsEffectPosition(float x, float y)
	{
		return x >= -PTS && x <= worldWidth + PTS && y >= -PTS && y <= worldHeight + PTS;
	}
}

PlayState::PlayState(int localPlayerId, const S2C_ROOM_INFO_PACKET& roomInfo)
	: localPlayerId(localPlayerId), roomInfo(roomInfo)
{
	if (MapManager::getInstance()->LoadTileMap(1, false))
		stage = 1;
	camPos = { worldWidth / 2.0f, worldHeight / 2.0f };
}

void PlayState::update()
{
	for (auto& bullet : bulletViews)
		bullet.second.update();
	if (portalVisible)
		Portal::getInstance()->update();
	for (auto& effect : effects)
		effect.update();
	std::erase_if(effects, [](const Effect& effect) { return effect.time >= Effect::effect[effect.type].frame; });
	SoundManager::getInstance()->update();

	const float elapsed = static_cast<float>(GetTickCount64() - snapshotReceivedAt);
	const float alpha = (std::min)(elapsed / GameData::LogicIntervalMs, 1.0f);
	for (auto& [id, view] : enemyViews) {
		view.interpolation = alpha;
		view.update();
	}
	std::erase_if(enemyViews, [](const auto& entry) { return entry.second.finished; });

	if (!hasSnapshot)
		return;

	for (int i = 0; i < MAX_PLAYERS; ++i) {
		auto& view = playerViews[i];
		if (!view.snapshot.used)
			continue;
		const auto& member = roomInfo.members[i];
		if (member.used) {
			view.charType = member.charType;
			std::memcpy(view.nickname, member.name, sizeof(member.name));
		}
		view.interpolation = alpha;
		view.update();
	}
	UpdateCamera();
	for (const auto& event : playerEvents)
		OnPlayerEvent(event);
	playerEvents.clear();
}

void PlayState::UpdateCamera()
{
	const auto isAlive = [](const PlayerView& view) {
		return view.snapshot.used && view.snapshot.hp > 0 && view.snapshot.state != STATE_DEAD;
	};
	const PlayerView* target = nullptr;
	if (localPlayerId >= 0 && localPlayerId < MAX_PLAYERS &&
		isAlive(playerViews[localPlayerId]))
		target = &playerViews[localPlayerId];
	else {
		const auto survivor = std::find_if(playerViews.begin(), playerViews.end(), isAlive);
		if (survivor != playerViews.end())
			target = &*survivor;
	}
	if (target)
		camPos = target->renderPosition;
}

void PlayState::handle_events()
{
	SampleInput();
	SendInput();
}

void PlayState::draw()
{
	const int width = screen.right - screen.left;
	const int height = screen.bottom - screen.top;
	if (!mDC || width <= 0 || height <= 0 || camSize.x <= 0 || camSize.y <= 0)
		return;

	FillRect(mDC, &screen, static_cast<HBRUSH>(GetStockObject(BLACK_BRUSH)));
	const HDC worldDC = CreateCompatibleDC(mDC);
	if (!worldDC)
		return;
	const HBITMAP worldBitmap = CreateCompatibleBitmap(mDC, worldWidth, worldHeight);
	if (!worldBitmap) {
		DeleteDC(worldDC);
		return;
	}
	const auto previousBitmap = SelectObject(worldDC, worldBitmap);
	PatBlt(worldDC, 0, 0, worldWidth, worldHeight, BLACKNESS);
	MapManager::getInstance()->PrintMap(worldDC);
	for (auto& [id, view] : dropViews)
		view.draw(worldDC);
	if (portalVisible) {
		auto* portal = Portal::getInstance();
		const int id = localPlayerId >= 0 ? localPlayerId : NetworkManager::getInstance()->playerId;
		const Vector2D<float>* hintPosition = nullptr;
		if (id >= 0 && id < MAX_PLAYERS) {
			const auto& view = playerViews[id];
			if (view.snapshot.used && view.snapshot.hp > 0 &&
				(view.renderPosition - portal->GetPos()).GetLenth() < Portal::UseRange)
				hintPosition = &view.renderPosition;
		}
		portal->Draw(worldDC, hintPosition);
	}

	struct DrawEntry {
		float y;
		PlayerView* player;
		EnemyView* enemy;
	};
	std::vector<DrawEntry> visible;
	visible.reserve(MAX_PLAYERS + enemyViews.size());
	for (auto& view : playerViews) {
		const auto& pos = view.renderPosition;
		if (view.snapshot.used && IsEffectPosition(pos.x, pos.y))
			visible.push_back({ pos.y, &view, nullptr });
	}
	for (auto& [id, view] : enemyViews) {
		const auto& pos = view.renderPosition;
		if (view.hasType && IsEffectPosition(pos.x, pos.y))
			visible.push_back({ pos.y, nullptr, &view });
	}
	std::stable_sort(visible.begin(), visible.end(), [](const DrawEntry& a, const DrawEntry& b) {
		return a.y < b.y;
	});
	for (const auto& entry : visible) {
		if (entry.player)
			entry.player->draw(worldDC);
		else
			entry.enemy->draw(worldDC);
	}
	for (auto& bullet : bulletViews)
		bullet.second.draw(worldDC);
	for (auto& effect : effects)
		effect.Draw(worldDC);

	// 카메라가 맵 바깥을 보는 부분은 검은색으로 둠
	const float left = camPos.x - camSize.x;
	const float top = camPos.y - camSize.y;
	const int srcLeft = static_cast<int>(std::clamp(left, 0.0f, static_cast<float>(worldWidth)));
	const int srcTop = static_cast<int>(std::clamp(top, 0.0f, static_cast<float>(worldHeight)));
	const int srcRight = static_cast<int>(std::clamp(camPos.x + camSize.x, 0.0f, static_cast<float>(worldWidth)));
	const int srcBottom = static_cast<int>(std::clamp(camPos.y + camSize.y, 0.0f, static_cast<float>(worldHeight)));
	if (srcLeft < srcRight && srcTop < srcBottom) {
		const float scaleX = width / (2 * camSize.x);
		const float scaleY = height / (2 * camSize.y);
		const int dstLeft = static_cast<int>(std::lround((srcLeft - left) * scaleX));
		const int dstRight = static_cast<int>(std::lround((srcRight - left) * scaleX));
		const int dstTop = static_cast<int>(std::lround((srcTop - top) * scaleY));
		const int dstBottom = static_cast<int>(std::lround((srcBottom - top) * scaleY));
		StretchBlt(mDC, screen.left + dstLeft, screen.top + dstTop, dstRight - dstLeft, dstBottom - dstTop,
			worldDC, srcLeft, srcTop, srcRight - srcLeft, srcBottom - srcTop, SRCCOPY);
	}

	SelectObject(worldDC, previousBitmap);
	DeleteObject(worldBitmap);
	DeleteDC(worldDC);

	if (localPlayerId >= 0 && localPlayerId < MAX_PLAYERS && playerViews[localPlayerId].snapshot.used)
		UI::getInstance()->draw(mDC, playerViews[localPlayerId].snapshot);
	const HWND window = GetActiveWindow();
	POINT point = mPoint;
	if (window && ScreenToClient(window, &point) && !cursor.IsNull())
		cursor.Draw(mDC, point.x - 20, point.y - 30, 40, 40);
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
	case S2C_STAGE_LOAD: dispatch(&PlayState::OnStageLoad); break;
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
		if (!it->second.dying && !ids.contains(it->first))
			it = enemyViews.erase(it);
		else
			++it;
	}
	for (int i = 0; i < packet.enemyCount; ++i) {
		const auto& enemy = packet.enemies[i];
		auto& view = enemyViews[enemy.id];
		if (view.dying)
			continue;
		move(view, enemy.x, enemy.y);
		view.snapshot = enemy;
	}
	if (!packet.boss.active)
		bossView = {};
	else
		move(bossView, packet.boss.x, packet.boss.y);
	bossView.snapshot = packet.boss;
	snapshotTick = packet.tick;
	snapshotReceivedAt = GetTickCount64();
	hasSnapshot = true;
}

void PlayState::OnBulletSpawn(const S2C_BULLET_SPAWN_PACKET& packet)
{
	constexpr int shotSounds[] = { PISTOL_SHOT, RIFLE_SHOT, SHOTGUN_SHOT };
	bool played[BROCKET + 1]{};
	for (int i = 0; i < packet.count; ++i) {
		const auto& spawn = packet.bullets[i];
		if (spawn.type >= std::size(GameData::Bullets) ||
			(spawn.side != playerBullet && spawn.side != enemyBullet) ||
			!IsEffectPosition(spawn.x, spawn.y) || !std::isfinite(spawn.dirX) || !std::isfinite(spawn.dirY))
			continue;

		Vector2D<float> direction(spawn.dirX, spawn.dirY);
		if (spawn.type != BOSSBULLET1) {
			const double length = std::hypot(static_cast<double>(spawn.dirX), static_cast<double>(spawn.dirY));
			if (length == 0)
				continue;
			direction = { static_cast<float>(spawn.dirX / length), static_cast<float>(spawn.dirY / length) };
		} else if (!IsEffectPosition(spawn.dirX, spawn.dirY)) {
			continue;
		}
		auto [it, inserted] = bulletViews.try_emplace(spawn.id);
		if (!inserted)
			continue;
		it->second.spawn = spawn;
		it->second.position = { spawn.x, spawn.y };
		it->second.direction = direction;
		if (!played[spawn.type] && spawn.side == playerBullet) {
			if (spawn.type <= BSHOTGUN)
				SoundManager::getInstance()->play(shotSounds[spawn.type]);
			else if (spawn.type == BROCKET)
				SoundManager::getInstance()->play(ROCKET_SHOT);
			played[spawn.type] = true;
		}
	}
}

void PlayState::OnBulletRemove(const S2C_BULLET_REMOVE_PACKET& packet)
{
	const auto it = bulletViews.find(packet.id);
	if (it == bulletViews.end())
		return;
	const UINT8 type = it->second.spawn.type;
	bulletViews.erase(it);
	if (!packet.hit)
		return;
	constexpr CEffect hits[] = { PISTOLBULLET, RIFLEBULLET, SHOTGUNBULLET, ENEMYBULLET, BOUNCEBULLET,
		PATTERNA, PATTERNB, PATTERNA, PATTERNB, EXPLOSION };
	AddEffect(hits[type], { packet.x, packet.y });
}

void PlayState::OnEnemySpawn(const S2C_ENEMY_SPAWN_PACKET& packet)
{
	if (packet.type > ENEMY_BOMBSHE || !IsEffectPosition(packet.x, packet.y))
		return;
	auto& view = enemyViews[packet.id];
	if (view.hasType && !view.dying)
		return;
	if (view.dying)
		view = {};

	view.type = packet.type;
	view.hasType = true;
	view.spawnTime = GameData::EnemySpawnSeconds;
	if (!view.hasPosition) {
		view.snapshot.id = packet.id;
		view.previousPosition = view.currentPosition = { packet.x, packet.y };
		view.hasPosition = true;
	}
	view.renderPosition = view.currentPosition;
}

void PlayState::OnEnemyDead(const S2C_ENEMY_DEAD_PACKET& packet)
{
	const auto it = enemyViews.find(packet.id);
	if (it == enemyViews.end() || !it->second.hasType || it->second.dying)
		return;
	it->second.dying = true;
	it->second.spawnTime = 0.0f;
	it->second.animationState = -1;
	SoundManager::getInstance()->play(ENEMYDEATH);
}

void PlayState::OnPlayerEvent(const S2C_PLAYER_EVENT_PACKET& packet)
{
	if (packet.playerId >= MAX_PLAYERS || packet.event > PE_REVIVE)
		return;
	if (!hasSnapshot) {
		playerEvents.push_back(packet);
		return;
	}
	const auto& view = playerViews[packet.playerId];
	if (!view.hasPosition)
		return;
	const auto position = view.currentPosition;
	auto* sound = SoundManager::getInstance();
	switch (packet.event) {
	case PE_HIT:
		AddEffect(DAMAGED, position);
		sound->play(PLAYER_HURT);
		break;
	case PE_DEAD:
		sound->play(PLAYER_DEATH);
		break;
	case PE_SWORD:
		AddEffect(SWORDATTACK, position);
		sound->play(SWORD_ATK);
		break;
	case PE_PICKUP:
		sound->play(CURSORON);
		break;
	case PE_REVIVE:
		AddEffect(TELEPORT, position);
		break;
	default:
		break;
	}
}

void PlayState::AddEffect(CEffect type, Vector2D<float> position)
{
	if (!IsEffectPosition(position.x, position.y))
		return;
	EffectManager::getInstance();
	effects.emplace_back(type, position);
}

void PlayState::OnDrop(const S2C_DROP_PACKET& packet)
{
	if (packet.add == 0)
		dropViews.erase(packet.id);
	else if (packet.add == 1 && packet.type <= DROCKET && IsEffectPosition(packet.x, packet.y))
		dropViews.insert_or_assign(packet.id, DropView(packet));
}

void PlayState::OnEffect(const S2C_EFFECT_PACKET& packet)
{
	static_assert(GameData::EFFECT_BANSHEE_START == static_cast<int>(BOMBSHE1));
	static_assert(GameData::EFFECT_BANSHEE_LOOP == static_cast<int>(BOMBSHE2));
	static_assert(GameData::EFFECT_BAT_EXPLOSION == static_cast<int>(BATDIE));
	if (packet.type >= EFFECT_AMOUNT || !IsEffectPosition(packet.x, packet.y))
		return;

	const auto type = static_cast<CEffect>(packet.type);
	AddEffect(type, { packet.x, packet.y });
	switch (type) {
	case PATTERNA: SoundManager::getInstance()->play(BOSSLAZOR); break;
	case PATTERNB: SoundManager::getInstance()->play(BOSSLIGHTNING); break;
	case PATTERND: SoundManager::getInstance()->play(BOSSCHARGING); break;
	default: break;
	}
}

void PlayState::OnBoss(const S2C_BOSS_PACKET& packet)
{
	// [C6] OnBoss: 보스 패턴 연출
}

void PlayState::OnPortal(const S2C_PORTAL_PACKET& packet)
{
	if (!IsEffectPosition(packet.x, packet.y))
		return;
	Portal::getInstance()->SetPos({ packet.x, packet.y });
	portalVisible = true;
}

void PlayState::OnStageLoad(const S2C_STAGE_LOAD_PACKET& packet)
{
	if (stage == packet.stage || !MapManager::getInstance()->LoadTileMap(packet.stage, false))
		return;

	stage = packet.stage;
	playerViews = {};
	enemyViews.clear();
	bossView = {};
	bulletViews.clear();
	dropViews.clear();
	effects.clear();
	playerEvents.clear();
	portalVisible = false;
	hasSnapshot = false;
	const auto spawn = GameData::PlayerSpawns[stage - 1];
	camPos = { spawn.x, spawn.y };
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
