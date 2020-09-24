#include "../common/structures.hlsl"

RWByteAddressBuffer outData: register(u0, space3);
ConstantBuffer<GrassConfig> grassConfig: register(b1, space3);

static const int SUPPORT_DATA_ATOMIC_OFFSET0 = 8;
static const int SUPPORT_DATA_ATOMIC_OFFSET1 = 16;
static const int SUPPORT_DATA_ATOMIC_OFFSET2 = 24;
static const int SUPPORT_DATA_ATOMIC_OFFSET3 = 32;

[numthreads(1, 1, 1)]
void CS()
{
    //8 is the element size in out Data, which is 64 bit or 8 bytes
    outData.Store<int>(0, (outData.Load<int>(SUPPORT_DATA_ATOMIC_OFFSET0*8) * 15) * int(grassConfig.pointsPerTileLod.x));
    outData.Store<int>(4, 1);
    outData.Store<int>(8, 0);
    outData.Store<int>(12, 0);
    outData.Store<int>(16, (outData.Load<int>(SUPPORT_DATA_ATOMIC_OFFSET1*8) * 15) * int(grassConfig.pointsPerTileLod.y));
    outData.Store<int>(20, 1);
    outData.Store<int>(24, 0);
    outData.Store<int>(28, 0);
    outData.Store<int>(32, (outData.Load<int>(SUPPORT_DATA_ATOMIC_OFFSET2*8) * 15) * int(grassConfig.pointsPerTileLod.z));
    outData.Store<int>(36, 1);
    outData.Store<int>(40, 0);
    outData.Store<int>(44, 0);
    outData.Store<int>(48, (outData.Load<int>(SUPPORT_DATA_ATOMIC_OFFSET3*8) * 15) * int(grassConfig.pointsPerTileLod.w));
    outData.Store<int>(52, 1);
    outData.Store<int>(56, 0);
    outData.Store<int>(60, 0);


	//resetting first atomic
    outData.Store<int>(SUPPORT_DATA_ATOMIC_OFFSET0 *8 +0, 0);
    outData.Store<int>(SUPPORT_DATA_ATOMIC_OFFSET0 *8 +4, 0);
    outData.Store<int>(SUPPORT_DATA_ATOMIC_OFFSET0 *8 +8, 0);
    outData.Store<int>(SUPPORT_DATA_ATOMIC_OFFSET0 *8 +12, 0);
    outData.Store<int>(SUPPORT_DATA_ATOMIC_OFFSET0 *8 +16, 0);
    outData.Store<int>(SUPPORT_DATA_ATOMIC_OFFSET0 *8 +20, 0);

	
	//resetting first atomic
    outData.Store<int>(SUPPORT_DATA_ATOMIC_OFFSET1 *8 +0, 0);
    outData.Store<int>(SUPPORT_DATA_ATOMIC_OFFSET1 *8 +4, 0);
    outData.Store<int>(SUPPORT_DATA_ATOMIC_OFFSET1 *8 +8, 0);
    outData.Store<int>(SUPPORT_DATA_ATOMIC_OFFSET1 *8 +12, 0);
    outData.Store<int>(SUPPORT_DATA_ATOMIC_OFFSET1 *8 +16, 0);
    outData.Store<int>(SUPPORT_DATA_ATOMIC_OFFSET1 *8 +20, 0);


    //resetting third atomic
    outData.Store<int>(SUPPORT_DATA_ATOMIC_OFFSET2 *8 +0, 0);
    outData.Store<int>(SUPPORT_DATA_ATOMIC_OFFSET2 *8 +4, 0);
    outData.Store<int>(SUPPORT_DATA_ATOMIC_OFFSET2 *8 +8, 0);
    outData.Store<int>(SUPPORT_DATA_ATOMIC_OFFSET2 *8 +12, 0);
    outData.Store<int>(SUPPORT_DATA_ATOMIC_OFFSET2 *8 +16, 0);
    outData.Store<int>(SUPPORT_DATA_ATOMIC_OFFSET2 *8 +20, 0);
	
    //resetting fourth atomic
    outData.Store<int>(SUPPORT_DATA_ATOMIC_OFFSET3 *8 +0, 0);
    outData.Store<int>(SUPPORT_DATA_ATOMIC_OFFSET3 *8 +4, 0);
    outData.Store<int>(SUPPORT_DATA_ATOMIC_OFFSET3 *8 +8, 0);
    outData.Store<int>(SUPPORT_DATA_ATOMIC_OFFSET3 *8 +12, 0);
    outData.Store<int>(SUPPORT_DATA_ATOMIC_OFFSET3 *8 +16, 0);
    outData.Store<int>(SUPPORT_DATA_ATOMIC_OFFSET3 *8 +20, 0);
}
