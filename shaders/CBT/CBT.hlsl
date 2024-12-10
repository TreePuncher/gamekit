float3x3 M0()
{
	return float3x3(
		1.0f, 0.0f, 0.0f,
		0.5f, 0.0f, 0.5f,
		0.0f, 1.0f, 0.0f);
}


float3x3 M1()
{
	return float3x3(
		0.0f, 1.0f, 0.0f,
		0.5f, 0.0f, 0.5f,
		0.0f, 0.0f, 1.0f);
}


/************************************************************************************************/


uint FindMSB(in uint heapID)
{
	return firstbithigh(heapID);
}


uint FindLSB(in uint x)
{
	return firstbitlow(x);
}


/************************************************************************************************/


uint Depth(in uint maxDepth, uint Bit)
{
	return maxDepth - log2(Bit + 1);
}


uint ipow(in uint base, in uint exp)
{
	uint result = 1;
	for (;;)
	{
		if (exp & 1)
			result *= base;
		exp >>= 1;
		if (!exp)
			break;
		base *= base;
	}

	return result;
}


/************************************************************************************************/


uint32_t GetBitOffset(uint32_t idx, in uint32_t maxDepth)
{
	const uint32_t d_k		= FindMSB(idx);
	const uint32_t N_d_k	= maxDepth - d_k + 1;
	const uint32_t X_k		= ipow(2, d_k + 1) + idx * N_d_k;

	return X_k;
}


/************************************************************************************************/


uint32_t HeapToBitIndex(uint32_t k, uint32_t maxDepth) 
{
	return k * ipow(2, maxDepth - FindMSB(k)) - ipow(2, maxDepth);
}


/************************************************************************************************/


template<typename TY> uint32_t ReadValue(in TY CBTBuffer, uint32_t start, uint32_t bitWidth)
{
	const uint32_t	mask	= (uint32_t(1) << (bitWidth)) - 1;
	const uint32_t	idx		= start / 32;
	const uint32_t	offset	= start % 32;

	uint32_t value = (CBTBuffer[idx] >> offset) & mask;
	if (((start % 32) + bitWidth) > 32)
	{
		uint32_t bitsRemaining = ((start % 32) + bitWidth) % 32;
		uint32_t remainingBits = CBTBuffer[idx + 1] << (bitWidth - bitsRemaining);

		value |= (remainingBits & mask);
	}

	return value;
}


/************************************************************************************************/


void WriteValue(in RWStructuredBuffer<uint> CBTBuffer, uint32_t start, uint32_t bitWidth, uint32_t value)
{
	const uint32_t	idx		= start / 32;
	const uint32_t	offset	= start % 32;
	const uint32_t	mask	= ~(((uint32_t(0x01) << (bitWidth)) - 1) << offset);

	InterlockedAnd(CBTBuffer[idx], mask);
	InterlockedOr(CBTBuffer[idx], (~mask & (value << offset)));
	
	if (((start % 32) + bitWidth) > 32)
	{
		const uint32_t bitsRemaining	= ((start % 32) + bitWidth) % 32;
		const uint32_t remainingBits	= value >> (bitWidth - bitsRemaining);
		const uint32_t mask				= ~(((uint32_t(1) << (bitWidth)) - 1) >> (bitWidth - bitsRemaining));

		InterlockedAnd(CBTBuffer[idx + 1], mask);
		InterlockedOr(CBTBuffer[idx + 1], ~mask & remainingBits);
	}
}


/************************************************************************************************/


void SetBit(in RWStructuredBuffer<uint> CBTBuffer, uint32_t bitIdx, bool bit, uint32_t maxDepth)
{
	bitIdx += GetBitOffset(ipow(2, maxDepth), maxDepth);
	const uint32_t wordIdx			= bitIdx / (8 * sizeof(uint32_t));
	const uint32_t bitOffset		= bitIdx % (8 * sizeof(uint32_t));
	const uint32_t mask				= ~(0x01 << bitOffset);
	
	InterlockedAnd(CBTBuffer[wordIdx], mask);
	InterlockedOr(CBTBuffer[wordIdx], bit << bitOffset);
}


