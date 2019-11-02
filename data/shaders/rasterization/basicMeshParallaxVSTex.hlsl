#include "../common/structures.hlsl"
#include "../common/vertexDefinitions.hlsl"

ConstantBuffer<CameraBuffer> g_cameraBuffer : register(b0);

FullMeshParallaxVertexOut VS(TexturedVertexIn12 vin)
{
    FullMeshParallaxVertexOut vout;
	
	// Transform to homogeneous clip space.
    vout.PosH = mul(float4(vin.PosL, 1.0f), g_cameraBuffer.MVP);
	
	// Just pass vertex color into the pixel shader.
    vout.Normal = vin.Normal;
    vout.uv = vin.uvs.xy;
    vout.tangent = vin.tangents.xyz;
    vout.worldPos = float4(vin.PosL, 1.0f);

    float3 N = normalize(vin.Normal.xyz);
    float3 T = normalize(vin.tangents.xyz);
    float3 B = normalize(cross( N,T));
    float3x3 NTB = transpose(float3x3(T, B, N));

    //extra data needed for compute view direction in tangent space
    //TODO we already have all the necessary data, normal, tangent etc in the frag
    //maybe we can simply built the matrix there and convert to tangent space?
    //might be more multiplications but less memory moving around from the raster
    vout.tangentFragPos = float4(mul(vout.worldPos.xyz, NTB), 0.0);
    vout.tangentViewPos = float4(mul(g_cameraBuffer.position.xyz, NTB), 0.0);

    return vout;
}