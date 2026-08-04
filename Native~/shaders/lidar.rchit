#version 460
#extension GL_EXT_ray_tracing : require

struct HitPayload
{
    float hitDistance;
    int instanceId;
};
layout(location = 0) rayPayloadInEXT HitPayload payload;

hitAttributeEXT vec2 barycentrics;

void main()
{
    payload.hitDistance = gl_HitTEXT;
    payload.instanceId = gl_InstanceCustomIndexEXT;
}
