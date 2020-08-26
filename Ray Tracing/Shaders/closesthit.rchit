#version 460
#extension GL_NV_ray_tracing : require
#extension GL_EXT_nonuniform_qualifier : enable

layout(location = 0) rayPayloadInNV vec3 hitValue;
layout(location = 2) rayPayloadNV bool isShadowed;
hitAttributeNV vec3 attribs;

layout(binding = 0, set = 0) uniform accelerationStructureNV topLevelAS;
layout(binding = 2, set = 0) buffer Vertices { vec4 v[]; } vertices;
layout(binding = 3, set = 0) buffer Indices { uint i[]; } indices;

void main()
{
  hitValue = vec3(1.0, 0.0, 0.0);
}