template<typename TY>
uint32_t GetHeapValue(in TY CBTBuffer, in uint32_t maxDepth, uint32_t heapIdx)
{
	const uint32_t bitIdx	= GetBitOffset(heapIdx, maxDepth);
	const uint32_t bitWidth = maxDepth - FindMSB(heapIdx) + 1;
	return ReadValue(CBTBuffer, bitIdx, bitWidth);
}


template<typename TY>
uint32_t DecodeNode(in TY CBTBuffer, in uint32_t maxDepth, int32_t leafID)
{
	uint32_t heapID = 1;
	uint32_t value	= GetHeapValue(CBTBuffer, maxDepth, heapID);

	for (int i = 0; i < maxDepth; i++)
	{
		if (value > 1)
		{
			uint32_t temp = GetHeapValue(CBTBuffer, maxDepth, 2 * heapID);
			if (leafID < temp)
			{
				heapID *= 2;
				value = temp;
			}
			else
			{
				leafID	= leafID - temp;
				heapID	= 2 * heapID + 1;
				value	= GetHeapValue(CBTBuffer, maxDepth, heapID);
			}
		}
		else
			break;
	}

	return heapID;
}


/************************************************************************************************/


template<typename TY>
uint32_t DecodeBitIdx(in TY cbt, in uint32_t maxDepth, int32_t idx)
{
	uint32_t heapID = 1;

	for (int i = 0; i < maxDepth; i++)
	{
		uint32_t temp = GetHeapValue(cbt, maxDepth, 2 * heapID);
		if (idx < temp)
			heapID *= 2;
		else
		{
			idx -= temp;
			heapID = 2 * heapID + 1;
		}
	}

	return HeapToBitIndex(heapID, maxDepth);
}


/************************************************************************************************/


bool GetBitValue(in uint heapID, in uint bitID)
{
	return (heapID >> bitID) & 0x01;
}


/************************************************************************************************/


float3x3 GetLEBMatrix(in const uint heapID)
{
	static const float3x3 M_b[] =
	{
		M0(),
		M1()
	};
	
	float3x3 m =
		float3x3(
			1, 0, 0,
			0, 1, 0,
			0, 0, 1);
	
	const uint d = FindMSB(heapID);
	for (int bitID = d - 2; bitID >= 0; --bitID)
	{
		int idx = GetBitValue(heapID, bitID);
		m = mul(M_b[idx], m);
	}
	
	return m;
}


/************************************************************************************************/


uint rule(uint n)
{
	return n > 1 ? n: 0;
}


uint4 GetNeighbors0(in const uint4 n)
{
	return uint4(
			rule(2 * n.w + 1), 
			rule(2 * n.z + 1), 
			rule(2 * n.y + 1), 
			2 * n.w);
}


uint4 GetNeighbors1(in const uint4 n)
{
	return uint4(
			rule(2 * n.z), 
			rule(2 * n.w), 
			rule(2 * n.x), 
			2 * n.w + 1);
}


struct Neighbors
{
	uint A;
	uint B;
	uint edge;
};

Neighbors LEBTriangleNeighbors(in const uint heapID)
{
	const uint	d = FindMSB(heapID);
	uint4		n = uint4(0, 0, 0, 1);
	
	for (int bitID = d - 1; bitID >= 0; --bitID)
	{
		uint b = GetBitValue(heapID, bitID);
		if (b == 0)
			n = GetNeighbors0(n);
		else 
			n = GetNeighbors1(n);
	}
	
	Neighbors OUT;
	OUT.A		= n.x;
	OUT.B		= n.y;
	OUT.edge	= n.z;
	return OUT;
}


Neighbors LEBQuadNeighbors(in const uint heapID)
{
	const uint d = FindMSB(heapID);
	uint4 n = (GetBitValue(heapID, FindMSB(heapID) - 1) == 0) ? uint4(0, 0, 3, 2) : uint4(0, 0, 2, 3);
	
	for (int bitID = d - 2; bitID >= 0; --bitID)
	{
		uint b = GetBitValue(heapID, bitID);
		if (b == 0)
			n = GetNeighbors0(n);
		else
			n = GetNeighbors1(n);
	}
	
	Neighbors OUT;
	OUT.A		= n.x;
	OUT.B		= n.y;
	OUT.edge	= n.z;
	return OUT;
}


