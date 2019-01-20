/**********************************************************************

Copyright (c) 2015 - 2017 Robert May

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

#ifdef _WIN32
#pragma once
#pragma warning ( disable : 4244 )
#endif

#ifndef MATHUTTILS
#define MATHUTTILS


// Includes
#include "../buildsettings.h"
#include "containers.h"

#include <math.h>
#include <stdint.h>
#include <string>
#include <emmintrin.h>
#include <smmintrin.h>
#include <xmmintrin.h>
#include <initializer_list>

using namespace std;

namespace FlexKit
{
	/************************************************************************************************/
	

	template<typename TY_1, typename TY_2> auto floor	(TY_1 x, TY_2 y)	{ return ((x > y) ? y : x);   }
	template<typename TY_1, typename TY_2> auto min		(TY_1 x, TY_2 y)	{ return ((x > y) ? y : x);   }
	template<typename TY_1, typename TY_2> auto max		(TY_1 x, TY_2 y)	{ return ((x > y) ? x : y);   }
	template<typename TY_1, typename TY_2> auto fastmod (TY_1 x, TY_2 y)	{ return ((x < y) ? x : x%y); }


	/************************************************************************************************/


	static const double	pi = 3.141592653589793;
	static const unsigned int Matrix_Size = 16;

	static const size_t X = 0;
	static const size_t Y = 1;
	static const size_t Z = 2;

	template<size_t N>	inline int Factorial	() { return N * Factorial<N-1>(); }
	template<>			inline int Factorial<1>	() { return 1; }

	template<typename Ty> static Ty DegreetoRad( Ty deg ) { return (Ty)(deg * pi ) /180; }
	template<typename Ty> static Ty RadToDegree( Ty deg ) { return (Ty)(deg * 180) / pi; }

	template<typename TY, typename TY_C, typename FN>
	inline TY GetMax(TY_C C, FN READ)
	{
		TY M = 0;
		for (auto& c : C)
			M = MAX(M, READ(c));

		return M;
	}

	template<typename TY, typename TY_C, typename FN>
	inline TY GetMin(TY_C C, FN READ)
	{
		TY M = 0;
		for (auto& c : C)
			M = MIN(M, READ(c));

		return M;
	}

	template<typename TY, typename FN>
	inline TY Simpson_Integrator( TY A, TY B, int N, FN F_X)
	{
		TY Acc   = 0;
		TY S	 = (B - A) / N;

		TY S_1 = 0;
		TY S_2 = 0;

		for ( int I = 1; I < N; I += 2 )
			S_1 += F_X( A + I * S );
		for ( int I = 2; I < N; I += 2 )
			S_2 += F_X( A + I * S );

		Acc = F_X(A) + F_X(B) + (S_1 * 4) + (S_2 * 2);
		return Acc / 3 * S;
	}

	/*
	template<typename FN_FX, typename FN_FPRIME>
	float newtonmethod(FN_FX fX, FN_FPRIME fprimeX, float initial, const size_t Iterations)
	{
		float V = 0;

		for (unsigned int i = 0; i < Iterations; ++i)
			V = (V - (fx(V) / fprimeX(V)));

		return V;
	}
	*/

	template<typename TY>
	inline TY Saturate(TY A) { return max( 0.0f, min(1.0f, A)); }

	template<typename TY>
	inline bool CompareFloats(TY A, TY B, TY E) {
		return fabs(A - B) <= E;
	}


	
	inline const float& GetElement(const __m128& V, size_t idx)
	{
#ifdef WIN32
		return V.m128_f32[idx];
#else
		return V[idx];
#endif
	}

	inline float* GetElement_ptr(__m128& V, size_t idx)
	{
#ifdef WIN32
		return &V.m128_f32[idx];
#else
		return &V[idx];
#endif
	}

	inline float& GetElement_ref(__m128& V, size_t idx)
	{
#ifdef WIN32
		return V.m128_f32[idx];
#else
		return V[idx];
#endif
	}

	inline void SetElement(__m128& V, float X, size_t idx)
	{
#ifdef WIN32
		V.m128_f32[idx] = X;
#else
		V[idx] = X;
#endif
	}

	inline float GetFirst	(__m128& V)	{ return GetElement(V, 0); } // Should Return the X Component
	inline float GetLast	(__m128& V)	{ return GetElement(V, 2); } // SHould Return the W Component

	inline void SetFirst	(__m128& V, float X)	{ return SetElement(V, X, 0); }
	inline void SetLast		(__m128& V, float X)	{ return SetElement(V, X, 3); }


	/************************************************************************************************/


	template< unsigned int SIZE, typename TY = float >
	class Vect
	{
		typedef Vect<SIZE, TY> THISTYPE;
	public:
		Vect(){}

		Vect( TY n )
		{
			for( auto e : Vector )
				e = n;
		}

		template<typename TY_2>
		Vect( Vect<SIZE, TY_2> in)
		{
			for (size_t I=0; I<SIZE; ++I)
				Vector[I] = in[I];
		}

		Vect( std::initializer_list<TY> il )
		{
			size_t itr = 0;
			for( auto n : il )
			{
				Vector[itr++] = n;
				if( itr > SIZE )
					return;
			}
		}

		template< typename TY_i >
		Vect<SIZE, TY>	operator *( TY_i scaler )
		{
			Vect<SIZE, TY>	out;
			size_t i = 0;
			for( auto element : Vector )
			{
				out[i] = element * scaler;
				++i;
			}
			return out;
		}

		template< typename TY_i >
		Vect<SIZE, TY>	operator *( Vect<SIZE, TY_i> rhs )
		{
			Vect<SIZE, TY>	out;
			for (auto i = 0; i < SIZE; ++i)
				out[i]= Vector[i] * rhs[i];
			return out;
		}

		template< typename TY_i >
		Vect<3, TY>	Cross( Vect<3, TY_i> rhs )
		{
			Vect<SIZE, TY> out;
			for( size_t i = 0; i < SIZE; ++i )
				out[i] = ( Vector[(1+i)%SIZE] * rhs[(2+i)%SIZE] ) - ( rhs[(1+i)%SIZE] * Vector[(2+i)%SIZE] );
			return out;
		}

		template< typename TY_i >
		TY Dot( const Vect<SIZE, TY_i>& rhs ) const
		{
			TY dotproduct = 0;
			for( size_t i = 0; i < SIZE; ++i )
				dotproduct += rhs[i] * Vector[i];

			return dotproduct;
		}

		template< typename TY_i >
		TY Dot( const Vect<SIZE, TY_i>* rhs_ptr )
		{
			auto& rhs = *rhs_ptr;
			Vect<SIZE> products;
			for( size_t i = 0; i < SIZE; ++i )
				products[i] += rhs[i] * Vector[i];

			return products.Sum();
		}

		TY Norm( unsigned int exp = 2 )
		{
			TY sum = 0;
			for( auto element : Vector )
			{
				TY product = 0;
				for( size_t i = 0; i < SIZE; i++ )
					product *= element;
				sum += product;	
			}

			return pow( sum, 1/SIZE );
		}

		TY Magnitude()
		{
			TY sum = 0;
			for( auto element : Vector )
			{
				sum += element * element;	
			}

			return sqrt( sum );
		}

		TY& operator []( size_t index )
		{
			return Vector[index];
		}

		const TY& operator []( size_t index ) const
		{
			return Vector[index];
		}

		TY Sum() const
		{
			TY sum = 0;
			for( auto element : Vector )
				sum += element;
			return sum;
		}

		TY Product() const
		{
			TY Product = 1;
			for( auto element : Vector )
				Product *= element;
			return Product;
		}

		template<typename TY_2>
		THISTYPE operator + (const Vect<SIZE, TY_2>& in)
		{
			THISTYPE temp = *this;
			for (auto I = 0; I < SIZE; ++I)
				temp[I] += in[I];
			
			return temp;
		}

		template<typename TY_2>
		THISTYPE& operator += (const Vect<SIZE, TY_2>& in)
		{
			for (auto I = 0; I < SIZE; ++I)
				Vector[I] += in[I];
			
			return *this;
		}

		template<typename TY_2>
		THISTYPE operator - (const Vect<SIZE, TY_2>& in)
		{
			THISTYPE temp = *this;
			for (auto I = 0; I < SIZE; ++I)
				temp[I] -= in[I];
			
			return temp;
		}

		template<typename TY_2>
		THISTYPE& operator -= ( const Vect<SIZE, TY_2>& in )
		{
			for (auto I = 0; I < SIZE; ++I)
				Vector[I] -= in[I];
			
			return *this;
		}

		template<typename TY_2>
		THISTYPE& operator = (const Vect<SIZE, TY_2>& in)
		{
			for (auto I = 0; I < SIZE; ++I)
				Vector[I] = in[I];
			
			return *this;
		}

		template<typename TY_2>
		THISTYPE operator / (TY_2 in)
		{
			THISTYPE temp = *this;
			for (auto I = 0; I < SIZE; ++I)
				temp[I] /= in;

			return temp;
		}

		template<typename TY_2>
		THISTYPE& operator /= (TY_2 in)
		{
			for (auto I = 0; I < SIZE; ++I)
				Vector[I] /= in;

			return *this;
		}

		template<typename TY_2>
		THISTYPE operator /= (const Vect<SIZE, TY_2> in)
		{
			THISTYPE Out;
			for (auto I = 0; I < SIZE; ++I)
				Out[I] = Vector[I] / in[I];

			return Out;
		}

		template<typename TY_2>
		bool operator == (const Vect<SIZE, TY_2>& in)
		{
			bool res = true;
			for (auto I = 0; I < SIZE; ++I)
				res = res & (Vector[I] == in[I]);

			return res;
		}

		template<typename TY_2>
		THISTYPE& operator = ( const std::initializer_list<TY_2>& il )
		{
			size_t itr = 0;
			for( auto n : il )
			{
				Vector[itr++] = n;
				if( itr > SIZE )
					break;
			}
			return *this;
		}

		static THISTYPE Zero()
		{
			THISTYPE zero;
			for( auto element : zero.Vector )
				element = 0;

			return zero;
		}

		operator TY* () { return Vector; }

		TY Vector[SIZE];
	};

	/************************************************************************************************/

	typedef Vect<2> Vect2;
	typedef Vect<3> Vect3;
	typedef Vect<4> Vect4;
	
	typedef Vect<3, double> double3;
	typedef Vect<4, double> double4;
	
	typedef Vect<2, size_t> uint2;
	typedef Vect<3, size_t> uint3;
	typedef Vect<4, size_t> uint4;
	typedef Vect<4, unsigned short> uint4_16;
	typedef Vect<4, uint32_t> uint4_32;

	typedef Vect<2, int> int2;
	typedef Vect<3, int> int3;
	typedef Vect<4, int> int4;

	/************************************************************************************************/

	union float2
	{
	public:
		float2() noexcept : x(0), y(0) {}
		float2( const float X, const float Y ) noexcept
		{
			x = X;
			y = Y;
		}

		float2(const float in_f) noexcept
		{
			x = in_f;
			y = in_f;
		}

		inline bool		operator == ( const float2& rhs ) const { return ( rhs.x == x && rhs.y == y ) ? true : false; }



		inline float&	operator[] ( size_t i )
		{	
		#ifdef _DEBUG
			FK_ASSERT( i < 2 );
		#endif
			return i ? y : x;
		}

		inline float operator[] ( size_t i ) const
		{
#ifdef _DEBUG
			FK_ASSERT( i < 2 );
#endif
			return i ? y : x;
		}

		inline float2 operator + ( const float2& a ) const { return float2( this->x + a.x,	this->y + a.y );				}
		inline float2 operator + ( const float   a ) const { return float2( x + a, y + a );								}
		inline float2 operator - ( const float2& a ) const { return float2( x - a.x, y - a.y );							}
		inline float2 operator - ( const float   a ) const { return float2( this->x - a,	this->y - a );					}
		inline float2 operator * ( const float2& a ) const { return float2( this->x * a.x,	this->y * a.y );				}
		inline float2 operator * ( const float   a ) const { return float2( this->x * a,	this->y * a );					}
		inline float2 operator / ( const float2& a ) const { return float2( this->x / a.x,	this->y / a.y );				}
		inline float2 operator / ( const float   a ) const { return float2( this->x / a,	this->y / a );					}
		inline float2 operator % ( const float2& a ) const { return float2( std::fmod(x, a.x), std::fmod(y, a.y));	}

		inline float2 operator = (const float2& a) { x = a.x; y = a.y; return *this; }

		inline float2 operator *= (const float2& a) 
		{ 
			*this = *this * a;
			return *this; 
		}


		inline float2&	operator -= (const float2& rhs)
		{
			*this = *this - rhs;
			return *this;
		}


		inline float2&	operator += (const float2& rhs)
		{
			*this = *this + rhs;
			return *this;
		}


		inline bool operator > (const float2& rhs) const
		{
			return (x > rhs.x) && (y > rhs.y);
		}


		inline bool operator < (const float2& rhs) const
		{
			auto temp = !(*this > rhs);
			return temp;
		}


		inline void Add( const float2& lhs, const float2& rhs )
		{
			x = lhs.x + rhs.x;
			y = lhs.y + rhs.y;
		}

		operator float* ()			{ return XY; }

		struct
		{
			float x, y;
		};

		float Product() { return x * y; }
		float Sum()		{ return x + y;	}

		float Magnitude() 
		{ 
			auto V_2 = (*this * *this);
			return  sqrt(V_2.Sum());
		}


		float XY[2];

	};

	template<typename TY_> float2 operator + (const float2& LHS, const Vect<2, TY_>& RHS){ return{ LHS.x + RHS[0], LHS.y + RHS[1] };}
	template<typename TY_> float2 operator - (const float2& LHS, const Vect<2, TY_>& RHS){ return{ LHS.x - RHS[0], LHS.y - RHS[1] };}
	template<typename TY_> float2 operator * (const float2& LHS, const Vect<2, TY_>& RHS){ return{ LHS.x * RHS[0], LHS.y * RHS[1] };}
	template<typename TY_> float2 operator / (const float2& LHS, const Vect<2, TY_>& RHS){ return{ LHS.x / RHS[0], LHS.y / RHS[1] };}

	inline float2 operator * (const float   lhs, const float2 rhs) { return float2(lhs) * rhs; }


	/************************************************************************************************/

	inline float DotProduct2(const float* lhs, float* rhs)
	{
#if USING(FASTMATH)
		// Windows
		__m128 l   = _mm_loadr_ps(lhs);
		__m128 r   = _mm_loadr_ps(rhs);
		__m128 res = _mm_dp_ps(l, r, 0x06);
		return GetFirst(res);
#else
#if WIN32
		return (lhs.m128_f32[0] * rhs.m128_f32[0]) + (lhs.m128_f32[1] * rhs.m128_f32[1]) + (lhs.m128_f32[2] * rhs.m128_f32[2]);
#else
		static_assert(false, "Not Implemented!");
		// Slow Path Not Implmented
#endif
#endif
	}

	inline float DotProduct3(const __m128& lhs, const __m128& rhs)
	{
#if USING(FASTMATH)
		__m128 res = _mm_dp_ps(lhs, rhs, 0xFF);
		return GetFirst(res);
#else
		return ( lhs.m128_f32[0] * rhs.m128_f32[0] ) + ( lhs.m128_f32[1] * rhs.m128_f32[1] ) + ( lhs.m128_f32[2] * rhs.m128_f32[2] );
#endif
	}

	inline float DotProduct4(const __m128& lhs, const __m128& rhs)
	{
#if USING(FASTMATH)
		__m128 res = _mm_dp_ps(lhs, rhs, 0xFF);
		return GetFirst(res);
#else
		return ( lhs.m128_f32[0] * rhs.m128_f32[0] ) + ( lhs.m128_f32[1] * rhs.m128_f32[1] ) + ( lhs.m128_f32[2] * rhs.m128_f32[2] );
#endif
	}

	inline __m128 CrossProductSlow(const __m128 lhs, const __m128 rhs)
	{
		__m128 out = _mm_set1_ps(0);
		SetElement( out, (GetElement(lhs, 1) * GetElement(rhs, 2)) - (GetElement(lhs, 2) * GetElement(rhs, 1)), 0 );
		SetElement( out, (GetElement(lhs, 2) * GetElement(rhs, 0)) - (GetElement(lhs, 0) * GetElement(rhs, 2)), 1 );
		SetElement( out, (GetElement(lhs, 0) * GetElement(rhs, 1)) - (GetElement(lhs, 1) * GetElement(rhs, 0)), 2 );
		return out;
	}
	

	inline __m128 CrossProduct( __m128& a, __m128& b )
	{
#if USING(FASTMATH)
		__m128 temp1 = _mm_mul_ps(_mm_shuffle_ps(a, a, 0x01 | 0x02 << 2 | 0x00 << 4 | 0x00 << 6), _mm_shuffle_ps(b, b, 0x02 | 0x00 << 2 | 0x01 << 4 | 0x00 << 6));
		__m128 temp2 = _mm_mul_ps(_mm_shuffle_ps(a, a, 0x02 | 0x00 << 2 | 0x01 << 4 | 0x00 << 6), _mm_shuffle_ps(b, b, 0x01 | 0x02 << 2 | 0x00 << 4 | 0x00 << 6));
		__m128 res	 = _mm_sub_ps(temp1, temp2);
		SetLast(res, 0.0f);

		return res;
#else
		return CrossProductSlow(a, b);
#endif
	}


	union Quaternion;
	FLEXKITAPI Quaternion GrassManProduct( Quaternion& lhs, Quaternion& rhs );

	/************************************************************************************************/

