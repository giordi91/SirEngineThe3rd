#version 450

#extension GL_GOOGLE_include_directive: require
#extension GL_KHR_shader_subgroup_ballot: enable
#extension GL_KHR_shader_subgroup_vote: enable
#extension GL_KHR_shader_subgroup_arithmetic: require 

#include "../common/structures.glsl"
#include "../common/utility.glsl"

layout (set=3,binding=0) buffer tileIndices 
{
	ivec2 values[];
};
layout (set=3,binding=1) uniform inConfigData 
{
	GrassConfig grassConfig;
}; 

const int SUPPORT_DATA_ATOMIC_OFFSET0 = 8;
const int SUPPORT_DATA_ATOMIC_OFFSET1 = 16;
const int SUPPORT_DATA_ATOMIC_OFFSET2 = 24;
const int SUPPORT_DATA_ATOMIC_OFFSET3 = 32;
const int SUPPORT_DATA_OFFSET = 40;

layout (local_size_x = 1,local_size_y =1) in;
void CS()
{	

values[0].x = values[SUPPORT_DATA_ATOMIC_OFFSET0].x*15*int(grassConfig.pointsPerTileLod[0]);
values[0].y = 1;
values[1].x = 0;
values[1].y = 0;
values[2].x = values[SUPPORT_DATA_ATOMIC_OFFSET1].x*15*int(grassConfig.pointsPerTileLod[1]);
values[2].y = 1;
values[3].x = 0;
values[3].y = 0;
values[4].x = values[SUPPORT_DATA_ATOMIC_OFFSET2].x*15*int(grassConfig.pointsPerTileLod[2]);
values[4].y = 1;
values[5].x = 0;
values[5].y = 0;
values[6].x = values[SUPPORT_DATA_ATOMIC_OFFSET3].x*15*int(grassConfig.pointsPerTileLod[3]);
values[6].y = 1;
values[7].x = 0;
values[7].y = 0;


//resetting first atomic
values[SUPPORT_DATA_ATOMIC_OFFSET0].x = 0;
values[SUPPORT_DATA_ATOMIC_OFFSET0].y= 0;
values[SUPPORT_DATA_ATOMIC_OFFSET0+1].x = 0;
values[SUPPORT_DATA_ATOMIC_OFFSET0+1].y= 0;
values[SUPPORT_DATA_ATOMIC_OFFSET0+2].x = 0;
values[SUPPORT_DATA_ATOMIC_OFFSET0+2].y= 0;

//resetting second atomic
values[SUPPORT_DATA_ATOMIC_OFFSET1].x = 0;
values[SUPPORT_DATA_ATOMIC_OFFSET1].y= 0;
values[SUPPORT_DATA_ATOMIC_OFFSET1+1].x = 0;
values[SUPPORT_DATA_ATOMIC_OFFSET1+1].y= 0;
values[SUPPORT_DATA_ATOMIC_OFFSET1+2].x = 0;
values[SUPPORT_DATA_ATOMIC_OFFSET1+2].y= 0;

//resetting third atomic
values[SUPPORT_DATA_ATOMIC_OFFSET2].x = 0;
values[SUPPORT_DATA_ATOMIC_OFFSET2].y= 0;
values[SUPPORT_DATA_ATOMIC_OFFSET2+1].x = 0;
values[SUPPORT_DATA_ATOMIC_OFFSET2+1].y= 0;
values[SUPPORT_DATA_ATOMIC_OFFSET2+2].x = 0;
values[SUPPORT_DATA_ATOMIC_OFFSET2+2].y= 0;

//resetting fourth atomic
values[SUPPORT_DATA_ATOMIC_OFFSET3].x = 0;
values[SUPPORT_DATA_ATOMIC_OFFSET3].y= 0;
values[SUPPORT_DATA_ATOMIC_OFFSET3+1].x = 0;
values[SUPPORT_DATA_ATOMIC_OFFSET3+1].y= 0;
values[SUPPORT_DATA_ATOMIC_OFFSET3+2].x = 0;
values[SUPPORT_DATA_ATOMIC_OFFSET3+2].y= 0;
}
