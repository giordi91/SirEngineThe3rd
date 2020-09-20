
vec3 createCube( uint vertexID)
{
	uint b = 1 << vertexID;
	float x = float((0x287a & b) != 0);
	float y = float((0x02af & b) != 0);
	float z = float((0x31e3 & b) != 0);
	return vec3(x,y,z);
}


vec2 remapUV(vec2 uv, vec4 remapper)
{
	return uv * remapper.xy + remapper.zw;
}