#ifdef WIN32
	__declspec( align( 16 ) ) union float3
#else
	__attribute__((aligned(16))) union float3
#endif
	{
	public:
		float3() noexcept {}

		template<class TY = float>
		float3(std::initializer_list<TY> il) noexcept
		{
			pfloats = _mm_set1_ps(0.0f);
			auto i = il.begin();
			for( auto count = 0; i != il.end() && count < 4; ++count, ++i)
				SetElement(pfloats, (float)*i, count);
		}

		inline float3( std::initializer_list<float> il )
		{
			auto n  = il.begin();
			pfloats = _mm_set_ps1(0);

			for (size_t itr = 0; n != il.end() && itr < 3; ++itr, ++n)
				SetElement(pfloats, *n, itr);

		}
		

#if USING( FASTMATH )

		inline float3 ( float val )						{ pfloats = _mm_set_ps1(val);					}
		inline float3 ( float X, float Y, float Z )		{ pfloats = _mm_set_ps(0.0f, Z, Y, X);			}
		inline float3 ( const float2 in, float Z )		{ pfloats = _mm_setr_ps(in.x, in.y, Z, 0.0f);	}
		inline float3 ( const float3& a )				{ pfloats = a.pfloats;							}
		inline float3 ( const __m128& in )				{ pfloats = in;									}


#else

		inline float3 ( float val )						{ x = y = z = val;								}
		inline float3 ( float X, float Y, float Z )		{ x = X, y = Y, z = Z;							}
		inline float3 ( const float3& a )				{ x = a.x, y = a.y, z = a.z;					}
		inline float3 ( const __m128& in )				{ pfloats = in;									}
		inline float3 ( const float2 in )				{ x = in.x, y = in.y;							}

#endif

		inline float3 xy() { return float3(x, y, 0.0f); }
		inline float3 xz() { return float3(x, 0.0f, z); }
		inline float3 yz() { return float3(0.0f, y, z); }


		inline float3& operator = (float F)
		{
			pfloats = _mm_set_ps1(F);
			return *this;
		}


		inline float& operator[] ( const size_t index )		  { return *GetElement_ptr(pfloats, index); }
		inline float operator[]  ( const size_t index )	const { return GetElement(pfloats, index); }

		// Operator Overloads
		inline float3 operator - ()							  { return _mm_mul_ps(pfloats, _mm_set_ps1(-1));	}
		inline float3 operator - ()	const					  { return _mm_mul_ps(pfloats, _mm_set_ps1(-1));	}
		inline float3 operator + ( const float& rhs )	const { return _mm_add_ps(pfloats, _mm_set_ps1(rhs));	}
		inline float3 operator + ( const float3& rhs )	const { return _mm_add_ps(pfloats, rhs);				}


		inline float3& operator += ( const float3& rhs )
		{
#if USING(FASTMATH)
			pfloats = _mm_add_ps(pfloats, rhs);
#else
			x += rhs.x;
			y += rhs.y;
			z += rhs.z;
#endif
			return *this;
		}


		inline float3 operator - ( const float rhs ) const
		{
#if USING(FASTMATH)
			return _mm_sub_ps(pfloats, _mm_set1_ps(rhs));
#else
			return float3( x - rhs, y - rhs, z - rhs );
#endif
		}

		
		inline float3& operator -= ( const float3& rhs )
		{
#if USING(FASTMATH)
			pfloats = _mm_sub_ps(pfloats, rhs.pfloats);
#else
			x -= rhs.x;
			y -= rhs.y;
			z -= rhs.z;
#endif
			return *this;
		}

		
		inline bool operator == ( const float3& rhs ) const
		{
			if( rhs.x == x )
				if( rhs.y == y )
					if( rhs.z == z )
						return true;
			return false;
		}

		
		inline float3 operator - ( const float3& a ) const
		{
#if USING(FASTMATH)
			return _mm_sub_ps(pfloats, a);
#else
			return float3( x - a.x, y - a.y, z - a.z );
#endif
		}
		
		static inline bool Compare(const float3& lhs, const float3& rhs, float ep = 0.001f)
		{
			float3 temp = lhs - rhs;
			return (temp.x < ep) && (temp.y < ep)  && (temp.z < ep);
		}
		
		inline float3 operator *	( const float3& a )		const 
		{ 
#if USING(FASTMATH)
			return _mm_mul_ps(a.pfloats, pfloats);
#else
			return float3( x * a.x, y * a.y, z * a.z );	
#endif
		}


		inline float3 operator *	( const float a )		const 
		{	
#if USING(FASTMATH)
			return _mm_mul_ps(_mm_set1_ps(a), pfloats);
#else
			return float3( x * a, y * a, z * a );		
#endif

		}
		
		inline float3& operator *=	( const float3& a )		
		{
			x *= a.x;
			y *= a.y;
			z *= a.z;
			return *this;
		}

		inline float3& operator *=	( float a )
		{
			x *= a;
			y *= a;
			z *= a;
			return *this;
		}

		inline float3 operator / ( const float& a ) const
		{
			return float3( x / a, y / a, z / a );
		}

		inline float3& Scale( float S )
		{
#if USING(FASTMATH)
			pfloats = _mm_mul_ps(pfloats, _mm_set1_ps(S));
#else
			(*this)[0] *= S;
			(*this)[1] *= S;
			(*this)[2] *= S;
#endif
			return *this;
		}

		inline float3 inverse()
		{
			return float3( -x, -y, -z );
		}

		// Identities
		inline float3 cross( float3 rhs ) 
		{
			return CrossProduct( pfloats, rhs.pfloats );
		}
		 

		inline float3 distance( float3 &b )
		{
			return b.magnitude() - magnitude();
		}

		//inline float dot( const float3 &b ) {
		//	return ( x * b.x )+( y * b.y )+( z * b.z );
		//}

		inline float dot( const float3 &b ) const 
		{
			return DotProduct3(pfloats, b.pfloats);
		}


		// Slow due to the use of a square root
		inline float magnitude() const 
		{
#if USING(FASTMATH)
			__m128 r = _mm_mul_ps(pfloats, pfloats);
			r = _mm_hadd_ps(r, r);
			r = _mm_hadd_ps(r, r);
			float x = GetFirst(r);
			//return _mm_mul_ps(_mm_rsqrt_ps(r), _mm_set1_ps(r.m128_f32[0])).m128_f32[0];
			auto M = _mm_mul_ps(_mm_rsqrt_ps(r), _mm_set1_ps(x));
			float Res = GetLast(M);

			return Res;
#else
			return ::std::sqrt( ( x*x ) + ( y*y ) + ( z*z ) );
#endif
		}


		inline float magnitudesquared() const 
		{
#if USING(FASTMATH)
			__m128 r = _mm_mul_ps(pfloats, pfloats);
			r = _mm_hadd_ps(r, r);
			r = _mm_hadd_ps(r, r);
			return GetLast(r);
#else		
			return ( x*x ) + ( y*y ) + ( z*z );
#endif
		}

		// Slow uses Square roots
		inline void normalize() 
		{
#if USING(FASTMATH)
			//pfloats.m128_f32[3] = 0;
			__m128 sq = _mm_mul_ps(pfloats, pfloats);
			sq = _mm_hadd_ps(sq, sq);
			sq = _mm_hadd_ps(sq, sq);
			sq = _mm_mul_ps(_mm_rsqrt_ps(sq), pfloats);
			pfloats = sq;
#else
			float3 temp = *this;
			pfloats = temp / magnitude();
#endif
		}

		inline float3 normal() 
		{
#if USING(FASTMATH)
			//pfloats.m128_f32[3] = 0;
			__m128 sq = _mm_mul_ps(pfloats, pfloats);
			sq = _mm_hadd_ps(sq, sq);
			sq = _mm_hadd_ps(sq, sq);
			sq = _mm_mul_ps(_mm_rsqrt_ps(sq), pfloats);
			return sq;
#else
			float3 temp = *this;
			pfloats = temp / magnitude();
#endif
		}


		operator __m128 () const	 {return pfloats;}
		inline float* toFloat3_ptr() {return reinterpret_cast<float*>( &pfloats );}

		operator Vect3 () const		 {return { x, y, z };};
		operator float2() const		 {return { x, y };};


		struct
		{
			float x, y, z, PAD;
		};

		__m128	pfloats;

		static float3 Load(float* a)
		{
			auto temp = _mm_loadu_ps(a);
			return float3(_mm_loadr_ps((float*)&temp));
		}

		private:
		static float3 SetVector(float in)
		{
			float3 V;
			V.x = in;
			V.y = in;
			V.z = in;
			return V;
		}

	};

	std::ostream& operator << (std::ostream& stream, float3 xyz)
	{
		stream << "{" << xyz.x << "," << xyz.y << "," << xyz.z << "}";
		return stream;
	}

	const float3 BLACK	= float3(0.0f, 0.0f, 0.0f);
	const float3 WHITE	= float3(1.0f, 1.0f, 1.0f);
	const float3 RED	= float3(1.0f, 0.0f, 0.0f);
	const float3 BLUE	= float3(0.0f, 0.0f, 1.0f);
	const float3 GREEN	= float3(0.0f, 1.0f, 0.0f);
	const float3 PURPLE	= float3(1.0f, 0.0f, 1.0f);


	float saturate(float x)
	{
		return min(max(x, 0.0f), 1.0f);
	}


	float3 saturate(float3 v)
	{
		float3 out = v;
		v.x = min(max(v.x, 0.0f), 1.0f);
		v.y = min(max(v.y, 0.0f), 1.0f);
		v.z = min(max(v.z, 0.0f), 1.0f);

		return out;
	}

	/************************************************************************************************/

	inline float3 operator* ( float s, float3 V )
	{
#if USING(FASTMATH)
		return _mm_mul_ps(V, _mm_set1_ps(s));
#else
		return V*s;
#endif
	}

	inline float3 RotateVectorAxisAngle( float3 N, float a, float3 V ) { return V*cos(a) + (V.dot(N) * N * (1-cos(a)) + (N.cross(V)*sin(a)));	}
	
