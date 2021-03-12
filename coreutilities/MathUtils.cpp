/**********************************************************************

Copyright (c) 2015 - 2019 Robert May

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

#include "..\pch.h"
#include "buildsettings.h"

#include "MathUtils.h"
#include "static_vector.h"

#if USING(FASTMATH)
#include <smmintrin.h>
#endif


namespace FlexKit
{
	inline int Exp( int32_t Number, uint32_t exp )
	{
		if( exp != 0) {
			int Origin = Number;
			while( exp > 1 ) {
				Number = Number * Origin;
				exp = exp - 1;
			}
		} else {
			Number = 1;
		}
		return Number;
	}


	Quaternion GrassManProduct(const Quaternion& P, const Quaternion& Q) noexcept
	{
		float3 PV = P.XYZ();
		float3 QV = Q.XYZ();

		float3 OP = PV.cross(QV);
		float  IP = DotProduct3(PV, QV);

		float3  Vout = (PV * Q.w) + (QV * P.w) + OP;
		float   Wout = P.w * Q.w - IP;
		
		return Quaternion{Vout, Wout};
	}


	/************************************************************************************************/

	void NumberToString( int32_t n, std::string& _Dest )
	{
        std::string Tmp;
		uint32_t pwr = 9;
		uint32_t exp = 1 * Exp( 10, pwr );
		uint32_t remainder = n;

		while( pwr >= 0 ) {
			uint32_t Test = remainder/exp;
			switch( remainder/exp ) {
			case 1:
				Tmp.push_back( '1' );
				break;
			case 2:
				Tmp.push_back( '2' );
				break;
			case 3:
				Tmp.push_back( '3' );
				break;
			case 4:
				Tmp.push_back( '4' );
				break;
			case 5:
				Tmp.push_back( '5' );
				break;
			case 6:
				Tmp.push_back( '6' );
				break;
			case 7:
				Tmp.push_back( '7' );
				break;
			case 8:
				Tmp.push_back( '8' );
				break;
			case 9:
				Tmp.push_back( '9' );
				break;
			case 0:
				Tmp.push_back( '0' );
				break;
			default:
				break;
			}
			pwr--;
			exp = Exp( 10, pwr);
			if( exp != 1) {
				remainder = n%exp;
			} else {
				remainder = n%10;
			}
		}
		_Dest = Tmp;
	}

	/************************************************************************************************/

	void printfloat2(const float2& in)
	{
		printf("{%f, %f}", in[0], in[1]);
	}

	/************************************************************************************************/

	void printfloat3(const float3& in)
	{
		printf("{%f, %f, %f}", in[0], in[1], in[2]);
	}

	/************************************************************************************************/
	
	void printfloat4(const float4& in)
	{
		printf("{%f, %f, %f, %f}", in[0], in[1], in[2], in[3]);
	}

	/************************************************************************************************/

	void printQuaternion(const Quaternion in)
	{
		printf("{%f, %f, %f, %f}", in[0], in[1], in[2], in[3]);
	}


	/************************************************************************************************/

}
