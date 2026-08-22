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
	dest[x] = source[x * 2 + 0] | source[x * 2 + 1];
}