#ifdef WIN32
	__declspec(align(16)) union float4
#else
	__attribute__((aligned(16))) union float4
#endif
	{
	public:
		float4() {}

		inline float4( float r ) 
		{
#if USING(FASTMATH)
			pFloats = _mm_set1_ps(r);
#else
			x = r;
			y = r;
			z = r;
			w = r;
#endif
		}

		inline float4( float X, float Y, float Z, float W  ) 
		{
#if USING(FASTMATH)
			pFloats = _mm_set_ps(W, Z, Y, X);
#else
			x = X;
			y = Y;
			z = Z;
			w = W;
#endif
		}

		inline float4( const float3& V, float W  ) 
		{
			x = V[0];
			y = V[1];
			z = V[2];
			w = W;
		}

		inline float4( __m128 in ) : pFloats(in){}

		inline float4( const float2& V1, const float2& V2  ) 
		{
			x = V1[0];
			y = V1[1];
			z = V2[0];
			w = V2[1];
		}

		inline float4( std::initializer_list<float> il )
		{
			auto n = il.begin();
			for (size_t itr = 0; n != il.end() && itr < 4; ++itr, ++n)
				SetElement(pFloats, *n, itr);
		}

		inline operator float*				()					{ return (float*)&pFloats;} 
		inline operator const float* const	()	const			{ return (float*)&pFloats;}
		inline float& operator[] ( const size_t index )			{ return GetElement_ref( pFloats, index); }
		inline float operator[]  ( const size_t index )	const	{ return GetElement( pFloats, index); }
		inline operator __m128	 ()						const	{ return pFloats;} 

		inline float4 operator+ ( const float4& a ) const 
		{
#if USING(FASTMATH)
			return _mm_add_ps(pFloats, a.pFloats);
#else
			return float4(	x + a.x, 
							y + a.y, 
							z + a.z, 
							w + a.w );
#endif
		}

		inline float4 operator+ ( const float a ) const
		{
#if USING(FASTMATH)
			return _mm_add_ps(pFloats, _mm_set1_ps(a));
#else
			return float4(	x + a, 
							y + a, 
							z + a, 
							w + a );
#endif
		}

		inline float4 operator- ( const float4& a ) const
		{
#if USING(FASTMATH)
			return _mm_sub_ps(pFloats, a);
#else
			return float4(	x - a.x, 
							y - a.y, 
							z - a.z, 
							w - a.w );
#endif
		}

		inline float4 operator- ( const float a ) const
		{
#if USING(FASTMATH)
			return _mm_sub_ps(pFloats, _mm_set1_ps(a));
#else
			return float4(	x - a, 
							y - a, 
							z - a, 
							w - a );
#endif
		}

		//inline float4 operator* ( const float4 a ) const
		//{
#if USING(FASTMATH)
		//	return _mm_mul_ps(pFloats, a);
#else
			return float4(	x * a.x, 
							y * a.y, 
							z * a.z, 
							w * a.w );
#endif
		//}

		inline float4 operator* ( const float a ) const
		{
#if USING(FASTMATH)
			return _mm_mul_ps(pFloats, _mm_set1_ps(a));
#else
			return float4(	x * a, 
							y * a, 
							z * a, 
							w * a );
#endif
		}

		inline float4 operator += ( const float4& a )
		{
#if USING(FASTMATH)
			pFloats = _mm_add_ps(pFloats, a);
#else
			return float4(	x + a.x, 
							y + a.y, 
							z + a.z, 
							w + a.w );
#endif
			return pFloats;
		}

		inline float4 operator / ( const float4& a ) const
		{
#if USING(FASTMATH)
			return _mm_div_ps(pFloats, a);
#else
			return float4(	x / a.x, 
							y / a.y, 
							z / a.z, 
							w / a.w );
#endif
		}

		inline float4 operator / ( const float a ) const
		{
#if USING(FASTMATH)
			return _mm_div_ps(pFloats, _mm_set1_ps(a));
#else
			return float4(	x / a, 
							y / a, 
							z / a, 
							w / a );
#endif
		}

		inline float4 operator % ( const float4& a ) const
		{
			return float4(	std::fmod( x, a.x ), 
							std::fmod( y, a.y ),
							std::fmod( z, a.z ),
							std::fmod( w, a.w ) );
		}

		struct
		{
			float x, y, z, w;
		};

		struct
		{
			float r, g, b, a;
		};

		float3 xyz()
		{
			return { x, y, z, 0 };
		}

		operator Vect4 ()		{ return{ x, y, z, w }; };
		operator Vect4 () const { return{ x, y, z, w }; };

		__m128 pFloats;
	};

	inline float F4Dot(float4 rhs, float4 lhs)						{ return DotProduct4(lhs, rhs); }
	inline float4 operator* (float4 lhs, float4 rhs)				{ return _mm_mul_ps(lhs, rhs); }
	inline float4 F4MUL		(const float4 lhs, const float4 rhs)	{ return _mm_mul_ps(lhs, rhs); }


	/************************************************************************************************/

