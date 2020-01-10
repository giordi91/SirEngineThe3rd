#version 450

layout (set=1,binding=1) uniform g_settings 
{
	vec4 color;
    float pointSize;
}; 

layout(location=0) out vec4 outputColor;

void PS() 
{
    outputColor = color;
}


