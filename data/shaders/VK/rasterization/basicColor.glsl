#version 450

/*
layout (set=3,binding=1) uniform g_settings 
{
	vec4 color;
    float pointSize;
}; 
*/

layout(location=0) out vec4 outputColor;

void PS() 
{
    //outputColor = color;
    outputColor = vec4(1,0,0,1);
}


