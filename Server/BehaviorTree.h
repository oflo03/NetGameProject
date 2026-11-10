#pragma once
#include <cstdint>
#include <functional>
#include <memory>
#include <vector>

class World;
class ServerEnemy;

namespace BT
{
	enum class Status : std::uint8_t
	{
		BT_SUCCESS, BT_FAILURE, BT_RUNNING
	};

	class Node
	{
	public:
		virtual ~Node() = default;
		virtual Status Tick(World& world, ServerEnemy& enemy, float deltaTime) = 0;
	};

	class Selector : public Node
	{
	public:
		std::vector<std::unique_ptr<Node>> children;
		Status Tick(World& world, ServerEnemy& enemy, float deltaTime) override;
	};

	class Sequence : public Node
	{
	public:
		std::vector<std::unique_ptr<Node>> children;
		Status Tick(World& world, ServerEnemy& enemy, float deltaTime) override;
	};

	class Condition : public Node
	{
	public:
		std::function<bool(World&, ServerEnemy&)> predicate;
		Status Tick(World& world, ServerEnemy& enemy, float deltaTime) override;
	};

	class Action : public Node
	{
	public:
		std::function<Status(World&, ServerEnemy&, float)> action;
		Status Tick(World& world, ServerEnemy& enemy, float deltaTime) override;
	};
}
