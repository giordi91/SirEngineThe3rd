


//this function constructs a tangent matrix to transform the normal stored in the normal map
//into world space normal that we can use for lighting
vec3 sampleNormalMap( sampler normalSampler, texture2D normalTexture, 
vec3 geometricNormal, vec3 tangent, vec2 uv )
{
   //sampling the texture, we convert the value from range 0-1 to range -1 to 1
   vec3 normalMapVec = normalize(texture (sampler2D (normalTexture, normalSampler), uv).xyz*2.0f - 1.0f);
   //build the matrix
   vec3 T = tangent;
   vec3 B = normalize(cross(geometricNormal, T));
   mat3x3 NTB;
   //apply matrix transformation
   vec3 normal = normalize(vec3(normalMapVec.x * T) + (normalMapVec.y * B) + (normalMapVec.z * geometricNormal));
   return normal;
}