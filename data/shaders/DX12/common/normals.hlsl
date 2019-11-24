#ifndef NORMAL_HLSL
#define NORMAL_HLSL

float2 OctWrap(float2 v) {
  return (1.0 - abs(v.yx)) * (v.xy >= 0.0 ? 1.0 : -1.0);
}

float2 EncodeOctNormal(float3 n) {
  n /= (abs(n.x) + abs(n.y) + abs(n.z));
  n.xy = n.z >= 0.0f ? n.xy : OctWrap(n.xy);
  n.xy = n.xy * 0.5f + 0.5f;
  return n.xy;
}

float3 DecodeOctNormal(float2 f) {
  f = f * 2.0f - 1.0f;

  // https://twitter.com/Stubbesaurus/status/937994790553227264
  float3 n = float3(f.x, f.y, 1.0f - abs(f.x) - abs(f.y));
  float t = saturate(-n.z);
  n.xy += n.xy >= 0.0f ? -t : t;
  return normalize(n);
}

half4 encodeSphereTransform(half3 n, float3 view) {
  half p = sqrt(n.z * 8 + 8);
  return half4(n.xy / p + 0.5, 0, 0);
}

half3 decodeSphereTransform(half2 enc, float3 view) {
  half2 fenc = enc * 4 - 2;
  half f = dot(fenc, fenc);
  half g = sqrt(1 - f / 4);
  half3 n;
  n.xy = fenc * g;
  n.z = 1 - f / 2;
  return n;
}

float3 computeNormalFromNormalMap(float3 geoNormal, float3 geoTangent, float3 texNormal) {

  float3 N = normalize(geoNormal);
  float3 T = normalize(geoTangent);
  float3 B = normalize(cross(N, T));
  return normalize(float3(texNormal.x * T) + (texNormal.y * B) + (texNormal.z * N));
}

#endif