
cbuffer constants : register(b0)
{
	uint bufferSize;
}

RWStructuredBuffer<uint> buffer : register(u0);

uint rand_xorshift(uint rng_state)
{
	// Xorshift algorithm from George Marsaglia's paper
	rng_state ^= (rng_state << 13);
	rng_state ^= (rng_state >> 17);
	rng_state ^= (rng_state << 5);
	return rng_state;
}


[numthreads(1024, 1, 1)]
void Initiate(const uint3 dispatchID : SV_DispatchThreadID)
{

	/*
	static const uint testSet[] = {
		41, 288, 292, 1869, 2995, 3902, 4639, 4827,
		4833, 4966, 5021, 5097, 5447, 5537, 5705,
		6270, 6334, 8723, 9040, 9161, 9961, 11478,
		12316, 13931, 13977, 14771, 15006, 15141, 15890,
		16118, 16944, 17035, 17421, 17673, 18756, 19169,
		19264, 19629, 19718, 20037, 22190, 22355, 22704,
		22929, 23281, 23655, 23986, 24084, 24370, 24393,
		24626, 25547, 25667, 26777, 26924, 26962, 27446,
		27529, 28253, 28703, 31322, 31673, 32391, 32662,

		153, 491, 778, 1842, 2082, 2306, 3035, 3548,
		4664, 5436, 5829, 6729, 6868, 7376, 7711, 8942,
		9741, 9894, 9930, 11323, 11538, 11840, 11942, 12382,
		12623, 12859, 13290, 14604, 15350, 15573, 15574, 15724,
		16512, 16541, 16827, 18467, 18636, 18716, 19072, 19895,
		19912, 19954, 21538, 21726, 22386, 22648, 23805, 23811,
		24464, 24767, 26299, 26308, 26500, 27644, 28145, 28745,
		29358, 29658, 30106, 30333, 31101, 31115, 32439, 32757,
	};
	*/

#if 0
	uint rng_state = dispatchID.x;
	buffer[dispatchID.x] = rand_xorshift(rng_state++);//
#elif 1
	buffer[dispatchID.x] = bufferSize - dispatchID.x;
#else
	buffer[dispatchID.x] = dispatchID.x;
#endif
}
