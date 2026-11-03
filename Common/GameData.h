#pragma once
#include <cstdint>
#include <span>

namespace GameData
{
	// 원작 WM_TIMER(10ms)에서 프레임마다 적용하던 값의 환산 기준
	inline constexpr float OriginalUpdatesPerSecond = 100.0f;

	struct WeaponStats
	{
		float cooldownSeconds;
		int maxAmmo;
		int projectileCount;
		int halfSpreadDegrees;
	};

	// Client/Weapon.h의 ID 순서. 샷건은 -10도부터 +10도까지 등간격 발사.
	inline constexpr WeaponStats Weapons[] = {
		{ 30 / OriginalUpdatesPerSecond, 1, 0, 0 },	// SWORD
		{ 15 / OriginalUpdatesPerSecond, 20, 1, 5 },	// PISTOL
		{ 7 / OriginalUpdatesPerSecond, 35, 1, 7 },	// RIFLE
		{ 30 / OriginalUpdatesPerSecond, 10, 5, 10 },	// SHOTGUN
		{ 10 / OriginalUpdatesPerSecond, 2, 1, 0 },	// ROCKET
	};
	inline constexpr int SwordDamage = 10;
	inline constexpr float SwordRange = 110.0f;
	inline constexpr float SwordHalfAngleDegrees = 60.0f;
	inline constexpr float RocketBlastRadius = 100.0f;
	inline constexpr int RocketBlastDamage = 20;

	struct BulletStats
	{
		int damage;
		float radius;
		float speedPerSecond;
	};

	// Client/Weapon.h의 bullet_table 순서. 직선 속도는 원작 dir 길이 × velocity × 초당 갱신 횟수.
	inline constexpr BulletStats Bullets[] = {
		{ 10, 8.0f, 20.0f * OriginalUpdatesPerSecond },	// BPISTOL
		{ 7, 8.0f, 20.0f * OriginalUpdatesPerSecond },	// BRIFLE
		{ 8, 8.0f, 20.0f * OriginalUpdatesPerSecond },	// BSHOTGUN
		{ 5, 8.0f, 5.0f * OriginalUpdatesPerSecond },	// BENEMY
		{ 5, 8.0f, 5.0f * OriginalUpdatesPerSecond },	// BBOUNCE
		{ 80, 10.0f, 0.0f },							// BOSSBULLET1: 중심 기준 회전
		{ 10, 10.0f, 20.0f * OriginalUpdatesPerSecond },	// BOSSBULLET2
		{ 10, 10.0f, 8.0f * OriginalUpdatesPerSecond },	// BOSSBULLET3
		{ 10, 10.0f, 20.0f * OriginalUpdatesPerSecond },	// BOUNCEDBOSSBULLET2
		{ 100, 8.0f, 10.0f * OriginalUpdatesPerSecond },	// BROCKET
	};
	inline constexpr float OrbitBulletDegreesPerSecond = 2.0f * OriginalUpdatesPerSecond;

	struct MapPosition
	{
		float x;
		float y;
	};

	// Client/Enemy.h의 enemytype과 같은 값
	enum class EnemyKind : std::uint8_t
	{
		Pistolman = 0, Rifleman = 1, Shotgunman = 2, Bat = 3, Bombshe = 4
	};

	struct EnemyStats
	{
		int hp;
		float moveSpeedPerSecond;
		float attackRange;
		float halfWidth;
		float halfHeight;
		int dropType;	// Client/Weapon.h의 DROP 값. -1은 드랍 없음.
	};

	// EnemyKind 순서. 박쥐의 절반 크기는 원형 충돌 반지름이다.
	inline constexpr EnemyStats Enemies[] = {
		{ 50, 100.0f, 400.0f, 14.0f, 25.0f, 0 },	// Pistolman
		{ 50, 100.0f, 400.0f, 16.0f, 25.0f, 1 },	// Rifleman
		{ 100, 100.0f, 400.0f, 14.0f, 31.0f, 2 },	// Shotgunman
		{ 10, 200.0f, 50.0f, 10.0f, 10.0f, -1 },	// Bat
		{ 70, 100.0f, 200.0f, 20.0f, 31.0f, 3 },	// Bombshe: 속도는 충돌 보정에 사용
	};
	inline constexpr int GunnerAmmoDivisor = 3;
	inline constexpr float GunnerCooldownMultiplier = 2.0f;
	inline constexpr float GunnerFirstShotDelaySeconds = 100 / OriginalUpdatesPerSecond;
	inline constexpr float GunnerReloadSeconds = 300 / OriginalUpdatesPerSecond;
	inline constexpr float BatBlastRadius = 100.0f;
	inline constexpr float BombsheWaveMaxRadius = 100.0f;
	inline constexpr float BombsheWaveGrowthPerSecond = 1.0f * OriginalUpdatesPerSecond;
	inline constexpr int BossHp = 1000;
	inline constexpr float BossRadius = 50.0f;

	// 스테이지 1~4의 기준 위치. 배열 인덱스는 stage - 1.
	inline constexpr MapPosition PlayerSpawns[] = {
		{ 224, 256 }, { 960, 256 }, { 224, 540 }, { 960, 1000 },
	};
	inline constexpr MapPosition PortalSpawns[] = {
		{ 1633, 256 }, { 960, 900 }, { 960, 540 }, { 960, 540 },
	};
	inline constexpr MapPosition BossSpawn{ 960, 500 };

