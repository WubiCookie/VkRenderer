#pragma once

#include "MyShaderWriter.hpp"
#include "PbrShadingModel.hpp"
#include "TextureInterface.hpp"
#include "UniformBuffer.hpp"
#include "VulkanDevice.hpp"
#include "VulkanHelperStructs.hpp"

#include "cdm_maths.hpp"

#include <map>
#include <string>
#include <variant>
#include <vector>

namespace cdm
{
class RenderWindow;
class RenderPass;
class CommandBuffer;
class Material;
class MaterialInstance;

class MaterialInterface
{
public:
	template <typename T>
	class NumberParameter
	{
	public:
		T value{};
		size_t offset{ 0 };
		Movable<UniformBuffer*> uniformBuffer;
	};

	using FloatParameter = NumberParameter<float>;
	using Vec2Parameter = NumberParameter<vector2>;
	using Vec3Parameter = NumberParameter<vector3>;
	using Vec4Parameter = NumberParameter<vector4>;
	using Mat2Parameter = NumberParameter<matrix2>;
	using Mat3Parameter = NumberParameter<matrix3>;
	using Mat4Parameter = NumberParameter<matrix4>;
	using UIntParameter = NumberParameter<uint32_t>;
	using IntParameter = NumberParameter<int32_t>;

protected:
	virtual float floatParameter(const std::string& name,
	                             uint32_t instanceIndex)
	{
		assert(false);
		return {};
	}
	virtual vector2 vec2Parameter(const std::string& name,
	                              uint32_t instanceIndex)
	{
		assert(false);
		return {};
	}
	virtual vector3 vec3Parameter(const std::string& name,
	                              uint32_t instanceIndex)
	{
		assert(false);
		return {};
	}
	virtual vector4 vec4Parameter(const std::string& name,
	                              uint32_t instanceIndex)
	{
		assert(false);
		return {};
	}
	virtual matrix2 mat2Parameter(const std::string& name,
	                              uint32_t instanceIndex)
	{
		assert(false);
		return {};
	}
	virtual matrix3 mat3Parameter(const std::string& name,
	                              uint32_t instanceIndex)
	{
		assert(false);
		return {};
	}
	virtual matrix4 mat4Parameter(const std::string& name,
	                              uint32_t instanceIndex)
	{
		assert(false);
		return {};
	}
	virtual uint32_t uintParameter(const std::string& name,
	                               uint32_t instanceIndex)
	{
		assert(false);
		return {};
	}
	virtual int32_t intParameter(const std::string& name,
	                             uint32_t instanceIndex)
	{
		assert(false);
		return {};
	}

	virtual void setFloatParameter(const std::string& name,
	                               uint32_t instanceIndex, float a)
	{
		assert(false);
	}
	virtual void setVec2Parameter(const std::string& name,
	                              uint32_t instanceIndex, const vector2& a)
	{
		assert(false);
	}
	virtual void setVec3Parameter(const std::string& name,
	                              uint32_t instanceIndex, const vector3& a)
	{
		assert(false);
	}
	virtual void setVec4Parameter(const std::string& name,
	                              uint32_t instanceIndex, const vector4& a)
	{
		assert(false);
	}
	virtual void setMat2Parameter(const std::string& name,
	                              uint32_t instanceIndex, const matrix2& a)
	{
		assert(false);
	}
	virtual void setMat3Parameter(const std::string& name,
	                              uint32_t instanceIndex, const matrix3& a)
	{
		assert(false);
	}
	virtual void setMat4Parameter(const std::string& name,
	                              uint32_t instanceIndex, const matrix4& a)
	{
		assert(false);
	}
	virtual void setUintParameter(const std::string& name,
	                              uint32_t instanceIndex, uint32_t a)
	{
		assert(false);
	}
	virtual void setIntParameter(const std::string& name,
	                             uint32_t instanceIndex, int32_t a)
	{
		assert(false);
	}

public:
	virtual void bind(CommandBuffer& cb, VkPipelineLayout layout) = 0;
	virtual void pushOffset(CommandBuffer& cb, VkPipelineLayout layout) = 0;

	virtual Material& material() = 0;
	virtual uint32_t index() { return 0; }
};

class MaterialInstance final : public MaterialInterface
{
	friend Material;

	std::reference_wrapper<Material> m_material;

