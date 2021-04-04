#version 450

layout (binding = 0) uniform sampler2D tex;

layout(location = 0) in vec2 inTexCoords;

layout(location = 0) out vec4 outColor;

vec3 sRGBToLinear(vec3 color)
{
	if (length(color) <= 0.04045)
    	return color / 12.92;
	else
		return pow((color + vec3(0.055)) / 1.055, vec3(2.4));
}

void main() 
{
	vec4 inputColor = texture(tex, inTexCoords).rgba;
	outColor = vec4(inputColor.rgb, inputColor.a);
}
