#ifndef VKRENDERER_KDTREE_HPP
#define VKRENDERER_KDTREE_HPP 1

#include <array>
#include <random>
#include <vector>
#include "cdm_maths.hpp"

namespace cdm
{
struct Box
{
	vector3 min;
	vector3 max;
	bool init = false;

	bool contains(vector3 p) const
	{
		return p.x > min.x && p.x < max.x && p.y > min.y && p.y < max.y &&
		       p.z > min.z && p.z < max.z;
	}
};

inline bool intersection(Box b, ray3d r)
{
	vector3 inv;
	inv.x = 1.0f / r.direction.x;
	inv.y = 1.0f / r.direction.y;
	inv.z = 1.0f / r.direction.z;

	float t1 = (b.min.x - r.origin.x) * inv.x;
	float t2 = (b.max.x - r.origin.x) * inv.x;

	float tmin = std::min(t1, t2);
	float tmax = std::max(t1, t2);

	t1 = (b.min.y - r.origin.y) * inv.y;
	t2 = (b.max.y - r.origin.y) * inv.y;

	tmin = std::max(tmin, std::min(t1, t2));
	tmax = std::min(tmax, std::max(t1, t2));

	t1 = (b.min.z - r.origin.z) * inv.z;
	t2 = (b.max.z - r.origin.z) * inv.z;

	tmin = std::max(tmin, std::min(t1, t2));
	tmax = std::min(tmax, std::max(t1, t2));

	return tmax >= tmin;
}

enum class Axis : int
{
	X,
	Y,
	Z,
};

inline Axis random_axis()
{
	static std::random_device rd;
	static std::mt19937 gen(rd());
	int a = int(Axis::X);
	int b = int(Axis::Z);
	std::uniform_int_distribution<int> dis(a, b);

	int c = dis(gen);

	return Axis(c);
}

inline bool collides(const Box& b1, const Box& b2)
{
	if (b1.init == false || b2.init == false)
		return false;

	if (b1.contains(b2.min))
		return true;
	if (b1.contains(b2.max))
		return true;
	if (b2.contains(b1.min))
		return true;
	if (b2.contains(b1.max))
		return true;

	std::array<vector3, 6> otherPoints;
	otherPoints[0] = { b1.min.x, b1.min.y, b1.max.z };
	otherPoints[1] = { b1.min.x, b1.max.y, b1.min.z };
	otherPoints[2] = { b1.max.x, b1.min.y, b1.min.z };
	otherPoints[3] = { b1.min.x, b1.max.y, b1.max.z };
	otherPoints[4] = { b1.max.x, b1.min.y, b1.max.z };
	otherPoints[5] = { b1.max.x, b1.max.y, b1.min.z };

	for (auto& p : otherPoints)
		if (b2.contains(p))
			return true;

	otherPoints[0] = { b2.min.x, b2.min.y, b2.max.z };
	otherPoints[1] = { b2.min.x, b2.max.y, b2.min.z };
	otherPoints[2] = { b2.max.x, b2.min.y, b2.min.z };
	otherPoints[3] = { b2.min.x, b2.max.y, b2.max.z };
	otherPoints[4] = { b2.max.x, b2.min.y, b2.max.z };
	otherPoints[5] = { b2.max.x, b2.max.y, b2.min.z };

	for (auto& p : otherPoints)
		if (b1.contains(p))
			return true;

	return false;
}

Box box_union(const Box& b1, const Box& b2)
{
	if (b1.init == false || b2.init == false)
		return {};

	Box res;
	res.init = true;
	res.min.x = std::min(b1.min.x, b2.min.x);
	res.min.y = std::min(b1.min.y, b2.min.y);
	res.min.z = std::min(b1.min.z, b2.min.z);
	res.max.x = std::max(b1.max.x, b2.max.x);
	res.max.y = std::max(b1.max.y, b2.max.y);
	res.max.z = std::max(b1.max.z, b2.max.z);

	return res;
}

bool split(const Box& parent, const Box& b1, const Box& b2, Box& out1,
           Box& out2, Axis axis)
{
	if (parent.init == false || b1.init == false || b2.init == false)
		return false;

	if (collides(b1, b2))
		return false;  // can not split

	std::array<vector3, 8> points1;
	points1[0] = { b1.min.x, b1.min.y, b1.max.z };
	points1[1] = { b1.min.x, b1.max.y, b1.min.z };
	points1[2] = { b1.max.x, b1.min.y, b1.min.z };
	points1[3] = { b1.min.x, b1.max.y, b1.max.z };
	points1[4] = { b1.max.x, b1.min.y, b1.max.z };
	points1[5] = { b1.max.x, b1.max.y, b1.min.z };
	points1[6] = { b1.min.x, b1.min.y, b1.min.z };
	points1[7] = { b1.max.x, b1.max.y, b1.max.z };

	std::array<vector3, 8> points2;
	points2[0] = { b2.min.x, b2.min.y, b2.max.z };
	points2[1] = { b2.min.x, b2.max.y, b2.min.z };
	points2[2] = { b2.max.x, b2.min.y, b2.min.z };
	points2[3] = { b2.min.x, b2.max.y, b2.max.z };
	points2[4] = { b2.max.x, b2.min.y, b2.max.z };
	points2[5] = { b2.max.x, b2.max.y, b2.min.z };
	points2[6] = { b2.min.x, b2.min.y, b2.min.z };
	points2[7] = { b2.max.x, b2.max.y, b2.max.z };

	vector3* nearestPoint1 = &points1[0];
	vector3* nearestPoint2 = &points2[0];

	float minDistance =
	    vector3::distance_squared_between(points1[0], points2[0]);

	for (auto& p1 : points1)
		for (auto& p2 : points2)
		{
			float newDistance = vector3::distance_squared_between(p1, p2);
			if (newDistance < minDistance)
			{
				minDistance = newDistance;

				nearestPoint1 = &p1;
				nearestPoint2 = &p2;
			}
		}

	out1 = parent;
	out2 = parent;

	// split in the middle
	if (axis == Axis::X)
	{
		float x = (nearestPoint1->x + nearestPoint2->x) / 2.0f;

		out1.max.x = x;
		out2.min.x = x;
	}
	else if (axis == Axis::Y)
	{
		float y = (nearestPoint1->y + nearestPoint2->y) / 2.0f;

		out1.max.y = y;
		out2.min.y = y;
	}
	else  // if (axis == Axis::Z)
	{
		float z = (nearestPoint1->z + nearestPoint2->z) / 2.0f;

		out1.max.z = z;
		out2.min.z = z;
	}

	return true;
}

template <typename T>
struct KDTree
{
	using DataPair = std::pair<Box, T>;

