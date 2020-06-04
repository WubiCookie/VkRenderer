#pragma once

#include "UniformBuffer.hpp"
#include "VulkanDevice.hpp"

#include "cdm_maths.hpp"

namespace cdm
{
class CommandBuffer;
class MaterialInterface;
class StandardMesh;
class RenderWindow;

class Model final
{
	Movable<RenderWindow*> rw;

	Movable<MaterialInterface*> m_material;
	Movable<StandardMesh*> m_mesh;

public:
	Model() = default;
	Model(RenderWindow& rw, StandardMesh& mesh, MaterialInterface& material);
	Model(const Model&) = delete;
	Model(Model&&) = default;
	~Model() = default;

	Model& operator=(const Model&) = delete;
	Model& operator=(Model&&) = default;

	const MaterialInterface* material() const noexcept { return m_material; }
	const StandardMesh* mesh() const noexcept { return m_mesh; }
};
}  // namespace cdm