	struct EnemySpawn
	{
		EnemyKind kind;
		MapPosition position;
	};

	// 원작 EnemyManager::SetEnemy의 웨이브 1~9
	inline constexpr EnemySpawn Wave1[] = {
		{ EnemyKind::Pistolman, { 300, 400 } }, { EnemyKind::Pistolman, { 1000, 400 } },
		{ EnemyKind::Pistolman, { 1800, 400 } },
	};
	inline constexpr EnemySpawn Wave2[] = {
		{ EnemyKind::Pistolman, { 300, 400 } }, { EnemyKind::Pistolman, { 1000, 400 } },
		{ EnemyKind::Pistolman, { 1800, 400 } }, { EnemyKind::Pistolman, { 300, 900 } },
		{ EnemyKind::Pistolman, { 1000, 900 } }, { EnemyKind::Pistolman, { 1800, 900 } },
	};
	inline constexpr EnemySpawn Wave3[] = {
		{ EnemyKind::Rifleman, { 300, 400 } }, { EnemyKind::Pistolman, { 1000, 400 } },
		{ EnemyKind::Pistolman, { 1800, 400 } }, { EnemyKind::Pistolman, { 300, 900 } },
		{ EnemyKind::Pistolman, { 1000, 900 } }, { EnemyKind::Rifleman, { 1800, 900 } },
	};
	inline constexpr EnemySpawn Wave4[] = {
		{ EnemyKind::Pistolman, { 600, 200 } }, { EnemyKind::Pistolman, { 1500, 200 } },
		{ EnemyKind::Rifleman, { 300, 600 } }, { EnemyKind::Rifleman, { 1800, 600 } },
		{ EnemyKind::Shotgunman, { 760, 980 } },
	};
	inline constexpr EnemySpawn Wave5[] = {
		{ EnemyKind::Bat, { 300, 200 } }, { EnemyKind::Bat, { 1600, 200 } },
		{ EnemyKind::Bat, { 600, 600 } }, { EnemyKind::Bat, { 1500, 600 } },
	};
	inline constexpr EnemySpawn Wave6[] = {
		{ EnemyKind::Shotgunman, { 600, 200 } }, { EnemyKind::Shotgunman, { 1300, 200 } },
		{ EnemyKind::Bat, { 300, 600 } }, { EnemyKind::Bat, { 1800, 600 } },
		{ EnemyKind::Shotgunman, { 100, 970 } }, { EnemyKind::Shotgunman, { 1800, 970 } },
		{ EnemyKind::Bat, { 300, 1000 } }, { EnemyKind::Bat, { 1800, 1000 } },
	};
	inline constexpr EnemySpawn Wave7[] = {
		{ EnemyKind::Bombshe, { 400, 600 } }, { EnemyKind::Bombshe, { 1500, 600 } },
		{ EnemyKind::Pistolman, { 800, 300 } }, { EnemyKind::Pistolman, { 1000, 300 } },
		{ EnemyKind::Rifleman, { 800, 800 } }, { EnemyKind::Rifleman, { 1000, 800 } },
	};
	inline constexpr EnemySpawn Wave8[] = {
		{ EnemyKind::Shotgunman, { 800, 300 } }, { EnemyKind::Shotgunman, { 1000, 300 } },
		{ EnemyKind::Shotgunman, { 800, 800 } }, { EnemyKind::Shotgunman, { 1000, 800 } },
		{ EnemyKind::Shotgunman, { 300, 200 } }, { EnemyKind::Shotgunman, { 1600, 200 } },
		{ EnemyKind::Shotgunman, { 300, 600 } }, { EnemyKind::Shotgunman, { 1800, 600 } },
	};
	inline constexpr EnemySpawn Wave9[] = {
		{ EnemyKind::Pistolman, { 800, 300 } }, { EnemyKind::Pistolman, { 1000, 300 } },
		{ EnemyKind::Rifleman, { 800, 800 } }, { EnemyKind::Rifleman, { 1000, 800 } },
		{ EnemyKind::Shotgunman, { 300, 200 } }, { EnemyKind::Shotgunman, { 1600, 200 } },
		{ EnemyKind::Bat, { 300, 600 } }, { EnemyKind::Bat, { 1800, 600 } },
	};

	// 스테이지 1~3의 [stage - 1][phase - 1]. 스테이지 4는 보스가 별도로 소환한다.
	inline constexpr std::span<const EnemySpawn> StageWaves[3][3] = {
		{ Wave1, Wave2, Wave3 },
		{ Wave4, Wave5, Wave6 },
		{ Wave7, Wave8, Wave9 },
	};
	inline constexpr EnemySpawn BossSummons[] = {
		{ EnemyKind::Pistolman, { 260, 310 } }, { EnemyKind::Rifleman, { 1700, 310 } },
		{ EnemyKind::Shotgunman, { 260, 780 } }, { EnemyKind::Bombshe, { 950, 880 } },
		{ EnemyKind::Bat, { 1400, 780 } },
	};
}
