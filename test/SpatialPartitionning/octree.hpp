#ifndef VKRENDERER_OCTREE_HPP
#define VKRENDERER_OCTREE_HPP 1

#include <array>
#include <utility>
#include <vector>
#include "cdm_maths.hpp"

namespace cdm
{
template <typename T>
struct Octree
{
	using DataPair = std::pair<aabb, T>;

	struct Node
	{
		aabb box;

		std::vector<DataPair> leafData;

		size_t firstChild = -1ull;
	};

	std::vector<Node> nodes;

	Octree(aabb rootBox) { nodes.push_back(Node{ rootBox }); }
	Octree(const Octree&) = default;
	Octree(Octree&&) = default;

	Node& root() { return nodes.front(); }
	const Node& root() const { return nodes.front(); }

	void insert(aabb b, T data, size_t nodeIndex = 0)
	{
		auto box = [=]() -> aabb& { return nodes[nodeIndex].box; };
		auto leafData = [=]() -> std::vector<DataPair>& {
			return nodes[nodeIndex].leafData;
		};

		if (collides(box(), b))
		{
			leafData().push_back(DataPair{ b, data });
		}
	}

	static void split(const aabb& parent, aabb& b000, aabb& b100, aabb& b010,
	                  aabb& b001, aabb& b110, aabb& b101, aabb& b011,
	                  aabb& b111)
	{
		vector3 center = parent.get_center();

		b000 = { parent.min, center };

		b100 = b000;
		b100.min.x = center.x;
		b100.max.x = parent.max.x;

		b010 = b000;
		b010.min.y = center.y;
		b010.max.y = parent.max.y;

		b001 = b000;
		b001.min.z = center.z;
		b001.max.z = parent.max.z;

		b110 = b100;
		b110.min.y = center.y;
		b110.max.y = parent.max.y;

		b101 = b100;
		b101.min.z = center.z;
		b101.max.z = parent.max.z;

		b011 = b010;
		b011.min.z = center.z;
		b011.max.z = parent.max.z;

		b111 = { center, parent.max };
	}

	void spread(size_t nodeIndex)
	{
		auto box = [=]() -> aabb& { return nodes[nodeIndex].box; };
		auto firstChild = [=]() -> size_t& {
			return nodes[nodeIndex].firstChild;
		};
		auto leafData = [=]() -> std::vector<DataPair>& {
			return nodes[nodeIndex].leafData;
		};

		if (leafData().empty())
			return;

		if (firstChild() == -1)
		{
			firstChild() = nodes.size();
			nodes.resize(nodes.size() + 8);

			auto b000 = [=]() -> aabb& { return nodes[firstChild() + 0].box; };
			auto b100 = [=]() -> aabb& { return nodes[firstChild() + 1].box; };
			auto b010 = [=]() -> aabb& { return nodes[firstChild() + 2].box; };
			auto b001 = [=]() -> aabb& { return nodes[firstChild() + 3].box; };
			auto b110 = [=]() -> aabb& { return nodes[firstChild() + 4].box; };
			auto b101 = [=]() -> aabb& { return nodes[firstChild() + 5].box; };
			auto b011 = [=]() -> aabb& { return nodes[firstChild() + 6].box; };
			auto b111 = [=]() -> aabb& { return nodes[firstChild() + 7].box; };

			split(box(), b000(), b100(), b010(), b001(), b110(), b101(),
			      b011(), b111());
		}

		for (auto& pair : leafData())
			for (size_t i = firstChild(); i < firstChild() + 8; i++)
				insert(pair.first, pair.second, i);

		leafData().clear();
	}

	void spread_n(size_t n)
	{
		nodes.reserve(size_t(std::pow(8, n)));

		size_t old_s = 0;
		size_t s = nodes.size();

		for (size_t j = 0; j < n; j++)
		{
			for (size_t i = old_s; i < s; i++)
				spread(i);
			old_s = s;
			s = nodes.size();
		}
	}

	template <typename Functor>
	void query(ray3d r, Functor F, size_t nodeIndex = 0) const
	{
		const Node& n = nodes[nodeIndex];

		if (collides(n.box, r))
		{
			for (const auto& pair : n.leafData)
				if (collides(pair.first, r))
					F(pair);

			if (n.firstChild != -1)
				for (size_t i = n.firstChild; i < n.firstChild + 8; i++)
					query(r, F, i);
		}
	}

	template <typename Find, typename Found>
	void traverse(Find find, Found found, size_t nodeIndex = 0) const
	{
		const Node& n = nodes[nodeIndex];

		if (find(n))
		{
			for (const auto& pair : n.leafData)
				found(pair);

			if (n.c000 != -1)
				for (size_t i = n.firstChild; i < n.firstChild + 8; i++)
					query(find, found, i);
		}
	}
};

}  // namespace cdm

#endif  // VKRENDERER_OCTREE_HPP
