#include "ServerEnemy.h"
#include "World.h"
#include "Bullet.h"
#include <cmath>
#include <random>

namespace
{
	using Status = BT::Status;

	template<class Function>
	std::unique_ptr<BT::Condition> Check(Function function)
	{
		auto node = std::make_unique<BT::Condition>();
		node->predicate = function;
		return node;
	}

	template<class Function>
	std::unique_ptr<BT::Action> Do(Function function)
	{
		auto node = std::make_unique<BT::Action>();
		node->action = function;
		return node;
	}

	float Distance(Vector2D<float> a, Vector2D<float> b)
	{
		return std::hypot(a.x - b.x, a.y - b.y);
	}

	bool HasTarget(World& world, ServerEnemy& enemy)
	{
		return world.FindNearestPlayer(enemy.pos) != nullptr;
	}

	bool InRange(World& world, ServerEnemy& enemy)
	{
		const auto* target = world.FindNearestPlayer(enemy.pos);
		return target && Distance(target->pos, enemy.pos) <= enemy.attackRange;
	}

	Status Chase(World& world, ServerEnemy& enemy, float)
	{
		const auto* target = world.FindNearestPlayer(enemy.pos);
		if (!target)
			return Status::BT_FAILURE;
		const auto offset = target->pos - enemy.pos;
		const float length = std::hypot(offset.x, offset.y);
		enemy.dir = length > 0 ? offset / length : Vector2D<float>();
		enemy.state = GameData::PLAYER_RUN;
		return Status::BT_RUNNING;
	}
}

std::unique_ptr<BT::Node> BuildGunnerTree()
{
	// [S11] BuildGunnerTree: 사거리 안에서는 사격·배회, 밖에서는 추적한다.
	auto root = std::make_unique<BT::Sequence>();
	root->children.push_back(Check(HasTarget));
	auto choices = std::make_unique<BT::Selector>();
	auto fire = std::make_unique<BT::Sequence>();
	fire->children.push_back(Check(InRange));
	fire->children.push_back(Do([](World& world, ServerEnemy& enemy, float) {
		static thread_local std::mt19937 random(std::random_device{}());
		if (enemy.moveTime <= 0) {
			enemy.moveTime = std::uniform_int_distribution<int>(20, 70)(random) / GameData::OriginalUpdatesPerSecond;
			const bool moving = std::uniform_int_distribution<int>(0, 1)(random) != 0;
			enemy.state = moving ? GameData::PLAYER_RUN : GameData::PLAYER_IDLE;
			enemy.dir = moving ? Vector2D<float>(1, 0).Rotate(static_cast<float>(std::uniform_int_distribution<int>(0, 359)(random))) : Vector2D<float>();
		}
		const auto& stats = GameData::Weapons[enemy.weapon];
		if (enemy.attackCooldown > 0)
			return Status::BT_RUNNING;
		if (enemy.reloading) {
			enemy.ammo = stats.maxAmmo / GameData::GunnerAmmoDivisor;
			enemy.reloading = false;
		}
		const auto* target = world.FindNearestPlayer(enemy.pos);
		const auto hand = enemy.pos + Vector2D<float>(std::abs(enemy.angle) < 90 ? 18.0f : -17.0f, 8.0f);
		const auto offset = target->pos - hand;
		const float length = std::hypot(offset.x, offset.y);
		if (length == 0)
			return Status::BT_RUNNING;
		for (int pellet = 0; pellet < stats.projectileCount; ++pellet) {
			const float degrees = stats.projectileCount == 1 ?
				static_cast<float>(std::uniform_int_distribution<int>(-stats.halfSpreadDegrees, stats.halfSpreadDegrees)(random)) :
				static_cast<float>(-stats.halfSpreadDegrees + pellet * 2 * stats.halfSpreadDegrees / (stats.projectileCount - 1));
			auto direction = offset / length;
			direction.Rotate(degrees);
			const auto muzzle = hand + direction * 30.0f + Vector2D<float>(0, -10);
			if (auto* bullet = world.SpawnBullet(GameData::BULLET_ENEMY, GameData::BULLET_SIDE_ENEMY, muzzle, direction * 10.0f))
				bullet->lastPos = enemy.pos;
		}
		--enemy.ammo;
		enemy.reloading = enemy.ammo == 0;
		enemy.attackCooldown = enemy.reloading ? GameData::GunnerReloadSeconds : stats.cooldownSeconds * GameData::GunnerCooldownMultiplier;
		return Status::BT_RUNNING;
	}));
	choices->children.push_back(std::move(fire));
	choices->children.push_back(Do(Chase));
	root->children.push_back(std::move(choices));
	return root;
}

std::unique_ptr<BT::Node> BuildBatTree()
{
	// [S11] BuildBatTree: 발견 후 계속 추적하고 근접하면 자폭을 준비한다.
	auto root = std::make_unique<BT::Sequence>();
	root->children.push_back(Check(HasTarget));
	auto choices = std::make_unique<BT::Selector>();
	auto explode = std::make_unique<BT::Sequence>();
	explode->children.push_back(Check(InRange));
	explode->children.push_back(Do([](World&, ServerEnemy& enemy, float) {
		if (!enemy.attackStarted) {
			enemy.attackStarted = true;
			enemy.actionTime = GameData::BatFuseSeconds;
			enemy.state = 1; // 원작 Bat::DEAD
		}
		enemy.dir = {};
		return Status::BT_RUNNING;
	}));
	choices->children.push_back(std::move(explode));
	choices->children.push_back(Do([](World& world, ServerEnemy& enemy, float) {
		const auto* target = world.FindNearestPlayer(enemy.pos);
		if (Distance(target->pos, enemy.pos) <= GameData::BatDetectionRange)
			enemy.targetLocked = true;
		if (enemy.targetLocked && !enemy.attackStarted)
			Chase(world, enemy, 0);
		else
			enemy.dir = {};
		enemy.state = enemy.attackStarted ? 1 : 0;
		return Status::BT_RUNNING;
	}));
	root->children.push_back(std::move(choices));
	return root;
}

std::unique_ptr<BT::Node> BuildBansheeTree()
{
	// [S11] BuildBansheeTree: 사거리 안에 들어오면 제자리에서 음파를 유지한다.
	auto root = std::make_unique<BT::Sequence>();
	root->children.push_back(Check([](World& world, ServerEnemy& enemy) {
		return enemy.attackStarted || InRange(world, enemy);
	}));
	root->children.push_back(Do([](World&, ServerEnemy& enemy, float) {
		if (!enemy.attackStarted) {
			enemy.attackStarted = true;
			enemy.actionTime = 0;
		}
		enemy.state = 1; // 원작 Bombshe::BATTACK
		enemy.dir = {};
		return Status::BT_RUNNING;
	}));
	return root;
}