	//Movable<uint32_t, 0ull> m_instanceOffset;
	uint32_t m_instanceOffset = 0;

	MaterialInstance(Material& material, uint32_t instanceOffset);

public:
	MaterialInstance(const MaterialInstance&) = delete;
	MaterialInstance(MaterialInstance&&) = default;
	~MaterialInstance() = default;

	MaterialInstance& operator=(const MaterialInstance&) = delete;
	MaterialInstance& operator=(MaterialInstance&&) = default;

	float floatParameter(const std::string& name);
	vector2 vec2Parameter(const std::string& name);
	vector3 vec3Parameter(const std::string& name);
	vector4 vec4Parameter(const std::string& name);
	matrix2 mat2Parameter(const std::string& name);
	matrix3 mat3Parameter(const std::string& name);
	matrix4 mat4Parameter(const std::string& name);
	uint32_t uintParameter(const std::string& name);
	int32_t intParameter(const std::string& name);

	void setFloatParameter(const std::string& name, float a);
	void setVec2Parameter(const std::string& name, const vector2& a);
	void setVec3Parameter(const std::string& name, const vector3& a);
	void setVec4Parameter(const std::string& name, const vector4& a);
	void setMat2Parameter(const std::string& name, const matrix2& a);
	void setMat3Parameter(const std::string& name, const matrix3& a);
	void setMat4Parameter(const std::string& name, const matrix4& a);
	void setUintParameter(const std::string& name, uint32_t a);
	void setIntParameter(const std::string& name, int32_t a);

	void bind(CommandBuffer& cb, VkPipelineLayout layout) override;
	void pushOffset(CommandBuffer& cb, VkPipelineLayout layout) override;

	Material& material() override { return m_material; }
	uint32_t index() override { return m_instanceOffset; }
};

class Material : public MaterialInterface
{
	friend MaterialInstance;

	Movable<RenderWindow*> rw;
	Movable<PbrShadingModel*> m_shadingModel;

	std::vector<MaterialInstance> m_instances;
	Movable<uint32_t, 0ull> m_instancePoolSize;

protected:
	UniqueDescriptorPool m_descriptorPool;

	UniqueDescriptorSetLayout m_descriptorSetLayout;
	Movable<VkDescriptorSet> m_descriptorSet;

public:
	struct VertexShaderBuildDataBase
	{
		virtual ~VertexShaderBuildDataBase() {}
	};
	struct FragmentShaderBuildDataBase
	{
		virtual ~FragmentShaderBuildDataBase() {}
	};

	Material() = default;
	Material(RenderWindow& rw, PbrShadingModel& shadingModel,
	         uint32_t instancePoolSize = 0);
	Material(const Material&) = delete;
	Material(Material&&) = default;
	~Material() = default;

	Material& operator=(const Material&) = delete;
	Material& operator=(Material&&) = default;

	// void vertexFunction(Vec3& inOutPosition, Vec3& inOutNormal);
	virtual MaterialVertexFunction vertexFunction(
	    sdw::VertexWriter& writer,
	    VertexShaderBuildDataBase* shaderBuildData) = 0;

	// void fragmentFunction(Vec4& inOutAlbedo,
	//                       Vec3& inOutWsPosition,
	//                       Vec3& inOutWsNormal,
	//                       Vec3& inOutWsTangent,
	//                       Float& inOutMetalness,
	//                       Float& inOutRoughness);
	virtual MaterialFragmentFunction fragmentFunction(
	    sdw::FragmentWriter& writer,
	    FragmentShaderBuildDataBase* shaderBuildData) = 0;

	virtual std::unique_ptr<VertexShaderBuildDataBase>
	instantiateVertexShaderBuildData();

	virtual std::unique_ptr<FragmentShaderBuildDataBase>
	instantiateFragmentShaderBuildData();

	RenderWindow& renderWindow() { return *rw; }
	PbrShadingModel& shadingModel() { return *m_shadingModel; }
	uint32_t instancePoolSize() const noexcept { return m_instancePoolSize; }

	const VkDescriptorSetLayout& descriptorSetLayout() const noexcept
	{
		return m_descriptorSetLayout;
	}
	const VkDescriptorSet& descriptorSet() const noexcept
	{
		return m_descriptorSet;
	}

