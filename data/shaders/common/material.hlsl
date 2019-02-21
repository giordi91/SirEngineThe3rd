// material buffer
struct PhongMaterial {
  float4 kd;
  float4 ka;
  float4 ks;
  // todo padd shiness into ka.w
  float shiness;
  float reflective;
  float padding2;
  float padding3;
};