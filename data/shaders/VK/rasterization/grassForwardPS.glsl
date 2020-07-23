#version 450



layout(location=0) out vec4 outputColor;
layout (location = 1) in vec2 inUV;

void PS()
{
   //vec2 uv = vec2(inUV.x,1.0f - inUV.y);
   vec4 color= vec4(0,0.8,0,1);
   outputColor = color;
}
