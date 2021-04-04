#version 450

// Vertex
layout(location = 0) in vec2 inPosition;
layout(location = 1) in vec2 inTexCoord;
 
// Instance
layout(location = 2) in vec4 inParticlePositionBornTime;
layout(location = 3) in vec4 inParticleVelocityLifeTime;

out gl_PerVertex
{
    vec4 gl_Position;
};

layout(location = 0) out vec2 outTexCoord;

void main() 
{
    if(inParticlePositionBornTime.w < 0.0)
        gl_Position = vec4(0.0/0.0);
    else
        gl_Position = vec4(inPosition.x / 20.0 + inParticlePositionBornTime.x, inPosition.y / 20.0 + inParticlePositionBornTime.y, 0.0, 1.0);

    outTexCoord = inTexCoord;
}
