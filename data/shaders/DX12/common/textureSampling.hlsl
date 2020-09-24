#ifndef TEXTURE_SAMPLING_HSLS
#define TEXTURE_SAMPLING_HSLS

float2 remapUV(float2 uv, float4 remapper)
{
	return uv * remapper.xy + remapper.zw;
}

#endif
