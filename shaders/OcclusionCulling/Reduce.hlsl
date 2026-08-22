[[fk::BeginRootSignatureDef(id=rootsig)]]
[[fk::UAVStructured(id=source,  binding=0, type=uint64_t)]]
[[fk::UAVStructured(id=dest,	binding=1, type=uint64_t)]]

[[fk::PushConstants(num=1)]] 
{
    uint size;
};

[[fk::RootSignature(id=rootsig)]]
[numthreads(32, 1, 1)]
void Reduce(const uint3 ID : SV_DispatchThreadID)
{
	const uint x = ID.x;

	uint64_t a = source[x * 2 + 0];
	uint64_t b = source[x * 2 + 1];
	
	dest[x] = a | b;
	source[x * 2] = !a & b;
}
