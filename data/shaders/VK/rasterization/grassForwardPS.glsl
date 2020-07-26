#version 450


layout (set=3,binding = 1) uniform texture2D windTex;

const vec3 baseColor = vec3(0.015,0.455,0.026);
const vec3 topColor = vec3(0.357,0.67,0.09);

layout(location=0) out vec4 outputColor;
layout (location = 1) in vec2 inUV;

layout (set=1,binding = 0) uniform sampler[7] colorSampler;

void PS()
{
   //vec2 uv = vec2(inUV.x,1.0f - inUV.y);
   vec3 albedo = texture (sampler2D (windTex, colorSampler[2]), inUV).xyz;
   vec4 color= vec4(mix(baseColor,topColor,inUV.y),1.0f);

   outputColor = color;
}
