#version 450

#extension GL_GOOGLE_include_directive: require
#extension GL_KHR_shader_subgroup_ballot: enable
#extension GL_KHR_shader_subgroup_vote: enable
#extension GL_KHR_shader_subgroup_arithmetic: require 


layout (set=3,binding=0) buffer tileIndices 
{
	ivec2 values[];
};

const int SUPPORT_DATA_ATOMIC_OFFSET = 8;
const int SUPPORT_DATA_OFFSET = 16;

layout (local_size_x = 1,local_size_y =1) in;
void CS()
{	

values[0].x = (values[SUPPORT_DATA_ATOMIC_OFFSET].x )*15*500;
//values[0].x = (values[SUPPORT_DATA_ATOMIC_OFFSET].x + int(gl_SubgroupSize ==32)*15)*15*500;
values[0].y = 1;
values[1].x = 0;
values[1].y = 0;


values[SUPPORT_DATA_ATOMIC_OFFSET].x = 0;
values[SUPPORT_DATA_ATOMIC_OFFSET].y= 0;
values[SUPPORT_DATA_ATOMIC_OFFSET+1].x = 0;
values[SUPPORT_DATA_ATOMIC_OFFSET+1].y= 0;
values[SUPPORT_DATA_ATOMIC_OFFSET+2].x = 0;
values[SUPPORT_DATA_ATOMIC_OFFSET+2].y= 0;
}
