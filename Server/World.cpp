#include "World.h"
#include "Bullet.h"
#include "ServerPlayer.h"
#include "ServerEnemy.h"
#include "ServerBoss.h"

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

void World::SpawnWave()
{
	// [S12] World::SpawnWave: 현재 스테이지·웨이브 표대로 적 생성
}

void World::UpdateStage()
{
	// [S12] World::UpdateStage: 다음 웨이브, 포탈 열기, 생존자 전원의 E키 상호작용으로 이동 판단
}

bool World::LoadStage(int nextStage)
{
	// [S12] World::LoadStage: 맵 교체, 플레이어 이동·부활, 보스 생성
	return false;
}

bool World::CheckGameEnd()
{
	// [S12] World::CheckGameEnd: 클리어·전멸·전원 이탈 판정, EndGame 호출
	return false;
}

void World::RemoveLeftPlayers()
{
	// [S12] World::RemoveLeftPlayers: room.lock을 잡고 이탈 목록을 꺼낸 뒤 월드의 해당 플레이어 제거
}

void World::PushEvent(const void* packet, int size)
{
	// [S13] World::PushEvent: Protocol.h 이벤트 패킷 바이트를 이번 틱 목록에 복사
}

void World::FlushEvents()
{
	// [S13] World::FlushEvents: 모은 이벤트를 Broadcast로 송신 큐에 넣고 목록 비우기
}

void World::SendSnapshot()
{
	// [S13] World::SendSnapshot: S2C_SNAPSHOT_PACKET에 틱·플레이어·적·보스 상태를 담아 Broadcast
	// enemyCount만큼의 가변 길이로 전송, room.lock을 잡은 채 Broadcast를 부르지 않는다
}
