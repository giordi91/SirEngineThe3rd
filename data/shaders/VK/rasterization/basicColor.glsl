#version 450

/*
layout (set=3,binding=1) uniform g_settings 
{
	vec4 color;
    float pointSize;
}; 
*/

layout(location=0) out vec4 outputColor;

layout (location = 1) in vec4 inColor;

void PS() 
{
    outputColor = inColor;
}


