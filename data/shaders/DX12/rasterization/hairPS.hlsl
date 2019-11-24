#include "../common/pbr.hlsl"
#include "../common/normals.hlsl"
#include "../common/structures.hlsl"
#include "../common/vertexDefinitions.hlsl"

ConstantBuffer<CameraBuffer> g_cameraBuffer : register(b0);
ConstantBuffer<DirectionalLightData> g_dirLight : register(b1);
ConstantBuffer<PhongMaterial> g_material : register(b2);
Texture2D albedoTex : register(t0);
Texture2D tangentTex : register(t1);
Texture2D separateAlpha : register(t2);
Texture2D aoTex: register(t3);


SamplerState gsamPointWrap : register(s0);
SamplerState gsamPointClamp : register(s1);
SamplerState gsamLinearWrap : register(s2);
SamplerState gsamLinearClamp : register(s3);
SamplerState gsamAnisotropicWrap : register(s4);
SamplerState gsamAnisotropicClamp : register(s5);
TextureCube skyboxIrradianceTexture : register(t4);
TextureCube skyboxRadianceTexture : register(t5);
Texture2D brdfTexture : register(t6);

float4 PS(FullMeshVertexOut input) : SV_Target
{
    float2 uv = float2(input.uv.x, 1.0f - input.uv.y);
    float3 albedo = albedoTex.Sample(gsamLinearClamp, uv).xyz; // * g_material.kd.xyz;
    float3 ao= aoTex.Sample(gsamLinearClamp, uv).xyz; // * g_material.kd.xyz;
    float3 alpha = separateAlpha.Sample(gsamLinearClamp, uv).xyz;
    float3 texNormal =
      normalize(tangentTex.Sample(gsamLinearClamp, uv) * 2.0f - 1.0f).xyz;

    float3 N = normalize(input.Normal.xyz);
    float3 T = normalize(input.tangent.xyz);
    float3 B = normalize(cross(N, T));
    float3x3 NTB;
    float3 normal = normalize(float3(texNormal.x * T) + (texNormal.y * B) + (texNormal.z * N));

    clip(alpha < 0.3f ? -1 : 1);

    float3 ldir = normalize(-g_dirLight.lightDir.xyz);
    float3 worldPos = input.worldPos.xyz;

    float3 finalColor =
		  albedo * saturate(dot(ldir, normal))*ao.x;

	  //compute specular
    float3 toEyeDir = normalize(g_cameraBuffer.position.xyz - worldPos);
    float3 halfWay = normalize(toEyeDir + ldir);
    float specularValue = saturate(dot(halfWay, normal));
    float specP = 250.0f;
    float3 highlightColor = float3(0.83, 0.55, 0.28);
    float3 finalSpecular = pow(specularValue, specP) * 0.3f* highlightColor;

	  //final color
    finalColor.x += finalSpecular.x;
    finalColor.y += finalSpecular.y;
    finalColor.z += finalSpecular.z;

    return float4(finalColor, 1.0f);


    /*
    float3 tanNorm = normalize(input.tangent.xyz);
    float3 lightV = normalize(-g_dirLight.lightDir.xyz);
    float3 viewNorm = normalize(g_cameraBuffer.position - input.worldPos).xyz;
    float cosTL = (dot(tanNorm, lightV));
    float sinTL = sqrt(1 - cosTL * cosTL);
    float diffuse = sinTL; // here sinTL is apparently larger than 0
    float3 col = albedo;

    float cosTRL = -cosTL;
    float sinTRL = sinTL;
    float cosTE = (dot(tanNorm, viewNorm));
    float sinTE = sqrt(1 - cosTE * cosTE);

    float coneAngleRadians = 10.0f * 3.14f / 180.0f;

				// primary highlight: reflected direction shift towards root (2 * coneAngleRadians)
    float cosTRL_root = cosTRL * cos(2 * coneAngleRadians) - sinTRL * sin(2 * coneAngleRadians);
    float sinTRL_root = sqrt(1 - cosTRL_root * cosTRL_root);
    float specular_root = max(0, cosTRL_root * cosTE + sinTRL_root * sinTE);

				// secondary highlight: reflected direction shifted toward tip (3*coneAngleRadians)
    float cosTRL_tip = cosTRL * cos(-3 * coneAngleRadians) - sinTRL * sin(-3 * coneAngleRadians);
    float sinTRL_tip = sqrt(1 - cosTRL_tip * cosTRL_tip);
    float specular_tip = max(0, cosTRL_tip * cosTE + sinTRL_tip * sinTE);

    float ks1 = 0.5f;
    float ks2 = 0.5f;
    float power = 12.0f;
				//col.x += specular_root + specular_tip;
    float spec1 = ks1 * pow(specular_root, power * 200.0f);
    float spec2 = ks2 * pow(specular_tip, power * 200.0f);
    col += spec1 + spec2;

    return float4(col   , 1);
    */

}
