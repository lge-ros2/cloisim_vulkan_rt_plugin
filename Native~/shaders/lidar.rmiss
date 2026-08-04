#version 460
#extension GL_EXT_ray_tracing : require

struct HitPayload
{
    float hitDistance;
    int instanceId;
};
layout(location = 0) rayPayloadInEXT HitPayload payload;

void main()
{
    payload.hitDistance = -1.0;
    payload.instanceId = -1;
}