template<typename TY>
bool IsLeafNode(in TY CBTBuffer, in const uint heapID, const in uint maxDepth)
{
	return (GetHeapValue(CBTBuffer, maxDepth, heapID) == 1);
}


template<typename TY>
bool IsDiamond(in TY CBTBuffer, in const uint heapID, const in uint maxDepth)
{
	return GetHeapValue(CBTBuffer, maxDepth, heapID) <= 2;
}


/************************************************************************************************/


uint BitFieldHeapID(uint32_t heapID, uint32_t maxDepth)
{
	uint32_t d = FindMSB(heapID);
	return heapID * ipow(2, maxDepth - d);
}


/************************************************************************************************/


template<typename TY>
Neighbors LEBQuadTrueNeighbors(in TY CBTBuffer, in const uint heapID, const in uint maxDepth)
{
	Neighbors n = LEBQuadNeighbors(heapID);
	if(!IsLeafNode(CBTBuffer, n.A, maxDepth))
		n.A = n.A * 2 + 1;
	if (!IsLeafNode(CBTBuffer, n.B, maxDepth))
		n.B = n.B * 2;
	if (!IsLeafNode(CBTBuffer, n.edge, maxDepth))
		n.edge = n.edge / 2;
	
	return n;
}


/************************************************************************************************/


template<typename TY>
bool IsLeafNode(in TY CBTBuffer, in const uint sibling, in const uint leftID, in const uint rightID, const in uint maxDepth)
{
	const bool a = IsLeafNode(CBTBuffer, sibling,	maxDepth);
	const bool b = IsLeafNode(CBTBuffer, leftID,	maxDepth);
	const bool c = IsLeafNode(CBTBuffer, rightID,	maxDepth);
	
	return a & b & c;
}


/************************************************************************************************/


void Split(RWStructuredBuffer<uint32_t> CBTBuffer, const in uint32_t heapID, const in uint32_t maxDepth)
{
	SetBit(CBTBuffer, HeapToBitIndex(heapID * 2 + 1, maxDepth), true, maxDepth);
		
	for (uint parentID = LEBQuadNeighbors(heapID).edge; parentID > 3;)
	{
		SetBit(CBTBuffer, HeapToBitIndex(parentID * 2, maxDepth), true, maxDepth);
		SetBit(CBTBuffer, HeapToBitIndex(parentID * 2 | 1, maxDepth), true, maxDepth);
		parentID = parentID / 2;
			
		SetBit(CBTBuffer, HeapToBitIndex(parentID * 2, maxDepth), true, maxDepth);
		SetBit(CBTBuffer, HeapToBitIndex(parentID * 2 | 1, maxDepth), true, maxDepth);
		parentID = LEBQuadNeighbors(parentID).edge;
	}
}


/************************************************************************************************/


void MergeNode(in RWStructuredBuffer<uint> CBTBuffer, uint32_t heapIdx, in const uint32_t maxDepth)
{
	SetBit(CBTBuffer, HeapToBitIndex(heapIdx | 1, maxDepth), false, maxDepth);
}


/************************************************************************************************/


void Merge(RWStructuredBuffer<uint32_t> CBTBuffer, const in uint32_t heapID, const in uint32_t maxDepth)
{
	const uint parent		= heapID / 2;
	const uint sibling		= LEBQuadNeighbors(heapID / 2).edge;
	
	if (heapID > 3 && 
		IsDiamond(CBTBuffer, sibling, maxDepth) &&
		IsDiamond(CBTBuffer, parent, maxDepth))
	{
		MergeNode(CBTBuffer, heapID, maxDepth);
		MergeNode(CBTBuffer, 2 * sibling, maxDepth);
	}
}


/**********************************************************************

Copyright (c) 2024 Robert May

Permission is hereby granted, free of charge, to any person obtaining a
copy of this software and associated documentation files (the "Software"),
to deal in the Software without restriction, including without limitation
the rights to use, copy, modify, merge, publish, distribute, sublicense,
and/or sell copies of the Software, and to permit persons to whom the
Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included
in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

**********************************************************************/
