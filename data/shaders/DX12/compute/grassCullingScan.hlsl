StructuredBuffer<int> g_input: register(t0,space3);
RWStructuredBuffer<int> g_output: register(u1,space3);

[numthreads( 16, 1, 1 )]
void CS( uint3 id : SV_DispatchThreadID)
{
    g_output[id.x] = g_input[id.x] + 1;
}