#ifdef WIN32
	__declspec(align(16)) union Quaternion
#else
	__attribute__((aligned(16))) union Quaternion
#endif
	{
	public:
		inline Quaternion() {}
		inline Quaternion(__m128 in) { floats = in; }


		inline explicit Quaternion( const float3& vector, float scaler )
		{
#if USING(FASTMATH)
			floats = _mm_set_ps(scaler, vector[2], vector[1],vector[0]);
#else
			x = vector[0];
			y = vector[1];
			z = vector[2];
			w = scaler;
#endif
		}


		inline Quaternion( float X, float Y, float Z, float W )
		{
#if USING(FASTMATH)
			floats = _mm_set_ps(W, Z, Y, X);
#else
			x = X;
			y = Y;
			z = Z;
			w = W;
#endif
		}


		inline Quaternion( std::initializer_list<float> il )
		{
			auto n = il.begin();
			for (size_t itr = 0; n != il.end() && itr < 4; ++itr, ++n)
				SetElement( floats, *n, itr);
		}


		inline explicit Quaternion( float* in )
		{
			_mm_store_ps( in, floats );
		}


		inline explicit Quaternion( const Quaternion& in ) :
			floats( in.floats )	{}


		template<typename TY>
		Quaternion( std::initializer_list<TY> il )
		{
			auto IV = il.begin();
			floats = _mm_set_ps(0.0f, 0.0f, 0.0f, 0.0f);
			for( size_t I = 0; I < FlexKit::min(il.size(), 4); ++I  )
			{
				float V = *IV;
				SetElement(floats,V, I);
				IV++;
			}
		}

		inline explicit Quaternion( float dX, float dY, float dZ ) // Degrees to Quat
		{
			FlexKit::Quaternion X, Y, Z;
			X.Zero();
			Y.Zero();
			Z.Zero();;
			
			float dX1_2 = DegreetoRad( dX / 2 );
			float dY1_2 = DegreetoRad( dY / 2 );
			float dZ1_2 = DegreetoRad( dZ / 2 );

			X.x = std::sin( dX1_2 );
			X.w = std::cos( dX1_2 );
			Y.y = std::sin( dY1_2 );
			Y.w = std::cos( dY1_2 );
			Z.z = std::sin( dZ1_2 );
			Z.w = std::cos( dZ1_2 );

			( *this ) = X * Y * Z;
		}

		inline Quaternion& operator *= ( Quaternion& rhs )
		{
			(*this) = GrassManProduct(*this, rhs);
			return (*this);
		}

		inline Quaternion& operator = ( Quaternion& rhs )
		{
			floats = rhs.floats;
			return (*this);
		}

		inline Quaternion& operator = ( __m128 rhs )
		{
			floats = rhs;
			return (*this);
		}


		inline Quaternion operator * (Quaternion q)
		{
			return GrassManProduct(*this, q);
		}


		inline float& operator [] ( const size_t index )		{ return GetElement_ref	(floats, index); }
		inline float operator  [] ( const size_t index ) const	{ return GetElement		(floats, index); }

		inline operator		  float* ()			{ return (float*)&floats; }
		inline operator const float* () const	{ return (float*)&floats; }
		inline operator		  __m128 ()			{ return floats; }
		inline operator const __m128 () const	{ return floats; }

		template< typename Ty_2 >
		inline Quaternion& operator = ( const float4& rhs )
		{
			x = rhs.x;
			y = rhs.y;
			z = rhs.z;
			w = rhs.w;

			return (*this);
		}

		inline Quaternion Conjugate() 
		{
			Quaternion Conjugate;
#if USING(FASTMATH)
			Conjugate = _mm_mul_ps(_mm_set_ps(1, -1, -1, -1), floats);
#else
			Conjugate.x = -x;
			Conjugate.y = -y;
			Conjugate.z = -z;
			Conjugate.w = w;
#endif
			return Conjugate;
		}
		inline Quaternion Inverse() { return  Conjugate(); }
		
		inline float dot(Quaternion rhs) 
		{ 
			return V().dot(rhs.V()) + w * rhs.w; 
		}

		inline Quaternion operator / (float denom)		{return { x / denom , y / denom , z / denom , w / denom };}
		inline Quaternion operator + (Quaternion RHS)	{return { x + RHS.x, y + RHS.y, z + RHS.z, w + RHS.w }; }

		inline float3 XYZ() const { return float3(x, y, z); }
		inline float3 V()   const { return float3(x, y, z); }

		inline float Magnitude() const
		{
#if USING( FASTMATH )
			__m128 q2 = _mm_mul_ps(floats, floats);
			q2 = _mm_hadd_ps(q2, q2);
			q2 = _mm_hadd_ps(q2, q2);
			return GetLast(q2);
#else
			return x * x + y * y + z * z + w * w;
#endif
		}

		inline Quaternion normalize() 
		{
			float mag2 = Magnitude();
			if( mag2 != 0 && ( fabs( mag2 - 1.0f ) > .00001f ) )
			{
#if USING(FASTMATH)

				__m128 rsq = _mm_rsqrt_ps(_mm_set1_ps(mag2));
				floats = _mm_mul_ps(rsq, floats);
#else
				float mag  = sqrt( mag2 );
				w = w / mag;
				x = x / mag;
				y = y / mag;
				z = z / mag;
#endif
			}
			return *this;
		}


		inline Quaternion normalized() const
		{
			float mag2 = Magnitude();
			__m128 Res;
			if (mag2 != 0 && (fabs(mag2 - 1.0f) > .00001f))
			{
#if USING(FASTMATH)

				__m128 rsq = _mm_rsqrt_ps(_mm_set1_ps(mag2));
				Res = _mm_mul_ps(rsq, floats);
#else
				float mag = sqrt(mag2);
				w = w / mag;
				x = x / mag;
				y = y / mag;
				z = z / mag;
#endif
			}
			return Res;
		}


		inline void Zero() 
		{
#if USING(FASTMATH)
			floats = _mm_set1_ps(0);
#else
			x = 0;
			y = 0;
			z = 0;
			w = 0;
#endif
		}

		static Quaternion Identity()
		{
			Quaternion Q(0.0f, 0.0f, 0.0f, 1.0f);
			return Q;
		}

		struct
		{
			float x, y, z, w;
		};

		 __m128	floats;
	};


	/************************************************************************************************/


	inline float3		operator * (Quaternion P, float3 V)
	{
		auto v = P.XYZ() * -1;
		auto vXV = v.cross(V);
		auto ret = float3(V + (vXV * (2 * P.w)) + (v.cross(vXV) * 2));

		return ret;
	}


	inline Quaternion operator * (Quaternion Q, float scaler)
	{
		__m128 r = Q;
		__m128 s = _mm_set1_ps(scaler);
		return _mm_mul_ps(r, s);
	}


	/************************************************************************************************/


	inline Quaternion Qlerp(Quaternion P, Quaternion Q, float W)
	{
		float W_Inverse = 1 - W;
		Quaternion Qout = P * W_Inverse + Q * W;
		Qout.normalize();

		return Qout;
	}


	inline Quaternion Slerp(Quaternion P, Quaternion Q, float W)
	{
		Quaternion Qout;

		float dot = P.V().dot(Q.V());
		if (dot < 0)
		{
			dot = -dot;
		}
		else
			Qout = Q;

		if (dot < 0.95f)
		{
			float Angle = acosf(dot);
			Quaternion A = P * sinf((1 - W) * Angle);
			Quaternion B = Q * sinf(W * Angle);
			Qout = (A + B) / sinf(Angle);
			return Qout.normalize();
		}

		Qout = Qlerp(P, Q, W);
		return Qout;
	}


	/************************************************************************************************/


	enum XYZ
	{
		x = 0,
		y = 0,
		z = 0
	};


	/************************************************************************************************/
	

	inline float3	Vect3ToFloat3(Vect3 R3) {return {R3[0], R3[1], R3[2]};}
	inline Vect3	Float3ToVect3(float3 R3){return {R3[0], R3[1], R3[2]};}
	
	inline float4	Vect4ToFloat4(Vect4  R4){return {R4[0], R4[1], R4[2], R4[3]};}
	inline Vect4	Float4ToVect4(float3 R4){return {R4[0], R4[1], R4[2], R4[3]};}


	/************************************************************************************************/


	inline float Vect3FDot(const Vect3& lhs, const Vect3& rhs)
	{
		auto temp1 = _mm_set_ps(lhs.Vector[3], lhs.Vector[2], lhs.Vector[1], lhs.Vector[0]);
		auto temp2 = _mm_set_ps(rhs.Vector[3], rhs.Vector[2], rhs.Vector[1], rhs.Vector[0]);

		__m128 res = _mm_dp_ps(temp1, temp2, 0xFF);

		return GetFirst(res);
	}


	/************************************************************************************************/


	inline float Vect4FDot( const Vect4& lhs, const Vect4& rhs )
	{
		auto temp1 = _mm_loadr_ps(lhs.Vector); //_mm_set_ps(lhs.Vector[3], lhs.Vector[2], lhs.Vector[1], lhs.Vector[0]);
		auto temp2 = _mm_loadr_ps(rhs.Vector); //_mm_set_ps(rhs.Vector[3], rhs.Vector[2], rhs.Vector[1], rhs.Vector[0]);

		__m128 res = _mm_dp_ps(temp1, temp2, 0xFF);

		return GetFirst(res);
	}


	/************************************************************************************************/

	// Row Major
	template< const int ROW, const int COL, typename Ty = float >
