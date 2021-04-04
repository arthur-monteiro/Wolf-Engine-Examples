#version 450

layout (binding = 1) uniform sampler2D tex;

layout(location = 0) in vec2 inTexCoords;

layout(location = 0) out vec4 outColor;

struct MyStruct
{
  vec4 pos;
  vec4 blurColor;
};

layout(std430, binding = 8) writeonly restrict buffer unused {
	MyStruct data[];
};

void main() 
{
	outColor = vec4(texture(tex, inTexCoords).rgb , 1.0);
	if(outColor.x > 100)
	{
		data[0].pos = vec4(1.0);
	}
}
