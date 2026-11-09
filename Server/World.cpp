#include "World.h"
#include "Bullet.h"
#include "ServerPlayer.h"
#include "ServerEnemy.h"

void World::Tick(float deltaTime)
{
	// [S9] World::Tick: 한 틱 실행 (S10 → S11 → S12 → S13)
}

void World::UpdatePlayers(float deltaTime)
{
	// [S10] World::UpdatePlayers: 플레이어마다 TakeInput으로 입력을 가져와 적용
}

Bullet* World::SpawnBullet(std::uint8_t type, std::uint8_t side, Vector2D<float> pos, Vector2D<float> dir)
{
	// [S10] World::SpawnBullet: 총알 ID 발급, 종류별 상태 설정, 총알 생성 이벤트 준비
	return nullptr;
}

void World::UpdateBullets(float deltaTime)
{
	// [S10] World::UpdateBullets: 총알 이동, 이동 구간을 나눠 벽 통과 방지
}

void World::ResolveCollisions()
{
	// [S10] World::ResolveCollisions: 벽, 총알, 드랍 충돌과 피격 판정
}

void World::SwordSlash(ServerPlayer& player)
{
	// [S10] World::SwordSlash: 칼 공격 범위 판정, 기사 총알 반사
}

void World::DamagePlayer(ServerPlayer& player, int damage)
{
	// [S10] World::DamagePlayer: 플레이어 피격, 사망 처리
}

void World::UpdateEnemies(float deltaTime)
{
	// [S11] World::UpdateEnemies: 적 타깃 선택, Behavior Tree 실행, 이동
}

ServerPlayer* World::FindNearestPlayer(const Vector2D<float>& pos) const
{
	// [S11] World::FindNearestPlayer: 가장 가까운 생존 플레이어 찾기
	return nullptr;
}

void World::DamageEnemy(ServerEnemy& enemy, int damage)
{
	// [S11] World::DamageEnemy: 적 피격, 사망, 드랍 생성
}
