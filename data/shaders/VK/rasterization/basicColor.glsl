#version 450

layout(location=0) out vec4 outputColor;

layout (location = 1) in vec4 inColor;

void PS() 
{
    outputColor = inColor;
}


