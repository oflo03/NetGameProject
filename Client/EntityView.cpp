#include "EntityView.h"

Vector2D<float> EntityView::Interpolate(float alpha) const
{
	// [C5] Interpolate: 두 스냅샷 사이 위치 보간
	return {};
}

void PlayerView::update()
{
	// [C5] update: 플레이어 방향과 애니메이션 갱신
}

void PlayerView::draw(HDC dc)
{
	// [C9] draw: 플레이어 몸, 무기, 닉네임 그리기
}

void EnemyView::update()
{
	// [C5] update: 적 애니메이션 갱신
}

void EnemyView::draw(HDC dc)
{
	// [C9] draw: 적 그리기
}

void BossView::update()
{
	// [C5] update: 보스 애니메이션 갱신
}

void BossView::draw(HDC dc)
{
	// [C9] draw: 보스 본체와 손 그리기
}
