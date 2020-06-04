#include "Model.hpp"

#include "CommandBuffer.hpp"
#include "Material.hpp"
#include "MyShaderWriter.hpp"
#include "PbrShadingModel.hpp"
#include "RenderWindow.hpp"
#include "Scene.hpp"
#include "StandardMesh.hpp"

#include <iostream>

namespace cdm
{
Model::Model(RenderWindow& renderWindow, StandardMesh& mesh,
             MaterialInterface& material)
    : rw(&renderWindow),
      m_mesh(&mesh),
      m_material(&material)
{
}
}  // namespace cdm
