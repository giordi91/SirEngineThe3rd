#version 450

layout (local_size_x = 16,local_size_y =1) in;

layout (set=3,binding=0) buffer inData 
{
	int value[];
};
layout (set=3,binding=1) buffer outData 
{
	int outValue[];
};


void CS()
{	
	int val =value[gl_GlobalInvocationID.x]; 
	outValue[gl_GlobalInvocationID.x] =val + 1;
}