	struct Node
	{
		Box box;

		std::vector<DataPair> leafData;

		size_t left = -1ull;
		size_t right = -1ull;
	};

	std::vector<Node> nodes;

	KDTree(Box rootBox) { nodes.push_back(Node{ rootBox }); }
	KDTree(const KDTree&) = default;
	KDTree(KDTree&&) = default;

	Node& root() { return nodes.front(); }
	const Node& root() const { return nodes.front(); }

	void insert(Box b, T data) { insert(0, b, data); }

	void insert(size_t nodeIndex_, Box b, T data, size_t depth = 0)
	{
		auto box = [=]() -> Box& { return nodes[nodeIndex_].box; };
		auto left = [=]() -> size_t& { return nodes[nodeIndex_].left; };
		auto right = [=]() -> size_t& { return nodes[nodeIndex_].right; };
		auto leafData = [=]() -> std::vector<DataPair>& {
			return nodes[nodeIndex_].leafData;
		};

		if (b.init == false)
			return;

		if (collides(box(), b))  // input collides with current node
		{
			if (depth >= 30)
			{
				leafData().push_back(DataPair{ b, data });
				return;
			}

			if (left() == -1)
			{
				if (leafData().empty())
				{
					leafData().push_back(DataPair{ b, data });
				}
				else
				{
					Box leavesBox = leafData().front().first;

					bool c = false;
					for (auto& pair : leafData())
					{
						if (collides(pair.first, b))
						{
							c = true;
						}

						leavesBox = box_union(leavesBox, pair.first);
					}

					leafData().push_back(DataPair{ b, data });

					if (!c)
					{
						Node l;
						Node r;

						split(box(), b, leavesBox, l.box, r.box,
						      random_axis());

						left() = nodes.size();
						nodes.push_back(std::move(l));

						for (auto& pair : leafData())
						{
							insert(left(), pair.first, pair.second, depth + 1);
						}

						right() = nodes.size();
						nodes.push_back(std::move(r));

						for (auto& pair : leafData())
						{
							insert(right(), pair.first, pair.second,
							       depth + 1);
						}

						leafData().clear();
					}
				}
			}
			else  // if (left != -1)  // has children
			{
				insert(left(), b, data, depth + 1);

				insert(right(), b, data, depth + 1);
			}
		}
		// Do nothing (box out of bounds)
	}

	template <typename Find, typename Found>
	void query(Find find, Found found) const
	{
		query(0, find, found);
	}

	template <typename Find, typename Found>
	void query(size_t nodeIndex, Find find, Found found) const
	{
		const Node& n = nodes[nodeIndex];

		if (find(n))
		{
			for (const auto& pair : n.leafData)
				found(pair);

			if (n.left != -1)
			{
				query(n.left, find, found);
				query(n.right, find, found);
			}
		}
	}

	template <typename Functor>
	void traverse(Functor F) const
	{
		traverse(0, F);
	}

	template <typename Functor>
	void traverse(size_t nodeIndex, Functor F) const
	{
		const Node& n = nodes[nodeIndex];

		F(n);

		if (n.left != -1)
		{
			traverse(n.left, F);
			traverse(n.right, F);
		}
	}
};

}  // namespace cdm

#endif  // VKRENDERER_KDTREE_HPP
