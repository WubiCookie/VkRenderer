#pragma once

#include "CompilerSpirV/compileSpirV.hpp"
#include "ShaderWriter/Intrinsics/Intrinsics.hpp"
#include "ShaderWriter/Source.hpp"

#define LocaleW(writer, name, value)                                          \
	auto name = writer.declLocale(#name, value);
#define FLOATW(writer, name) auto name = writer.declLocale<Float>(#name);
#define VEC2W(writer, name) auto name = writer.declLocale<Vec2>(#name);
#define VEC3W(writer, name) auto name = writer.declLocale<Vec3>(#name);
#define ConstantW(writer, name, value)                                        \
	auto name = writer.declConstant(#name, value);

#define Locale(name, value) LocaleW(writer, name, value)
#define FLOAT(name) FLOATW(writer, name)
#define VEC2(name) VEC2W(writer, name)
#define VEC3(name) VEC3W(writer, name)
#define Constant(name, value) ConstantW(writer, name, value)

namespace cdm
{
// void vertexFunction(Vec3& inOutPosition, Vec3& inOutNormal);
using MaterialVertexFunction =
    sdw::Function<sdw::Float, sdw::InOutVec3, sdw::InOutVec3>;

// void fragmentFunction(UInt inMaterialInstanceIndex,
//                       Vec4& inOutAlbedo,
//                       Vec3& inOutWsPosition,
//                       Vec3& inOutWsNormal,
//                       Vec3& inOutWsTangent,
//                       Float& inOutMetalness,
//                       Float& inOutRoughness);
using MaterialFragmentFunction =
    sdw::Function<sdw::Float, sdw::InUInt, sdw::InOutVec4, sdw::InOutVec3,
                  sdw::InOutVec3, sdw::InOutVec3, sdw::InOutFloat,
                  sdw::InOutFloat>;

// Vec4 combinedMaterialShading(UInt inMaterialInstanceIndex,
//                              Vec3 wsPosition,
//                              Vec3 wsNormal,
//                              Vec3 wsTangent,
//                              Vec3 wsViewPosition);
using CombinedMaterialShadingFragmentFunction =
    sdw::Function<sdw::Vec4, sdw::InUInt, sdw::InVec3, sdw::InVec3,
                  sdw::InVec3, sdw::InVec3>;
}  // namespace cdm
