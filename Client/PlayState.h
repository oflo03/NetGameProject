#pragma once
#include "Framework.h"

// 서버 상태를 표시하는 온라인 플레이 화면
class PlayState : public GameState
{
public:
	void update() override;
	void handle_events() override;
	void draw() override;
};