#ifdef WIN32
	class __declspec(align(16))  Matrix
#else
	__attribute__((aligned(16))) union Matrix
#endif
	{
	public:
		Matrix(){}


		Matrix<ROW, COL> operator*(const float rhs)
		{
			Matrix<ROW, COL> out = *this;

			for (auto& c : out.matrix)
				for(auto& e : c)
					e = e * rhs;

			return out;
		}


		template< const int RHS_COL >
		Matrix<ROW, RHS_COL>	operator*( const Matrix<ROW, RHS_COL>& rhs )
		{
			static_assert( ROW == RHS_COL, "ROW AND RHS COLS DO NOT MATCH" );
			Matrix<ROW, RHS_COL> out;
			auto transposed = rhs.Transpose();

			for( size_t i = 0; i < ROW; ++i )
			{
				const auto v = *((Vect<4>*)matrix[i]);
				for( size_t i2 = 0; i2 < COL; ++i2 )
				{
					const auto v2 = *((Vect<4>*)transposed[i2]);
					out[i][i2] = v.Dot(v2);
				}
			}
			return out;
		}

		Matrix<3, 3> operator*(const Matrix<3, 3>& rhs)
		{
			Matrix<3, 3> out;
			auto transposed = rhs.Transpose();

			for (size_t i = 0; i < ROW; ++i)
			{
				const auto v = *((Vect<3>*)matrix[i]);
				for (size_t i2 = 0; i2 < COL; ++i2)
				{
					const auto v2 = transposed[i2];
					out[i][i2] = Vect3FDot(v, v2);
				}
			}
			return out;
		}


		Matrix<4, 4> operator*( const Matrix<4, 4>& rhs )
		{
			Matrix<4, 4> out;
			auto transposed = rhs.Transpose();

			for( size_t i = 0; i < ROW; ++i )
			{
				const auto v = *((Vect<4>*)matrix[i]);
				for( size_t i2 = 0; i2 < COL; ++i2 )
				{
					const auto v2 = transposed[i2];
					out[i][i2] = Vect4FDot(v, v2);
				}
			}
			return out;
		}

		Vect<ROW>&		operator[] ( size_t i )			{ return *((Vect<ROW>*)matrix[i]); }
		const float*	operator[] ( size_t i ) const	{ return matrix[i]; }

		static inline Matrix<ROW, COL> Identity()
		{
			Matrix<ROW, COL> m = Zero();

			for( size_t i = 0; i < ROW; i++ )
				m[i][i] = 1;

			return m;
		}

		static inline Matrix<ROW, COL> Zero()
		{
			Matrix<ROW, COL> m;
			for (size_t I = 0; I < ROW; ++I)
				for (size_t II = 0; II < COL; II++)
					m[I][II] = 0.0;

			return m;
		}

		Matrix<ROW, COL> Transpose() const
		{
			Matrix<ROW, COL> m_transposed;

			for( size_t col = 0; col < COL; ++col )
				for( size_t row = 0; row < ROW; ++row )
					m_transposed[col][row] = matrix[row][col];

			return m_transposed;
		}

		Ty matrix[COL][ROW];
	};

	/************************************************************************************************/


	inline float2 Mulfloat2(Matrix<2, 2, float>& LHS, float2& RHS)
	{
		float2 Out;
		auto transposed = LHS.Transpose();

		for (size_t I = 0; I < 2; ++I)
			Out[I] = DotProduct2(transposed.matrix[I], RHS);

		return Out;
	}

	inline float3 Mulfloat3(Matrix<3, 3, float>& LHS, float3& RHS)
	{
		float3 Out;
		auto transposed = LHS.Transpose();
		__m128 Temp;
		for (size_t I = 0; I < 2; ++I) {
			Temp = _mm_set_ps(0, transposed.matrix[I][2], transposed.matrix[I][1], transposed.matrix[I][0]);
			//Temp = _mm_loadu_ps(transposed.matrix[I]);
			//Temp = _mm_shuffle_ps(Temp, Temp, 0x6c);
			Out[I] = DotProduct3(Temp, RHS);
		}

		return Out;
	}


	/************************************************************************************************/


	typedef FlexKit::Matrix<4,4> float4x4;
	typedef FlexKit::Matrix<3,3> float3x3;

	float4x4 TranslationMatrix(float3 POS)
	{
		float4x4 Out = float4x4::Identity();
		
		Out[3] = float4(POS, 1);

		return Out;
	}

	FLEXKITAPI inline float CalcMatrixTrace( float in[Matrix_Size] )
	{
		float sum = 0.0f;
		for( size_t itr = 0; itr < Matrix_Size; itr++ )
			for( size_t itr_2 = 0;itr_2 < 3; itr_2+= 4, itr += itr_2 )
				sum += in[itr];
		return sum;
	}



	/************************************************************************************************/


	template<typename TY>
	inline TY Lerp(const TY A, const TY B, float t){ return A * (1.0f - t) + t*B; }

	template<typename TY>
	inline TY lerp(const TY A, const TY B, float t) { return A * (1.0f - t) + t * B; }

	namespace Conversion
	{
		template< typename Ty >
		static float3 toVector3( Ty& Convert ) { return Vect3( Convert.x, Convert.y, Convert.z ); }

		template< typename Ty > Ty Vect3To( const Vect3& Convert )	{ return Ty{Convert[0], Convert[1], Convert[2]}; }
		template< typename Ty >	Ty Vect4To( const Vect4& Convert )	{ return Ty{Convert[0], Convert[1], Convert[2], Convert[3]}; }

		template< typename Ty_Out, typename Ty_2>	Ty_Out Vect2To( const Ty_2& Convert) { return Ty_Out{ Convert[0], Convert[1] }; }

		template<typename TY>
		float2 Vect2TOfloat2(Vect<2, TY> Convert) { return float2{ (float)Convert[0], (float)Convert[1] }; }
	}


	/************************************************************************************************/


	inline float4 operator * (const float4x4& LHS, const float4 rhs)
	{// TODO: FAST PATH
		Vect<4, float> out;
		Vect4 Temp = rhs;

		auto T = LHS.Transpose();
		for (size_t i = 0; i < 4; ++i)
		{
			const auto v = T[i];
			out[i] = v.Dot(Temp);
		}

		return Conversion::Vect4To<float4>(out);
	}


	/************************************************************************************************/


	FLEXKITAPI int			Exp( int32_t Number, uint32_t exp );
	FLEXKITAPI Quaternion	MatrixToQuat( Matrix<4,4>& );
	FLEXKITAPI void			NumberToString( int32_t n, string& _Dest );
	FLEXKITAPI int			Testing();

	FLEXKITAPI void printfloat2(const float2& in);
	FLEXKITAPI void printfloat3(const float3& in);
	FLEXKITAPI void printfloat4(const float4& in);
	FLEXKITAPI void printQuaternion(const Quaternion in);


	template<typename TY>
	float dot(const TY lhs, const TY rhs)
	{
		return 0;
	}


	template<>
	float dot<float3>(const float3 lhs, const float3 rhs)
	{
		return DotProduct3(lhs, rhs);
	}

	template<>
	float dot<float4>(const float4 lhs, const float4 rhs)
	{
		return DotProduct4(lhs, rhs);
	}

	/************************************************************************************************/


	/*
	inline FlexKit::float4x4 TranslationMatrix(float3 XYZ)
	{
		FlexKit::float4x4 out(float4x4::Identity());
		out[3][0] = XYZ.x;
		out[3][1] = XYZ.y;
		out[3][2] = XYZ.z;
		out[3][3] = 1.0f;

		return out;
	}
	*

	/************************************************************************************************/


	inline FlexKit::float4x4 Quaternion2Matrix(Quaternion q)
	{
		float4x4 m1, m2;

		// Assign m1
		m1[0][0] =  q[3];
		m1[0][1] =  q[2];
		m1[0][2] = -q[1];
		m1[0][3] =  q[0];

		m1[1][0] = -q[2];
		m1[1][1] =  q[3];
		m1[1][2] =  q[0];
		m1[1][3] =  q[1];

		m1[2][0] =  q[1];
		m1[2][1] = -q[0];
		m1[2][2] =  q[3];
		m1[2][3] =  q[2];
		
		m1[3][0] = -q[0];
		m1[3][1] = -q[1];
		m1[3][2] = -q[2];
		m1[3][3] =  q[3];

		// Assign m2
		m2[0][0] =  q[3];
		m2[0][1] =  q[2];
		m2[0][2] = -q[1];
		m2[0][3] = -q[0];
		
		m2[1][0] = -q[2];
		m2[1][1] =  q[3];
		m2[1][2] =  q[0];
		m2[1][3] = -q[1];
		
		m2[2][0] =  q[1];
		m2[2][1] = -q[0];
		m2[2][2] =  q[3];
		m2[2][3] = -q[2];
		
		m2[3][0] =  q[0];
		m2[3][1] =  q[1];
		m2[3][2] =  q[2];
		m2[3][3] =  q[3];

		return m1 * m2;
	}

	
	/************************************************************************************************/


	inline __m128 SSE_CopySign(__m128 sign, __m128 abs)
	{
		const uint32_t M1 = (1 << 31);
		const uint32_t M2 =~(1 << 31);

		__m128 Sgn = _mm_and_ps(sign, _mm_castsi128_ps(_mm_set1_epi32(M1)));
		__m128 Abs = _mm_and_ps(abs, _mm_castsi128_ps(_mm_set1_epi32(M2)));

		__m128 res = _mm_or_ps(Sgn, Abs);
		return res;
	}


	/************************************************************************************************/
	

	inline Quaternion Matrix2Quat(const float4x4& M)
	{
#if USING(FASTMATH)
		Quaternion Q
		(
			1.0f + M[0][0] - M[1][1] - M[2][2], 
			1.0f - M[0][0] + M[1][1] - M[2][2], 
			1.0f - M[0][0] - M[1][1] + M[2][2], 
			1.0f + M[0][0] + M[1][1] + M[2][2]
		);

		__m128 Temp1 = _mm_max_ps(Q, _mm_set1_ps(0.0f));
		Temp1 = _mm_sqrt_ps(Temp1);
		Temp1 = _mm_mul_ps(Temp1, _mm_set1_ps(0.5f));

		// Copy Sign
		__m128 Temp3 = _mm_set_ps(GetFirst(Temp1),		M[0][1], M[2][0], M[1][2]);
		__m128 Temp4 = _mm_set_ps(0.0f,					M[1][0], M[0][2], M[2][1]);
		__m128 Temp5 = _mm_sub_ps(Temp3, Temp4);
		__m128 res = SSE_CopySign(Temp5, Temp1);

		return res;
#else 

		Quaternion Q
		{
			sqrtf( max(1.0f + M[0][0] - M[1][1] - M[2][2], 0.0f))/2, 
			sqrtf( max(1.0f - M[0][0] + M[1][1] - M[2][2], 0.0f))/2, 
			sqrtf( max(1.0f - M[0][0] - M[1][1] + M[2][2], 0.0f))/2, 
			sqrtf( max(1.0f + M[0][0] + M[1][1] + M[2][2], 0.0f))/2
		};
		return Quaternion
		{
			_copysignf(Q.x, M[1][2] - M[2][1]),
			_copysignf(Q.y, M[2][0] - M[0][2]),
			_copysignf(Q.z, M[0][1] - M[1][0]),
			Q.w
		};
#endif
	}


	/************************************************************************************************/


	inline float4x4 Vector2RotationMatrix(const float3& Forward, const float3& Up, const float3 &Right)
	{
		float4x4 Out = float4x4::Identity();
		Out[0][0] = Forward.x;
		Out[0][1] = Forward.y;
		Out[0][2] = Forward.z;
		Out[0][3] = 0;

		Out[1][0] = Up.x;
		Out[1][1] = Up.y;
		Out[1][2] = Up.z;
		Out[1][3] = 0;

		Out[2][0] = Right.x;
		Out[2][1] = Right.y;
		Out[2][2] = Right.z;
		Out[2][3] = 0;

		return Out;
	}


	/************************************************************************************************/


	inline Quaternion Vector2Quaternion(const float3& Forward, const float3& Up, const float3& Right)
	{
		return Matrix2Quat(Vector2RotationMatrix(Forward, Up, Right));
	}


	/************************************************************************************************/


	// NEED TO OPTIMIZE THIS SOMEDAY
	inline float4x4 Q2M(Quaternion Q)
	{
		float4x4 out;
		out[0] = float4{ 1 - Q.y*Q.y - 2 * Q.z*Q.z, 2 * Q.x*Q.y + 2 * Q.z*Q.w, 2 * Q.x * Q.z - 2 * Q.y * Q.w, 0  };
		out[1] = float4{ 2 * Q.x*Q.y - 2 * Q.z*Q.w, 1 - Q.x*Q.x - 2 * Q.z*Q.z, 2 * Q.y * Q.z - 2 * Q.x * Q.w, 0  };
		out[2] = float4{ 2*Q.x*Q.z + 2*Q.y*Q.w, 2*Q.y*Q.z - 2*Q.x*Q.w, 1 - 2 * Q.x * Q.x - 2 * Q.y*Q.y,		 0	 };
		out[3] = float4{ 0,						0,						0,								1		 };

		return out;
	}


	/************************************************************************************************/


	inline Quaternion PointAt(float3 A, float3 B)
	{
		float3 Dir		= (B - A).magnitude();
		Dir = {Dir.z, Dir.y, -Dir.x};

		float3 UpV		= (0, 1, 0);
		float3 DirXUpV	= Dir.cross(UpV);

		return Vector2Quaternion(Dir, DirXUpV.cross(Dir), DirXUpV);
	}


	/************************************************************************************************/

}

#endif