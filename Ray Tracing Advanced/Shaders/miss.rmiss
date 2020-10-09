#version 460
#extension GL_NV_ray_tracing : require

struct CommonPayload
{
    // From raygen values
    vec3 rayOrigin;

    // From closest hit values
    vec3 value;
    vec3 origin;
    vec3 direction;
    vec3 kS;
};

layout(location = 0) rayPayloadInNV CommonPayload payload;

void main()
{
    payload.value = vec3(0.0);
    payload.kS = vec3(0.0);
}