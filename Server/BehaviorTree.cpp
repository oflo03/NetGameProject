#include "BehaviorTree.h"

BT::Status BT::Selector::Tick(World& world, ServerEnemy& enemy, float deltaTime)
{
	// [S11] Selector::Tick: 자식을 순서대로 실행, 성공하거나 실행 중인 자식의 결과 반환
	return Status::BT_FAILURE;
}

BT::Status BT::Sequence::Tick(World& world, ServerEnemy& enemy, float deltaTime)
{
	// [S11] Sequence::Tick: 자식을 순서대로 실행, 실패하거나 실행 중이면 중단
	return Status::BT_FAILURE;
}

BT::Status BT::Condition::Tick(World& world, ServerEnemy& enemy, float deltaTime)
{
	// [S11] Condition::Tick: 조건 콜백 결과를 성공 또는 실패로 반환
	return Status::BT_FAILURE;
}

BT::Status BT::Action::Tick(World& world, ServerEnemy& enemy, float deltaTime)
{
	// [S11] Action::Tick: 행동 콜백 실행, 성공·실패·실행 중 결과 반환
	return Status::BT_FAILURE;
}