	MaterialInstance* instanciate();

protected:
	using MaterialInterface::floatParameter;
	using MaterialInterface::intParameter;
	using MaterialInterface::mat2Parameter;
	using MaterialInterface::mat3Parameter;
	using MaterialInterface::mat4Parameter;
	using MaterialInterface::setFloatParameter;
	using MaterialInterface::setIntParameter;
	using MaterialInterface::setMat2Parameter;
	using MaterialInterface::setMat3Parameter;
	using MaterialInterface::setMat4Parameter;
	using MaterialInterface::setUintParameter;
	using MaterialInterface::setVec2Parameter;
	using MaterialInterface::setVec3Parameter;
	using MaterialInterface::setVec4Parameter;
	using MaterialInterface::uintParameter;
	using MaterialInterface::vec2Parameter;
	using MaterialInterface::vec3Parameter;
	using MaterialInterface::vec4Parameter;

public:
	float floatParameter(const std::string& name)
	{
		return MaterialInterface::floatParameter(name, 0);
	}
	vector2 vec2Parameter(const std::string& name)
	{
		return MaterialInterface::vec2Parameter(name, 0);
	}
	vector3 vec3Parameter(const std::string& name)
	{
		return MaterialInterface::vec3Parameter(name, 0);
	}
	vector4 vec4Parameter(const std::string& name)
	{
		return MaterialInterface::vec4Parameter(name, 0);
	}
	matrix2 mat2Parameter(const std::string& name)
	{
		return MaterialInterface::mat2Parameter(name, 0);
	}
	matrix3 mat3Parameter(const std::string& name)
	{
		return MaterialInterface::mat3Parameter(name, 0);
	}
	matrix4 mat4Parameter(const std::string& name)
	{
		return MaterialInterface::mat4Parameter(name, 0);
	}
	uint32_t uintParameter(const std::string& name)
	{
		return MaterialInterface::uintParameter(name, 0);
	}
	int32_t intParameter(const std::string& name)
	{
		return MaterialInterface::intParameter(name, 0);
	}

	void setFloatParameter(const std::string& name, float a)
	{
		MaterialInterface::setFloatParameter(name, 0, a);
	}
	void setVec2Parameter(const std::string& name, const vector2& a)
	{
		MaterialInterface::setVec2Parameter(name, 0, a);
	}
	void setVec3Parameter(const std::string& name, const vector3& a)
	{
		MaterialInterface::setVec3Parameter(name, 0, a);
	}
	void setVec4Parameter(const std::string& name, const vector4& a)
	{
		MaterialInterface::setVec4Parameter(name, 0, a);
	}
	void setMat2Parameter(const std::string& name, const matrix2& a)
	{
		MaterialInterface::setMat2Parameter(name, 0, a);
	}
	void setMat3Parameter(const std::string& name, const matrix3& a)
	{
		MaterialInterface::setMat3Parameter(name, 0, a);
	}
	void setMat4Parameter(const std::string& name, const matrix4& a)
	{
		MaterialInterface::setMat4Parameter(name, 0, a);
	}
	void setUintParameter(const std::string& name, uint32_t a)
	{
		MaterialInterface::setUintParameter(name, 0, a);
	}
	void setIntParameter(const std::string& name, int32_t a)
	{
		MaterialInterface::setIntParameter(name, 0, a);
	}

	void setParameter(const std::string& name, float a)
	{
		setFloatParameter(name, a);
	}
	void setParameter(const std::string& name, const vector2& a)
	{
		setVec2Parameter(name, a);
	}
	void setParameter(const std::string& name, const vector3& a)
	{
		setVec3Parameter(name, a);
	}
	void setParameter(const std::string& name, const vector4& a)
	{
		setVec4Parameter(name, a);
	}
	void setParameter(const std::string& name, const matrix2& a)
	{
		setMat2Parameter(name, a);
	}
	void setParameter(const std::string& name, const matrix3& a)
	{
		setMat3Parameter(name, a);
	}
	void setParameter(const std::string& name, const matrix4& a)
	{
		setMat4Parameter(name, a);
	}
	void setParameter(const std::string& name, uint32_t a)
	{
		setUintParameter(name, a);
	}
	void setParameter(const std::string& name, int32_t a)
	{
		setIntParameter(name, a);
	}

	void bind(CommandBuffer& cb, VkPipelineLayout layout) override;
	void pushOffset(CommandBuffer& cb, VkPipelineLayout layout) override;

	Material& material() override { return *this; }
};
}  // namespace cdm
