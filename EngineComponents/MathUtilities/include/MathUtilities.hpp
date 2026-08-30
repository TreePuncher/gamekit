#pragma once
#pragma warning(disable : 4201)
#include <BuildSettings.hpp>

// Includes

#include <bit>
#include <bitset>
#include <cstring>
#include <initializer_list>
#include <limits>
#include <cmath>
#include <ostream>
#include <random>
#include <ranges>
#include <tuple>
#include <type_traits>

#include <simde/simde-common.h>
#if defined(__x86_64__) || defined(_M_X64)
#include <immintrin.h>
#include <simde/x86/avx2.h>
#include <simde/x86/fma.h>
#elif defined(__aarch64__) || defined(_M_ARM64)
#include <simde/arm/neon.h>
#endif

using std::views::iota;
using std::views::zip;

#ifdef __clang__
using std::views::iota;
using std::views::zip;

auto enumerate(auto&& container)
{
	return zip(iota(0), container);
}

#else

using std::views::enumerate;

#endif

#ifndef _MM_SHUFFLE
#define _MM_SHUFFLE(D, C, B, A) (A | (B<<2) | (C<<4) | (D<<6))
#endif

#ifdef max
#undef max
#endif

#ifdef min
#undef min
#endif

namespace FlexKit
{   /************************************************************************************************/

	
	template<class T>
	concept Scaler_t = std::is_integral_v<T> || std::is_floating_point_v<T> || std::is_convertible_v<T, int> || std::is_convertible_v<T, float> || std::is_convertible_v<T, double>;


	template<size_t>
	struct Vector_t_helper{};
	
	template<class T>
	concept Vector_t = !Scaler_t<T> && requires(T t)
	{
		t[0];
		t.size();
		//typename Vector_t_helper<t.size()>;
	}  && Scaler_t<decltype(std::declval<T>()[0])>;

	template<typename TY_V>
	concept VectorSIMD_t = requires(TY_V v)
	{
		{ v.GetSIMD(0) } -> std::convertible_to<simde__m128>;
	}	&& Vector_t<TY_V>;


	template<typename TY_V>
	concept VectorSIMDDouble_t = requires(TY_V v)
	{
		{ v.GetSIMD256(0) } -> std::convertible_to<simde__m256d>;
	}&& Vector_t<TY_V>;

	template<Scaler_t TY_1, Scaler_t TY_2> [[nodiscard]] constexpr auto Floor		(const TY_1 x, const TY_2 y) noexcept { return (((TY_1)x > (TY_1)y) ? y : x);   }
	template<Scaler_t TY_1, Scaler_t TY_2> [[nodiscard]] constexpr auto Min			(const TY_1 x, const TY_2 y) noexcept { return (((TY_1)x > (TY_1)y) ? y : x);   }
	template<Scaler_t TY_1, Scaler_t TY_2> [[nodiscard]] constexpr auto Max			(const TY_1 x, const TY_2 y) noexcept { return (((TY_1)x > (TY_1)y) ? x : y);   }
	template<Scaler_t TY_1, Scaler_t TY_2> [[nodiscard]] constexpr auto Fastmod		(const TY_1 x, const TY_2 y) noexcept { return (((TY_1)x < (TY_1)y) ? x : x%y); }

	template<Vector_t TY_V> [[nodiscard]] constexpr auto Floor(const TY_V x, const TY_V y) noexcept
	{
		TY_V out;
		for (size_t I = 0; I < TY_V::size(); I++)
		   out[I] = Floor(x[I], y[I]);

		return out;
	}

	template<Vector_t TY_V> [[nodiscard]] constexpr auto Min(const TY_V x, const TY_V y) noexcept
	{
		TY_V out;
		for (size_t I = 0; I < TY_V::size(); I++)
			out[I] = Min(x[I], y[I]);

		return out;
	}

	template<Vector_t TY_V> [[nodiscard]] constexpr auto Max(const TY_V x, const TY_V y) noexcept
	{
		TY_V out;
		for (size_t I = 0; I < TY_V::size(); I++)
			out[I] = Max(x[I], y[I]);

		return out;
	}

	template<Vector_t TY_V> [[nodiscard]] constexpr auto Fastmod(const TY_V x, const TY_V y) noexcept
	{
		TY_V out;
		for (size_t I = 0; I < TY_V::size(); I++)
			out[I] = Fastmod(x[I], y[I]);

		return out;
	}


	/************************************************************************************************/

	
	// Source: https://stackoverflow.com/questions/8622256/in-c11-is-sqrt-defined-as-constexpr
	template <typename T>
	constexpr T sqrt_helper(T x, T lo, T hi)
	{
		if (lo == hi)
			return lo;

		const T mid = (lo + hi + 1) / 2;

		if (x / mid < mid)
			return sqrt_helper<T>(x, lo, mid - 1);
		else
			return sqrt_helper(x, mid, hi);
	}

	template <typename T>
	constexpr T ct_sqrt(T x)
	{
		return sqrt_helper<T>(x, 0, x / 2 + 1);
	}


	/************************************************************************************************/


	template<typename TY>
	TY ipow(TY base, TY exp)
	{
		TY result = 1;
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


	inline static const double	pi	= 3.141592653589793;
	inline static const float	fpi = 3.141592653589793f;
	inline static const unsigned int Matrix_Size = 16;

	template<size_t N>	inline int Factorial	() { return N * Factorial<N-1>(); }
	template<>			inline int Factorial<1>	() { return 1; }

	template<typename Ty> constexpr Ty DegreetoRad( Ty deg ) noexcept { return (Ty)(deg * pi) /180; }
	template<typename Ty> constexpr Ty RadToDegree( Ty deg ) noexcept { return (Ty)(deg * 180.0f) / (float)pi; }


	template<typename TY, typename TY_C, typename FN>
	TY GetMax(TY_C C, FN READ)
	{
		TY M = 0;
		for (auto& c : C)
			M = MAX(M, READ(c));

		return M;
	}

	template<typename TY, typename TY_C, typename FN>
	TY GetMin(TY_C C, FN READ)
	{
		TY M = 0;
		for (auto& c : C)
			M = MIN(M, READ(c));

		return M;
	}

	template<typename TY, typename FN>
	TY Simpson_Integrator( TY A, TY B, int N, FN F_X)
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

		
	/************************************************************************************************/


	inline simde__m128 SSE_CopySign(simde__m128 sign, simde__m128 abs) noexcept
	{
		const uint32_t M1 = (1u << 31);
		const uint32_t M2 =~(1u << 31);

		const simde__m128 Sgn = simde_mm_and_ps(sign, simde_mm_castsi128_ps(simde_mm_set1_epi32(M1)));
		const simde__m128 Abs = simde_mm_and_ps(abs, simde_mm_castsi128_ps(simde_mm_set1_epi32(M2)));
		const simde__m128 res = simde_mm_or_ps(Sgn, Abs);

		return res;
	}


	/************************************************************************************************/


	inline simde__m128 SSE_ABS(simde__m128 abs) noexcept
	{
		return SSE_CopySign(simde_mm_set_ps1(1), abs);
	}


	/************************************************************************************************/


	template<typename TY>
	inline TY Saturate(TY A) { return Max( 0.0f, Min(1.0f, A)); }


	template<typename TY>
	inline bool CompareFloats(TY A, TY B, TY E) { return fabs(A - B) <= E;	}


	template<typename TY>
	bool VectorCompare(TY A, TY B, float E) noexcept { return (A - B).magnitudeSq() < E * E; }


	inline const	float* GetArray_ptr_const(const simde__m128& V)  noexcept { return reinterpret_cast<const float*>(&V); }
	inline			float* GetArray_ptr(simde__m128& V)              noexcept { return reinterpret_cast<float*>(&V); }

	inline const	float& GetElement(const simde__m128& V, const size_t idx) noexcept { return GetArray_ptr_const(V)[idx]; }

	inline			float* GetElement_ptr(simde__m128& V, const size_t idx ) noexcept { return GetArray_ptr(V) + idx; }
	inline			float& GetElement_ref(simde__m128& V, const size_t idx)  noexcept { return GetArray_ptr(V)[idx]; }

	inline			void SetElement	(simde__m128& V, float X, const size_t idx) noexcept { GetArray_ptr(V)[idx] = X;	}

	inline			float GetFirst	(const simde__m128& V) noexcept { return GetElement(V, 0); } // Should Return the X Component
	inline			float GetLast	(const simde__m128& V) noexcept { return GetElement(V, 2); } // SHould Return the W Component

	inline			void SetFirst	(simde__m128& V, const float X) noexcept { return SetElement(V, X, 0); }
	inline			void SetLast	(simde__m128& V, const float W) noexcept { return SetElement(V, W, 3); }


	/************************************************************************************************/


	inline float DotProduct2(const float* lhs, const float* rhs) noexcept
	{
		simde__m128 l = simde_mm_loadr_ps(lhs);
		simde__m128 r = simde_mm_loadr_ps(rhs);
		simde__m128 res = simde_mm_dp_ps(l, r, 0x06);
		return GetFirst(res);
	}

	inline float DotProduct3(const simde__m128& lhs, const simde__m128& rhs) noexcept
	{
		simde__m128 res = simde_mm_dp_ps(lhs, rhs, 0x77);
		return simde_mm_cvtss_f32(res);
	}

	inline float DotProduct4(const simde__m128& lhs, const simde__m128& rhs) noexcept
	{
		simde__m128 res = simde_mm_dp_ps(lhs, rhs, 0xFF);
		return GetFirst(res);
	}

	inline simde__m128 CrossProductSlow(const simde__m128 lhs, const simde__m128 rhs) noexcept
	{
		simde__m128 out = simde_mm_set1_ps(0);
		SetElement(out, (GetElement(lhs, 1) * GetElement(rhs, 2)) - (GetElement(lhs, 2) * GetElement(rhs, 1)), 0);
		SetElement(out, (GetElement(lhs, 2) * GetElement(rhs, 0)) - (GetElement(lhs, 0) * GetElement(rhs, 2)), 1);
		SetElement(out, (GetElement(lhs, 0) * GetElement(rhs, 1)) - (GetElement(lhs, 1) * GetElement(rhs, 0)), 2);

		return out;
	}

	inline simde__m128 CrossProduct(const simde__m128& a, const simde__m128& b) noexcept
	{
		auto t0 = simde_mm_shuffle_ps(a, a, _MM_SHUFFLE(0, 0, 2, 1));
		auto t1 = simde_mm_shuffle_ps(b, b, _MM_SHUFFLE(0, 1, 0, 2));

		simde__m128 temp1 = simde_mm_mul_ps(t0, t1);

		auto t2 = simde_mm_shuffle_ps(a, a, _MM_SHUFFLE(0, 1, 0, 2));
		auto t3 = simde_mm_shuffle_ps(b, b, _MM_SHUFFLE(0, 0, 2, 1));

		simde__m128 temp2	= simde_mm_mul_ps(t2, t3);
		simde__m128 res		= simde_mm_sub_ps(temp1, temp2);

		return res;
	}

	union Quaternion;
	Quaternion GrassManProduct(const Quaternion& lhs, const Quaternion& rhs) noexcept;


	/************************************************************************************************/


	struct float2
	{
	public:
		constexpr float2() noexcept : x(0), y(0) {}


		constexpr float2(const float X, const float Y) noexcept
		{
			x = X;
			y = Y;
		}

		constexpr explicit float2(const float in_f) noexcept { x = in_f; y = in_f; }

		constexpr float2(const Vector_t auto& vect) noexcept
		{
			x = (float)vect[0];
			y = (float)vect[1];
		}


		constexpr bool		operator == ( const float2& rhs ) const { return ( rhs.x == x && rhs.y == y ) ? true : false; }

		float&		operator[] (const size_t i) noexcept        
		{ 
			return i ? y : x; 
		}

		constexpr const float operator[] (const size_t i) const noexcept
		{
			return i ? y : x;
		}

		constexpr float2 operator + ( const float2& a ) const noexcept { return float2( x + a.x,	y + a.y );					}
		constexpr float2 operator + ( const float   a ) const noexcept { return float2( x + a, y + a );							}
		constexpr float2 operator - ( const float2& a ) const noexcept { return float2( x - a.x, y - a.y );						}
		constexpr float2 operator - ( const float   a ) const noexcept { return float2( x - a,	y - a );						}
		constexpr float2 operator * ( const float2& a ) const noexcept { return float2( x * a.x,	y * a.y );					}
		constexpr float2 operator * ( const float   a ) const noexcept { return float2( x * a,	y * a );						}
		constexpr float2 operator / ( const float2& a ) const noexcept { return float2( x / a.x,	y / a.y );					}
		constexpr float2 operator / ( const float   a ) const noexcept { return float2( x / a,	y / a );						}
		constexpr float2 operator % ( const float2& a ) const noexcept { return float2( std::fmod(x, a.x), std::fmod(y, a.y));	}

		constexpr float2& operator = (const float2& a) noexcept = default;// { x = a.x; y = a.y; return *this; }

		constexpr float2& operator *= (const float2& v) noexcept
		{ 
			*this = *this * v;
			return *this; 
		}

		constexpr float2& operator *= (const float s) noexcept
		{
			*this = *this * s;
			return *this;
		}

		constexpr float2& operator /= (const float2& v) noexcept
		{
			*this = *this / v;
			return *this;
		}

		constexpr float2& operator /= (const float s) noexcept
		{
			*this = *this / s;
			return *this;
		}

		constexpr float2&	operator -= (const float2& rhs) noexcept
		{
			*this = *this - rhs;
			return *this;
		}

		constexpr float2& operator -= (const float rhs) noexcept
		{
			*this = *this - rhs;
			return *this;
		}


		constexpr float2&	operator += (const float2& rhs) noexcept
		{
			*this = *this + rhs;
			return *this;
		}


		constexpr float2& operator += (const float rhs) noexcept
		{
			*this = *this + rhs;
			return *this;
		}


		constexpr bool operator > (const float2& rhs) const noexcept
		{
			return (x > rhs.x) && (y > rhs.y);
		}


		constexpr bool operator < (const float2& rhs) const noexcept
		{
			auto temp = !(*this > rhs);
			return temp;
		}


		constexpr void Add( const float2& lhs, const float2& rhs ) noexcept
		{
			x = lhs.x + rhs.x;
			y = lhs.y + rhs.y;
		}


		constexpr float2 floor() const noexcept
		{
			return { ::floorf(x), ::floorf(y) };
		}


		constexpr float2 ceil() const noexcept
		{
			return { ::ceilf(x), ::ceilf(y) };
		}


		constexpr operator float* ()			 noexcept { return &x; }
		constexpr operator const float* () const noexcept { return &x; }


		constexpr float Product()	const noexcept { return x * y; }
		constexpr float Sum()	    const noexcept { return x + y; }


		bool isNaN() const noexcept
		{
			return std::isnan(x) || std::isnan(y);
		}


		constexpr float2 abs() const noexcept
		{
			return  float2{ std::abs(x), std::abs(y) };
		}


		constexpr static bool Compare(float2 lhs, float2 rhs, float e) noexcept
		{
			return (lhs - rhs).magnitudeSq() <= (e * e);
		}


		float2& normalize() noexcept
		{
			const float m = magnitude();
			x /= m;
			y /= m;

			return *this;
		}

		float2 normal() const noexcept
		{
			const float		m		= magnitude();
			const float2	self	= *this;

			return self / m;
		}

		float magnitude() const noexcept
		{ 
			const auto V_2 = (*this * *this);
			return  sqrt(V_2.Sum());
		}

		constexpr float magnitudeSq() const noexcept
		{
			return (*this * *this).Sum();
		}


		ALIAS_TEMPLATE_FUNCTION			(Normal,		normal);
		CONST_ALIAS_TEMPLATE_FUNCTION	(Normalize,		normalize);
		CONST_ALIAS_TEMPLATE_FUNCTION	(Magnitude,		magnitude);
		CONST_ALIAS_TEMPLATE_FUNCTION	(MagnitudeSq,	magnitudeSq);


		constexpr static float2 Zero() noexcept
		{
			return { 0, 0 };
		}

		const float* data() const noexcept { return &x; }

		float x, y;

		constexpr static size_t Size() { return 2; }
		constexpr static size_t size() { return Size(); }
	};


	/************************************************************************************************/

	constexpr unsigned floorlog2(unsigned x)
	{
		return x == 1 ? 0 : 1 + floorlog2(x >> 1);
	}

	constexpr unsigned ceillog2(unsigned x)
	{
		return x == 1 ? 0 : floorlog2(x - 1) + 1;
	}
	
	template<bool Condition, typename TY_A, typename TY_B>
	struct ConditionalType {};

	template<typename TY_A, typename TY_B>
	struct ConditionalType<true, TY_A, TY_B>
	{
		using Type = TY_A;
	};

	template<typename TY_A, typename TY_B>
	struct ConditionalType<false, TY_A, TY_B>
	{
		using Type = TY_B;
	};

	template<bool Condition, typename TY_A, typename TY_B>
	using ConditionalType_t = ConditionalType<Condition, TY_A, TY_B>::Type;

	struct Empty {};

	template<typename TY, size_t SIZE, bool PAD>
	struct VectorData
	{
		template<size_t elementSize, size_t blockSize>
		static constexpr size_t GetPadding() noexcept
		{
			if (PAD)
			{
				if (blockSize % elementSize == 0)
					return 0;
				else
					return (blockSize - (blockSize % sizeof(TY))) % sizeof(TY);
			}
			else return 0;
		}
		
		static constexpr size_t GetElementCount() noexcept
		{
			return SIZE + GetPadding<sizeof(TY), 16>();
		}

		VectorData() = default;
		VectorData() requires(GetPadding() == 0) = default;


		using simd256FloatContainer = ConditionalType_t<std::is_same_v<TY, double>, simde__m256d, simde__m256>;
		using simd128container		= ConditionalType_t<std::is_floating_point_v<TY>, simde__m128, simde__m128i>;
		using simd256container		= ConditionalType_t<std::is_floating_point_v<TY>, simd256FloatContainer, simde__m256i>;

		constexpr static bool	simd128Enabled	= ((sizeof(TY) * GetElementCount()) % 16 == 0);
		constexpr static size_t	simd128Count	= ((sizeof(TY) * (GetElementCount())) / sizeof(simd128container));
		
		constexpr static bool	simd256Enabled	= (simd128Count % 2 == 0);
		constexpr static size_t simd256Count	= ((sizeof(TY) * (GetElementCount())) / sizeof(simd256container));

		using simd128 = ConditionalType_t<simd128Enabled, simd128container, Empty>;
		using simd256 = ConditionalType_t<simd256Enabled, simd256container, Empty>;

		union
		{
			TY				vector[GetElementCount()];
			simd128			m128[simd128Enabled ? simd128Count : 1];
			simd256			m256[simd256Enabled ? simd256Count : 1];
		};

		constexpr static size_t GetSIMDWidth() noexcept
        {
		    return sizeof(simd128) / sizeof(TY);
		}

		constexpr static size_t GetSIMDB128locks() noexcept
		{
			return sizeof(simd128) / sizeof(simd128);
		}

		constexpr static size_t GetSIMD256Blocks() noexcept
		{
			return sizeof(simd256) / sizeof(simd128);
		}

		auto GetSIMD(const size_t idx = 0) const
		{
			if constexpr (std::is_same_v<TY, float> && !simd128Enabled)
			{
				auto r = (SIZE - 4 * idx);
				switch (r)
				{
				case 1:
				{
					auto load = simde_mm_load_ps(vector + 4 * idx);
					return simde_mm_blend_ps(load, simde_mm_set1_ps(0.0f), 0b1110);
				}
				case 2:
				{
					auto load = simde_mm_load_ps(vector + 4 * idx);
					return simde_mm_blend_ps(load, simde_mm_set1_ps(0.0f), 0b1100);
				}
				case 3:
				{
					auto load = simde_mm_load_ps(vector + 4 * idx);
					return simde_mm_blend_ps(load, simde_mm_set1_ps(0.0f), 0b1000);
				}
				default:
					return simde_mm_load_ps(vector + 4 * idx);
				}
			}
			else if constexpr (std::is_same_v<TY, double> && !simd128Enabled)
			{
				auto r = (SIZE - 4 * idx);
				switch (r)
				{
				case 1:
				{
					auto load = simde_mm_load_ps(vector + 4 * idx);
					return simde_mm_blend_pd(load, simde_mm_set1_pd(0.0f), 0b1110);
				}
				case 2:
				{
					auto load = simde_mm_load_ps(vector + 4 * idx);
					return simde_mm_blend_pd(load, simde_mm_set1_pd(0.0f), 0b1100);
				}
				case 3:
				{
					auto load = simde_mm_load_ps(vector + 4 * idx);
					return simde_mm_blend_pd(load, simde_mm_set1_pd(0.0f), 0b1000);
				}
				default:
					return simde_mm_load_pd(vector + 4 * idx);
				}
			}
			else if constexpr (std::is_same_v<TY, float> && simd128Enabled)
			{
				if constexpr (PAD)
					return m128[idx];

				auto r = (SIZE - 4 * idx);
				switch (r)
				{
				case 1:
				{
					auto load = simde_mm_load_ps(vector + 4 * idx);
					return simde_mm_blend_ps(load, simde_mm_set1_ps(0.0f), 0b1110);
				}
				case 2:
				{
					auto load = simde_mm_load_ps(vector + 4 * idx);
					return simde_mm_blend_ps(load, simde_mm_set1_ps(0.0f), 0b1100);
				}
				case 3:
				{
					auto load = simde_mm_load_ps(vector + 4 * idx);
					return simde_mm_blend_ps(load, simde_mm_set1_ps(0.0f), 0b1000);
				}
				default:
					return m128[idx];
				}
			}
			else if constexpr (std::is_same_v<TY, double> && simd128Enabled)
			{
				if constexpr (PAD)
					return m128[idx];

				auto r = (SIZE - 4 * idx);
				switch (r)
				{
				case 1:
				{
					auto load = simde_mm_load_pd(vector + 4 * idx);
					return simde_mm_blend_pd(load, simde_mm_set1_pd(0.0f), 0b1110);
				}
				case 2:
				{
					auto load = simde_mm_load_pd(vector + 4 * idx);
					return simde_mm_blend_pd(load, simde_mm_set1_pd(0.0f), 0b1100);
				}
				case 3:
				{
					auto load = simde_mm_load_pd(vector + 4 * idx);
					return simde_mm_blend_pd(load, simde_mm_set1_pd(0.0f), 0b1000);
				}
				default:
					return m128[idx];
				}
			}
			else if constexpr (std::is_integral_v<TY> && !simd128Enabled)
			{
				auto r = (SIZE - 4 * idx);
				switch (r)
				{
				default:
					return simde_mm_loadu_epi32(vector + 4 * idx);
				case 1:
				{
					auto load = simde_mm_loadu_epi32(vector + 4 * idx);
					return simde_mm_blend_epi32(load, simde_mm_set1_epi32(0), 0b1110);
				}	break;
				case 2:
				{
					auto load = simde_mm_loadu_epi32(vector + 4 * idx);
					return simde_mm_blend_epi32(load, simde_mm_set1_epi32(0), 0b1100);
				}
				break;
				case 3:
				{
					auto load = simde_mm_loadu_epi32(vector + 4 * idx);
					return simde_mm_blend_epi32(load, simde_mm_set1_epi32(0), 0b1000);
				}
				}
			}
			else if constexpr (std::is_integral_v<TY> && simd128Enabled)
			{
				if constexpr (PAD)
					return m128[idx];

				auto r = (SIZE - 4 * idx);
				switch (r)
				{
				default:
					return m128[idx];
				case 1:
				{
					auto load = simde_mm_loadu_epi32(vector + 4 * idx);
					return simde_mm_blend_epi32(load, simde_mm_set1_epi32(0), 0b1110);
				}	break;
				case 2:
				{
					auto load = simde_mm_loadu_epi32(vector + 4 * idx);
					return simde_mm_blend_epi32(load, simde_mm_set1_epi32(0), 0b1100);
				}
				break;
				case 3:
				{
					auto load = simde_mm_loadu_epi32(vector + 4 * idx);
					return simde_mm_blend_epi32(load, simde_mm_set1_epi32(0), 0b1000);
				}
				}
			}
		}

		auto GetSIMD256(const size_t idx = 0) const noexcept requires(sizeof(vector) >= 32 && PAD)
		{
			if constexpr (std::is_same_v<TY, float> && !simd256Enabled)
			{
				auto r = (SIZE - 4 * idx);
				switch (r)
				{
				case 1:
				{
					auto load = simde_mm256_load_ps(vector + 8 * idx);
					return simde_mm256_blend_ps(load, simde_mm256_set1_ps(0.0f), 0b1110);
				}
				case 2:
				{
					auto load = simde_mm256_load_ps(vector + 8 * idx);
					return simde_mm256_blend_ps(load, simde_mm256_set1_ps(0.0f), 0b1100);
				}
				case 3:
				{
					auto load = simde_mm256_load_ps(vector + 8 * idx);
					return simde_mm256_blend_ps(load, simde_mm256_set1_ps(0.0f), 0b1000);
				}
				default:
					return simde_mm256_load_ps(vector + 8 * idx);
				}
			}
			else if constexpr (std::is_same_v<TY, double> && !simd256Enabled)
			{
				auto r = (SIZE - 4 * idx);
				switch (r)
				{
				case 1:
				{
					auto load = simde_mm256_load_pd(vector + 4 * idx);
					return simde_mm256_blend_pd(load, simde_mm256_set1_pd(0.0f), 0b1110);
				}
				case 2:
				{
					auto load = simde_mm256_load_ps(vector + 4 * idx);
					return simde_mm256_blend_pd(load, simde_mm256_set1_pd(0.0f), 0b1100);
				}
				case 3:
				{
					auto load = simde_mm256_load_ps(vector + 4 * idx);
					return simde_mm256_blend_pd(load, simde_mm256_set1_pd(0.0f), 0b1000);
				}
				default:
					return simde_mm256_load_pd(vector + 4 * idx);
				}
			}
			else if constexpr (std::is_same_v<TY, float> && simd256Enabled)
			{
				if constexpr (PAD)
					return m128[idx];

				auto getMask = [&](constexpr uint32_t i) {	return 0xff << i; };

				auto r = (SIZE - 4 * idx);
				if (r == 0)
					return m128[idx];

				auto load = simde_mm256_load_ps(vector + 8 * idx);
				switch (r)
				{
				case 1:
					return simde_mm256_blend_ps(load, simde_mm256_set1_ps(0.0f), getMask(1));
				case 2:
					return simde_mm256_blend_ps(load, simde_mm256_set1_ps(0.0f), getMask(2));
				case 3:
					return simde_mm256_blend_ps(load, simde_mm256_set1_ps(0.0f), getMask(3));
				case 4:
					return simde_mm256_blend_ps(load, simde_mm256_set1_ps(0.0f), getMask(4));
				case 5:
					return simde_mm256_blend_ps(load, simde_mm256_set1_ps(0.0f), getMask(5));
				case 6:
					return simde_mm256_blend_ps(load, simde_mm256_set1_ps(0.0f), getMask(6));
				case 7:
					return simde_mm256_blend_ps(load, simde_mm256_set1_ps(0.0f), getMask(7));
				default:
				}
			}
			else if constexpr (std::is_same_v<TY, double> && simd256Enabled)
			{
				if constexpr (PAD)
					return m128[idx];

				auto r = (SIZE - 4 * idx);
				switch (r)
				{
				case 1:
				{
					auto load = simde_mm_load_pd(vector + 4 * idx);
					return simde_mm_blend_pd(load, simde_mm256_set1_pd(0.0f), 0b1110);
				}
				case 2:
				{
					auto load = simde_mm_load_pd(vector + 4 * idx);
					return simde_mm_blend_pd(load, simde_mm256_set1_pd(0.0f), 0b1100);
				}
				case 3:
				{
					auto load = simde_mm_load_pd(vector + 4 * idx);
					return simde_mm_blend_pd(load, simde_mm256_set1_pd(0.0f), 0b1000);
				}
				default:
					return m128[idx];
				}
			}
			else if constexpr (std::is_integral_v<TY> && !simd256Enabled)
			{
				auto r = (SIZE - 4 * idx);
				switch (r)
				{
				default:
					return simde_mm_loadu_epi32(vector + 4 * idx);
				case 1:
				{
					auto load = simde_mm_loadu_epi32(vector + 4 * idx);
					return simde_mm_blend_epi32(load, simde_mm_set1_epi32(0), 0b1110);
				}	break;
				case 2:
				{
					auto load = simde_mm_loadu_epi32(vector + 4 * idx);
					return simde_mm_blend_epi32(load, simde_mm_set1_epi32(0), 0b1100);
				}
				break;
				case 3:
				{
					auto load = simde_mm_loadu_epi32(vector + 4 * idx);
					return simde_mm_blend_epi32(load, simde_mm_set1_epi32(0), 0b1000);
				}
				}
			}
			else if constexpr (std::is_integral_v<TY> && simd256Enabled)
			{
				if constexpr (PAD)
					return m128[idx];

				auto getMask = [&](constexpr uint32_t i) {	return 0xff << i; };

				auto r = (SIZE - 4 * idx);
				if (r == 0)
					return m128[idx];

				auto load = simde_mm256_load_epi32(vector + 8 * idx);
				switch (r)
				{
				case 1:
					return simde_mm256_blend_epi32(load, simde_mm256_set1_epi32(0.0f), getMask(1));
				case 2:
					return simde_mm256_blend_epi32(load, simde_mm256_set1_epi32(0.0f), getMask(2));
				case 3:
					return simde_mm256_blend_epi32(load, simde_mm256_set1_epi32(0.0f), getMask(3));
				case 4:
					return simde_mm256_blend_epi32(load, simde_mm256_set1_epi32(0.0f), getMask(4));
				case 5:
					return simde_mm256_blend_epi32(load, simde_mm256_set1_epi32(0.0f), getMask(5));
				case 6:
					return simde_mm256_blend_epi32(load, simde_mm256_set1_epi32(0.0f), getMask(6));
				case 7:
					return simde_mm256_blend_epi32(load, simde_mm256_set1_epi32(0.0f), getMask(7));
				}
			}
		}

		auto SetSIMD(const simde__m128& v, size_t idx = 0)
		{
			if (PAD)
				memcpy(vector + 4 * idx, &v, sizeof(TY) * 4);
			else
				memcpy(vector + 4 * idx, &v, sizeof(TY) * Max(SIZE - 4 * idx, 4));
		}

		auto SetSIMD256(const simde__m256& v, size_t idx = 0)
		{
			if (PAD)
				memcpy(vector + (sizeof(m256) / sizeof(TY)) * idx, &v, sizeof(m256));
			else
				memcpy(vector + (sizeof(m256) / sizeof(TY)) * idx, &v, sizeof(TY) * Max(SIZE - 4 * idx, 4));
		}

		auto SetSIMD256d(const simde__m256d& v, size_t idx = 0)
		{
			if (PAD)
				memcpy(vector + (sizeof(m256) / sizeof(TY)) * idx, &v, sizeof(m256));
			else
				memcpy(vector + (sizeof(m256) / sizeof(TY)) * idx, &v, sizeof(TY) * Max(SIZE - 4 * idx, 4));
		}

		/*
		auto GetSIMD256(size_t idx = 0) const
		{
			auto load = simde_mm256_load_ps(vector + 8 * idx);

			auto r = (SIZE - 4 * idx);
			if (!PAD && r < 4)
			{
				auto m = Max(SIZE - 4 * idx, 4);
				switch (m)
				{
				case 1:
					load = simde_mm_blend_ps(load, simde_mm256_set1_ps(0.0f), 0b0111);
					break;
				case 2:
					load = simde_mm_blend_ps(load, simde_mm256_set1_ps(0.0f), 0b0011);
					break;
				case 3:
					load = simde_mm_blend_ps(load, simde_mm256_set1_ps(0.0f), 0b0001);
					break;
				default:
					break;
				}
			}

			return load;
		}
		*/

		template<size_t idx>
		static constexpr uint32_t GetSIMD128ShuffleMask() noexcept
		{
			auto GetIdx = []<size_t i>()
			{
				if (idx * 4 + i > SIZE)
					return 0;
				else
					return idx % 4;
			};

			auto m = _MM_SHUFFLE(
				GetIdx.template operator()<0>(),
				GetIdx.template operator()<1>(),
				GetIdx.template operator()<2>(),
				GetIdx.template operator()<3>());

			return 0;
		}

		template<size_t idx>
		static constexpr uint32_t GetSIMD128Mask() noexcept
		{
			auto r = (SIZE - 4 * idx);
			switch (r)
			{
			default:	return 0b0000;
			case 1:		return 0b1110;
			case 2:		return 0b1100;
			case 3:		return 0b1000;
			}
		}
	};

	template<typename TY, bool PAD>
	struct VectorData<TY, 1, PAD>
	{
		union
		{
			TY vector[PAD ? 4 : 1];

			struct {
				TY x;
			};

			struct {
				TY r;
			};
		};


		VectorData() = default;

		constexpr static size_t GetSIMDWidth()
		{
			return sizeof(vector) / sizeof(TY);
		}

		constexpr static size_t GetSIMD128Blocks()
		{
			return 0;
		}

		constexpr static size_t GetSIMD256Blocks()
		{
			return 0;
		}

		auto GetSIMD(size_t idx = 0) const noexcept
		{
			TY temp[16 / (sizeof(TY))];
			memset(&temp, 0, sizeof(vector));
			memcpy(&temp, &vector, sizeof(vector));

			return simde_mm_loadr_ps((float*)&temp);
		}

		auto SetSIMD(const simde__m128& v, size_t idx = 0) noexcept
		{
			if (PAD)
				simde_mm_store_ps(vector, v);
			else
			{
				memcpy(vector, v, sizeof(vector));
			}
		}

		template<size_t i>
		static constexpr uint32_t GetSIMD128ShuffleMask() noexcept
		{
			return 0b1110;
		}

		template<size_t i>
		static constexpr uint32_t GetSIMD128Mask() noexcept
		{
			auto m = _MM_SHUFFLE(0, 0, 1, 0);
			return m;
		}
	};

	template<typename TY, bool PAD>
	struct VectorData<TY, 2, PAD>
	{
		union
		{
			TY vector[PAD ? (16 / (sizeof(TY))): 2];

			struct {
				TY x, y;
			};

			struct {
				TY r, g;
			};
		};

		constexpr static size_t GetSIMDWidth()
		{
			return sizeof(vector) / sizeof(TY);
		}

		constexpr static size_t GetSIMD128Blocks()
		{
			return 0;
		}

		constexpr static size_t GetSIMD256Blocks()
		{
			return 0;
		}


		auto GetSIMD(size_t idx = 0) const noexcept requires(!std::is_same_v<TY, double>)
 		{
			TY temp[16 / (sizeof(TY))];
			memset(&temp, 0, sizeof(temp));
			memcpy(&temp, &vector, sizeof(vector));

			return simde_mm_load_ps((float*)&temp);
		}

		auto GetSIMD(size_t idx = 0) const noexcept requires(std::is_same_v<TY, double>)
		{
			TY temp[16 / (sizeof(TY))];
			memset(&temp, 0, sizeof(temp));
			memcpy(&temp, &vector, sizeof(vector));

			return simde_mm_load_pd((double*)&temp);
		}

		auto SetSIMD(const simde__m128& vin, size_t idx = 0) noexcept
		{
			memcpy(&vector, &vin, sizeof(TY) * 2);
		}

		auto SetSIMD(const simde__m128d& vin, size_t idx = 0) noexcept
		{
			memcpy(&vector, &vin, sizeof(TY) * 2);
		}

		template<size_t i>
		static constexpr uint32_t GetSIMD128ShuffleMask() noexcept
		{
			auto m = _MM_SHUFFLE(0, 0, 1, 0);
			return m;
		}

		template<size_t i>
		static constexpr uint32_t GetSIMD128Mask() noexcept
		{
			return 0b1100;
		}
	};

	template<typename TY, bool PAD>
	struct VectorData<TY, 3, PAD>
	{
		using V_TY		= ConditionalType_t<std::is_same_v<double, TY>, simde__m128d, simde__m128>;

		using V128_TY = ConditionalType_t<PAD, V_TY, Empty>;
		using V256_TY =
			ConditionalType_t<PAD, 
	            ConditionalType_t<std::is_same_v<double, TY>, simde__m256d, simde__m128>, 
	            Empty>;

		using SIMD_TY	= ConditionalType_t<PAD, V_TY, Empty>;

		union
		{
			NO_UNIQUE_ADDRESS TY		vector[PAD ? 4 : 1];
			NO_UNIQUE_ADDRESS V128_TY	vector128[2];
			NO_UNIQUE_ADDRESS V256_TY	vector256[1];

			struct {
				NO_UNIQUE_ADDRESS TY x, y, z;
			};

			struct {
				NO_UNIQUE_ADDRESS TY r, g, b;
			};
		};

		VectorData() = default;

		constexpr static size_t GetSIMDWidth()
		{
			return sizeof(SIMD_TY) / sizeof(TY);
		}

		constexpr static size_t GetSIMD128Blocks()
		{
			return PAD ? 2 : 0;
		}

		constexpr static size_t GetSIMD256Blocks()
		{
			return PAD ? 1 : 0;
		}

		auto GetSIMD(size_t idx = 0) const noexcept
		{
			if constexpr (PAD)
				return vector128[idx];
			else
				return simde_mm_loadr_ps((float*)&vector);
		}

		auto GetSIMD256(size_t idx = 0) const noexcept requires(std::is_same_v<double, TY> && PAD)
		{
			if constexpr (PAD)
				return vector256[idx];
			else
				return simde_mm_loadr_pd((float*)&vector);
		}

		auto SetSIMD(const V_TY& vin, size_t idx = 0) noexcept
		{
			if constexpr(PAD)
				vector128[idx] = vin;
			else
			{
				memcpy(&vector, &vin, sizeof(TY) * 3);
			}
		}

		auto SetSIMD256(const V256_TY& vin, size_t idx = 0) const noexcept requires(std::is_same_v<double, TY>&& PAD)
		{
			if constexpr (PAD)
				vector256[idx] = vin;
			else
			{
				memcpy(&vector, &vin, sizeof(TY) * 3);
			}
		}

		template<size_t idx>
		static constexpr uint32_t GetSIMD128ShuffleMask() noexcept
		{
			auto m = _MM_SHUFFLE(0, 2, 1, 0);
			return m;
		}

		template<size_t i>
		static constexpr uint32_t GetSIMD128Mask() noexcept
		{
			return 0b1000;
		}
	};

	template<typename TY, bool PAD>
	struct VectorData<TY, 4, PAD>
	{
		using V_TY = ConditionalType_t<std::is_same_v<double, TY>, simde__m128d, simde__m128>;
		using V256_TY =
			ConditionalType_t<PAD,
			ConditionalType_t<std::is_same_v<double, TY>, simde__m256d, simde__m128>,
			Empty>;

		using SIMD_TY = ConditionalType_t<PAD, V_TY, Empty>;

		union
		{
			TY			vector[4];
			SIMD_TY		m128[sizeof(vector) / sizeof(simde__m128)];
			V256_TY		m256[1];
			struct {
				TY x, y, z, w;
			};

			struct {
				TY r, g, b, a;
			};
		};

		VectorData() = default;

		constexpr static size_t GetSIMDWidth()
		{
			return sizeof(SIMD_TY) / sizeof(TY);
		}

		constexpr static size_t GetSIMD128Blocks()
		{
			return 2;
		}

		constexpr static size_t GetSIMD256Blocks()
		{
			return 1;
		}

		auto GetSIMD(size_t idx = 0) const noexcept 
		{
			return m128[idx];
		}

		auto GetSIMD256(size_t idx = 0) const noexcept requires(sizeof(V256_TY) > sizeof(Empty))
		{
			return m256[idx];
		}

		auto SetSIMD(const auto& vin, size_t idx = 0) noexcept
		{
			if constexpr(std::is_same_v<decltype(vin), const SIMD_TY&>)
			    m128[idx] = vin;

			if constexpr (std::is_same_v<decltype(vin), const V256_TY&>)
				m256[idx] = vin;
		}

		template<size_t idx>
		static constexpr uint32_t GetSIMD128ShuffleMask() noexcept
		{
			auto m = _MM_SHUFFLE(3, 2, 1, 0);

			return m;
		}

		template<size_t i>
		static constexpr uint32_t GetSIMD128Mask() noexcept
		{
			return 0b0000;
		}
	};


	template<unsigned int SIZE, typename TY = float, bool PAD = false>
	struct Vect : public VectorData<TY, SIZE, PAD>
	{
		using VectorData<TY, SIZE, PAD>::vector;
		using VectorData<TY, SIZE, PAD>::GetSIMD;
		using VectorData<TY, SIZE, PAD>::SetSIMD;
		using VectorData<TY, SIZE, PAD>::template GetSIMD128Mask;
		using VectorData<TY, SIZE, PAD>::template GetSIMD128ShuffleMask;

		using THISTYPE = Vect<SIZE, TY, PAD>;

		template<typename TY_tuple, int ... ints>
		constexpr void helper(const TY_tuple& tuple, const std::integer_sequence<int, ints...>&) noexcept
		{
			auto halp =
				[&]<int i = 0>
			{
				vector[i] = static_cast<TY>(std::get<i>(tuple));
			};

			(halp.template operator() < ints > (), ...);
		}

		template<typename TY_vect, int ... ints>
		constexpr static decltype(auto) ExtractVect(const TY_vect& vect, const std::integer_sequence<int, ints...>&) noexcept
		{
			return std::forward_as_tuple(vect[ints]...);
		}


		template<typename T>
		constexpr static auto BuildTuple(const T& vectorValue) noexcept requires(Vector_t<T>)
		{
			auto extractElements =
				[&]<size_t ... indices>(std::index_sequence<indices...>)
			{
				return std::forward_as_tuple(vectorValue[indices]...);
			};

			return extractElements(std::make_index_sequence<T::size()>());
		}


		template<typename T>
		constexpr static auto BuildTuple(const T& value) noexcept requires(Scaler_t<T>)
		{
			return std::forward_as_tuple(value);
		}


		constexpr static auto BuildTuple(const float2& f2) noexcept
		{
			return std::forward_as_tuple(f2.x, f2.y);
		}


		template<size_t vectorSize>
		constexpr static auto BuildTuple(const Vect<vectorSize, TY>& vect)
		{
			return ExtractVect(vect, std::make_integer_sequence<int, vectorSize>());
		}


		template<typename ... TY_args>
		constexpr static auto BuildTuple(const TY_args& ... args) noexcept
		{
			return std::tuple_cat(BuildTuple(args)...);
		}


	public:
		using Type = TY;

		constexpr Vect() noexcept = default;


		constexpr Vect(Scaler_t auto n) noexcept
		{
			if (!PAD || std::is_constant_evaluated())
			{
				for (size_t i = 0; i < SIZE; i++)
					vector[i] = n;
			}

			if constexpr (PAD && std::is_floating_point_v<decltype(n)>)
			{
				auto helper_internal = []<int ... ints>(const std::index_sequence<ints...>&, const auto& action) noexcept
				{
					(action.template operator() < ints > (), ...);
				};

				auto helper = [&](const auto& action) noexcept
					{
						constexpr size_t end = (SIZE / 4 + (SIZE % 4 == 0 ? 0 : 1));
						helper_internal(std::make_index_sequence<end>(), action);
					};

				auto set1 = [&]<uint32_t i>
				{
					simde__m128 v = simde_mm_set1_ps(n);
					SetSIMD(v, i);
				};

				helper(set1);
			}
		}


		constexpr explicit Vect(const TY* const in) noexcept
		{
			for (size_t I = 0; I < SIZE; ++I)
				vector[I] = in[I];
		}


		template<typename TY_2, bool RHS_Padded, size_t RHS_size>
		constexpr Vect(const Vect<RHS_size, TY_2, RHS_Padded>& in) noexcept
		{
			for (size_t I = 0; I < SIZE; ++I)
				vector[I] = static_cast<TY>(in[I]);

			for (size_t I = RHS_size; I < SIZE; ++I)
				vector[I] = 0;
		}


		template<Scaler_t TY>
		constexpr Vect(const std::initializer_list<TY> in) noexcept
		{
			for (size_t I = 0; I < SIZE; ++I)
				vector[I] = static_cast<Type>(in.begin()[I]);

			for (size_t I = in.size(); I < SIZE; ++I)
				vector[I] = static_cast<Type>(0);
		}


		template<Vector_t TY_V>
		constexpr Vect(const TY_V in) noexcept
		{
			for (size_t I = 0; I < in.size(); ++I)
				vector[I] = static_cast<Type>(in[I]);

			for (size_t I = in.size(); I < SIZE; ++I)
				vector[I] = static_cast<Type>(0);
		}


		template<typename ... TY_ARGS>
		constexpr Vect(TY_ARGS ... args) noexcept requires(sizeof ... (TY_ARGS) > 1)
		{
			static_assert(sizeof ... (args) <= SIZE, "Input value count must be less than container size!");

			const auto			tuple = BuildTuple(args...);
			constexpr size_t	valueCount = std::tuple_size_v<decltype(tuple)>;
			const auto			indexes = std::make_integer_sequence<int, valueCount>{};

			helper(tuple, indexes);
		}


		constexpr TY* begin() noexcept
		{
			return vector;
		}


		constexpr TY* end() noexcept
		{
			return vector + SIZE;
		}


		constexpr const TY* begin() const noexcept
		{
			return vector;
		}


		constexpr const TY* end() const noexcept
		{
			return vector + SIZE;
		}

		template<typename TY_i, bool RHS_PAD>
		[[nodiscard]] constexpr TY Dot(const Vect<SIZE, TY_i, RHS_PAD>& rhs) const noexcept requires (std::is_same_v<float, TY> || (PAD == false) && SIZE > 4 && SIZE < 8)
		{
			simde__m128 sum = [&]
				{
					auto GetRHSVector = [&]
						{
							if constexpr (std::is_same_v<float, TY_i>)
							{
								return rhs.GetSIMD(0);
							}
							else if constexpr (std::is_same_v<double, TY_i>)
							{
								const simde__m128 a = simde_mm_cvtpd_ps(rhs.GetSIMD(0));
								const simde__m128 b = simde_mm_cvtpd_ps(rhs.GetSIMD(1));

								return 
									simde_mm_shuffle_ps(
										a,
										b,
										0b01000100);
							}
						};

					constexpr auto mask = []
						{
							switch (SIZE)
							{
							case 1:
								return 0x1F;
							case 2:
								return 0x3F;
							case 3:
								return 0x7F;
							case 4:
								return 0xFF;
							}
						}();

					simde__m128 l = GetSIMD(0);
					simde__m128 r = GetRHSVector();
					return simde_mm_dp_ps(l, r, mask);
				}();
			
			auto helper_internal = []<int ... ints>(const std::index_sequence<ints...>&, const auto& action) noexcept
			{
				(action.template operator() < ints > (), ...);
			};

			auto helper = [&](const auto& action) noexcept
				{
					constexpr size_t end = (SIZE / 4 + (SIZE % 4 == 0 ? 0 : 1)) - 1;
					helper_internal(std::make_index_sequence<end>(), action);
				};

			auto dp = [&]<uint32_t i>
			{
				simde__m128 l = GetSIMD(1 + i);
				simde__m128 r = rhs.GetSIMD(1 + i);

				switch (SIZE - i * 4)
				{
				case 1:
					sum = simde_mm_add_ps(sum, simde_mm_dp_ps(l, r, 0x1F));
					break;
				case 2:
					sum = simde_mm_add_ps(sum, simde_mm_dp_ps(l, r, 0x3F));
					break;
				case 3:
					sum = simde_mm_add_ps(sum, simde_mm_dp_ps(l, r, 0x7F));
					break;
				default:
					sum = simde_mm_add_ps(sum, simde_mm_dp_ps(l, r, 0xFF));
				}
			};

			if (SIZE > 4)
				helper(dp);

			return simde_mm_cvtss_f32(sum);
		}


		template<typename RHS_TY, bool RHS_PAD, size_t RHS_SIZE>
		[[nodiscard]] constexpr TY Dot(const Vect<RHS_SIZE, RHS_TY, RHS_PAD>& rhs_v) const noexcept requires (std::is_same_v<float, TY> && SIZE <= 4)
		{
			simde__m128 lhs = GetSIMD();
			simde__m128 rhs = rhs_v.GetSIMD();

			constexpr auto mask = []
				{
					switch (SIZE)
					{
					case 1:
						return 0x1F;
					case 2:
						return 0x3F;
					case 3:
						return 0x7F;
					case 4:
						return 0xFF;
					}
				}();

			return simde_mm_cvtss_f32(simde_mm_dp_ps(lhs, rhs, mask));
		}


		template<typename RHS_TY, bool RHS_PAD, size_t RHS_SIZE>
		[[nodiscard]] constexpr TY Dot(const Vect<RHS_SIZE, RHS_TY, RHS_PAD>& rhs_v) const noexcept requires (std::is_same_v<double, RHS_TY> && std::is_same_v<double, TY>&& SIZE <= 4)
		{
			auto product = *this* rhs_v;
			auto s = product.Sum();

			return s;
		}


		template<typename RHS_TY, bool RHS_PAD, size_t RHS_SIZE>
		[[nodiscard]] constexpr TY Dot(const Vect<RHS_SIZE, RHS_TY, RHS_PAD>& rhs_v) const noexcept requires (std::is_same_v<double, TY> && std::is_same_v<float, RHS_TY> && SIZE <= 4)
		{
			simde__m256d a = this->GetSIMD256();
			simde__m256d b = simde_mm256_cvtps_pd(rhs_v.GetSIMD());
			simde__m256d c = simde_mm256_mul_pd(a, b);

			// TODO: go through the effort of shuffling with a zero vector instead of this
			switch (SIZE)
			{
			case 1:
				//simde_mm256_blend_pd
                c = simde_mm256_mul_pd(c, simde_mm256_set_pd(0, 0, 0, 1));
				break;
			case 2:
				c = simde_mm256_mul_pd(c, simde_mm256_set_pd(0, 0, 1, 1));
				break;
			case 3:
				c = simde_mm256_mul_pd(c, simde_mm256_set_pd(0, 1, 1, 1));
				break;
			}

			simde__m256d d			= simde_mm256_hadd_pd(c, c);
		    simde__m256d shuffled	= simde_mm256_permute4x64_pd(d, 0b00001000);
			simde__m256d dp			= simde_mm256_hadd_pd(shuffled, shuffled);
			
		    return simde_mm256_cvtsd_f64(dp);



		}


		template<typename RHS_TY, bool RHS_PAD, size_t RHS_SIZE>
		[[nodiscard]] constexpr TY Dot(const Vect<RHS_SIZE, RHS_TY, RHS_PAD>& rhs_v) const noexcept requires (std::is_integral_v<RHS_TY> && std::is_integral_v<TY>)
		{
			TY dp = 0;
			for (auto [x, y] : zip(vector, rhs_v.vector))
				dp += x * y;

			return dp;
		}


		CONST_ALIAS_TEMPLATE_FUNCTION(dot, Dot);


		[[nodiscard]] constexpr THISTYPE Cross(const THISTYPE& rhs) const noexcept requires (SIZE == 3)
		{
			if consteval
			{
				Vect<SIZE, TY, PAD> out;
				for (size_t i = 0; i < SIZE; ++i)
					out[i] = (vector[(1 + i) % SIZE] * rhs[(2 + i) % SIZE]) - (rhs[(1 + i) % SIZE] * vector[(2 + i) % SIZE]);
				return out;
			}
			else
			{
				THISTYPE out;
				out.SetSIMD(CrossProduct(GetSIMD(), rhs.GetSIMD()));
				return out;
			}
		}


		decltype(auto) cross(const auto& rhs) const noexcept
		{
			return Cross(rhs);
		}


		THISTYPE distance(const THISTYPE& b) const noexcept
		{
			return (*this - b).magnitude();
		}


		[[nodiscard]] constexpr TY Norm(unsigned int exp = 2) noexcept
		{
			TY sum = 0;
			for (auto element : vector)
			{
				TY product = 0;
				for (size_t i = 0; i < SIZE; i++)
					product *= element;
				sum += product;
			}

			return pow(sum, 1 / SIZE);
		}


		[[nodiscard]] constexpr TY Magnitude() const noexcept
		{
			if consteval
			{
				TY sum = 0;
				for (auto element : vector)
					sum += element * element;

				return sqrt(sum);
			}
			else
			{
				if constexpr ((SIZE % 4 == 0) && std::is_same_v<float, TY>)
				{
					auto v		= GetSIMD();
					auto sq		= simde_mm_mul_ps(v, v);
					auto a		= simde_mm_hadd_ps(sq, sq);
					auto hsum	= simde_mm_hadd_ps(a, a);
					auto rsqrt	= simde_mm_rsqrt_ps(hsum);
					auto m		= simde_mm_mul_ps(hsum, rsqrt);
					auto s		= simde_mm_cvtss_f32(m);
					return s;
				}
				else
				{
					TY sum = 0;
					for (auto element : vector)
						sum += element * element;

					return sqrt(sum);
				}
			}
		}


		[[nodiscard]] constexpr TY MagnitudeSq() const noexcept
		{
			return ((*this) * (*this)).Sum();
		}


		CONST_ALIAS_TEMPLATE_FUNCTION(magnitude,	Magnitude);
		CONST_ALIAS_TEMPLATE_FUNCTION(magnitudeSq,	MagnitudeSq);


		constexpr		TY& at(const size_t index) noexcept
		{
			return vector[index];
		}

		constexpr		TY at(const size_t index) const noexcept
		{
			return vector[index];
		}


		constexpr		TY& operator [](const size_t index) noexcept
		{
			return vector[index];
		}


		constexpr const	TY& operator [](const size_t index) const noexcept
		{
			return vector[index];
		}


		constexpr		Vect operator & (const TY index) noexcept
		{
			Vect out;

			for (size_t I = 0; I < 3; I++)
				out[I] = at(I) & index;

			return out;
		}


		[[nodiscard]] constexpr TY Sum() const noexcept
		{
			if (PAD)
			{
				if constexpr (std::is_same_v<TY, float>)
				{
				    auto vsum = GetSIMD(0);
				    const size_t end = SIZE / 4 + ((SIZE % 4 == 0) ? 0 : 1);
				    for (auto i = 1; i < end; i++)
					    vsum = simde_mm_add_ps(vsum, GetSIMD(i));

				    auto a = simde_mm_hadd_ps(vsum, vsum);
				    auto b = simde_mm_hadd_ps(a, a);

				    return simde_mm_cvtss_f32(b);
				}
				if constexpr (std::is_same_v<TY, double>)
				{
					if constexpr (this->GetSIMD256Blocks())
					{
						auto vsum = this->GetSIMD256(0);
						const size_t end = this->GetSIMD256Blocks();
						for (auto i = 1; i < end; i++)
							vsum = simde_mm256_add_pd(vsum, this->GetSIMD256(i));

						auto a = simde_mm256_hadd_pd(vsum, vsum);
						auto b = simde_mm256_permute4x64_pd(a, 0b1000);
						auto c = simde_mm256_hadd_pd(b, b);

						return simde_mm256_cvtsd_f64(c);
					}

					if constexpr (this->GetSIMD128Blocks())
					{
						auto vsum = GetSIMD(0);
						const size_t end = this->GetSIMD128Blocks();
						for (auto i = 1; i < end; i++)
							vsum = simde_mm_add_pd(vsum, GetSIMD(i));

						auto a = simde_mm_hadd_pd(vsum, vsum);

						return simde_mm_cvtsd_f64(vsum);
					}
				}
			}
			else
			{
				TY sum = 0;
				for (auto element : vector)
					sum += element;
				return sum;
			}
		}


		template<int ... ints>
		constexpr auto _ceil_helper(const std::integer_sequence<int, ints...> x) const noexcept
		{
			return Vect{ ::ceil(vector[ints])... };
		}


		[[nodiscard]] constexpr auto Ceil() const noexcept requires(std::is_floating_point_v<TY>)
		{
			const auto indexes = std::make_integer_sequence<int, SIZE>{};

			return _ceil_helper(indexes);
		}


		template<int ... ints>
		constexpr auto _floor_helper(const std::integer_sequence<int, ints...> x) const noexcept
		{
			return Vect{ ::floor(vector[ints])... };
		}


		[[nodiscard]] constexpr auto Floor() const noexcept requires(std::is_floating_point_v<TY>)
		{
			const auto indexes = std::make_integer_sequence<int, SIZE>{};

			return _floor_helper(indexes);
		}


		bool isNaN() const noexcept
		{
			bool nan = false;
			for (const auto f : *this)
				nan |= std::isnan(f);

			return nan;
		}


		[[nodiscard]] constexpr auto Min() const noexcept
		{
			TY min = std::numeric_limits<TY>::max();

			for (const auto& v : vector)
				min = min < v ? min : v;

			return min;
		}


		[[nodiscard]] constexpr auto Max() const noexcept
		{
			TY max = std::numeric_limits<TY>::min();

			for (const auto& v : vector)
				max = (max < v) ? max : v;

			return max;
		}


		[[nodiscard]] constexpr TY Product() const noexcept
		{
			if (PAD)
			{
				auto vproduct = GetSIMD(0);
				const size_t end = SIZE / 4;
				for (auto i = 1; i < end; i++)
					vproduct = simde_mm_mul_ps(vproduct, GetSIMD(i));

				if (SIZE % 4 != 0)
				{
					constexpr auto m = []()
						{
							switch (SIZE % 4)
							{
							case 1:
								return 0x0E;
							case 2:
								return 0x0C;
							case 3:
								return 0x08;
							default:
								return 0x00;
							}
						}();

					auto t = GetSIMD(end);
					auto y = simde_mm_blend_ps(t, simde_mm_set_ps1(1), m);
					vproduct = simde_mm_mul_ps(vproduct, y);
				}

				auto a = simde_mm_shuffle_ps(vproduct, vproduct, _MM_SHUFFLE(2, 3, 0, 1));
				auto b = simde_mm_mul_ps(a, vproduct);
				auto c = simde_mm_shuffle_ps(b, b, _MM_SHUFFLE(0, 0, 0, 2));
				auto d = simde_mm_mul_ps(b, c);
				return simde_mm_cvtss_f32(d);
			}
			else
			{
				TY Product = 1;
				for (auto element : vector)
					Product *= element;
				return Product;
			}
		}


		template<Scaler_t TY_2, bool Packed>
		constexpr THISTYPE operator + (const Vect<SIZE, TY_2, Packed>& rhs) const noexcept
		{
			if (PAD || (SIZE % 4 == 0))
			{
				THISTYPE temp = *this;

				const size_t end = SIZE / 4 + ((SIZE % 4 == 0) ? 0 : 1);
				for (auto I = 0; I < end; ++I)
				{
					if constexpr(std::is_same_v<TY, TY_2> && std::is_same_v<TY, float>)
					{
						auto a = temp.GetSIMD(I);
						auto b = rhs.GetSIMD(I);
					    auto c = simde_mm_add_ps(a, b);
					    temp.SetSIMD(c, I);
					}

					if constexpr (std::is_same_v<TY, TY_2> && std::is_same_v<TY, double>)
					{
						auto a = temp.GetSIMD256(I);
						auto b = rhs.GetSIMD256(I);
						auto c = simde_mm256_add_pd(a, b);
						temp.SetSIMD(c, I);
					}

					if constexpr (std::is_same_v<TY, float> && std::is_same_v<TY_2, double>)
					{
						auto a = temp.GetSIMD(I);
						auto b = simde_mm256_cvtpd_ps(rhs.GetSIMD256(I));
						auto c = simde_mm_add_ps(a, b);
						temp.SetSIMD(c, I);
					}

					if constexpr (std::is_same_v<TY, double> && std::is_same_v<TY_2, float>)
					{
						auto a = temp.GetSIMD256(I);
						auto b = simde_mm256_cvtps_pd(rhs.GetSIMD256(I));
						auto c = simde_mm256_add_pd(a, b);
						temp.SetSIMD(c, I);
					}
				}
				return temp;
			}
			else
			{
				THISTYPE temp = *this;

				for (size_t i = 0; i < SIZE; i++)
					temp.vector[i] += rhs[i];

				return temp;
			}
		}


		constexpr THISTYPE operator + (const Scaler_t auto rhs) const noexcept
		{
			THISTYPE temp = *this;
			for (auto I = 0; I < SIZE; ++I)
				temp[I] += rhs;

			return temp;
		}


		THISTYPE& operator += (const Vector_t auto& rhs) noexcept requires((rhs.size() == SIZE))
		{
			if ((PAD || (SIZE % 4 == 0)) && VectorSIMD_t<decltype(rhs)>)
			{
				const size_t end = SIZE / 4 + ((SIZE % 4 == 0) ? 0 : 1);
				for (auto I = 0; I < end; ++I)
				{
					auto a = GetSIMD(I);
					auto b = rhs.GetSIMD(I);

					if constexpr(std::is_same_v<float, TY>)
					{
					    auto c = simde_mm_add_ps(a, b);
					    SetSIMD(c, I);
					}
					else if constexpr (std::is_same_v<double, TY>)
					{
						auto c = simde_mm_add_pd(a, b);
						SetSIMD(c, I);
					}
				}
				return *this;
			}
			else
			{
				for (size_t i = 0; i < SIZE; i++)
					vector[i] += rhs[i];

				return *this;
			}
		}


		THISTYPE& operator -= (const Vector_t auto& rhs) noexcept requires(rhs.size() == SIZE)
		{
			if ((PAD || (SIZE % 4 == 0)) && VectorSIMD_t<decltype(rhs)>)
			{
				const size_t end = SIZE / 4 + ((SIZE % 4 == 0) ? 0 : 1);
				for (auto I = 0; I < end; ++I)
				{
					auto a = GetSIMD(I);
					auto b = rhs.GetSIMD(I);
					auto c = simde_mm_sub_ps(a, b);

					SetSIMD(c, I);
				}
				return *this;
			}
			else
			{
				for (size_t i = 0; i < SIZE; i++)
					vector[i] -= rhs[i];

				return *this;
			}
		}


		THISTYPE& operator += (const Scaler_t auto rhs) noexcept
		{
			if ((PAD || (SIZE % 4 == 0)))
			{
				const size_t end = SIZE / 4 + ((SIZE % 4 == 0) ? 0 : 1);
				for (auto I = 0; I < end; ++I)
				{
					THISTYPE v_RHS{ rhs };
					auto a = GetSIMD(I);
					auto b = v_RHS.GetSIMD(I);
					auto c = simde_mm_add_ps(a, b);

					SetSIMD(c, I);
				}
				return *this;
			}
			else
			{
				for (size_t i = 0; i < SIZE; i++)
					vector[i] += rhs;

				return *this;
			}
		}


		THISTYPE& operator -= (const Scaler_t auto rhs) noexcept
		{
			if ((PAD || (SIZE % 4 == 0)))
			{
				THISTYPE v_RHS{ rhs };
				const size_t end = SIZE / 4 + ((SIZE % 4 == 0) ? 0 : 1);
				for (auto I = 0; I < end; ++I)
				{
					auto a = GetSIMD(I);
					auto b = v_RHS.GetSIMD(I);
					auto c = simde_mm_sub_ps(a, b);

					SetSIMD(c, I);
				}
				return *this;
			}
			else
			{
				for (size_t i = 0; i < SIZE; i++)
					vector[i] -= rhs;

				return *this;
			}
		}


		THISTYPE& MAdd (const VectorSIMD_t auto& V_a, const VectorSIMD_t auto& V_b) noexcept
		{
			if (PAD || (SIZE % 4 == 0))
			{
				const size_t end = SIZE / 4 + ((SIZE % 4 == 0) ? 0 : 1);
				for (auto I = 0; I < end; ++I)
				{
					auto v = GetSIMD(I);
					auto a = V_a.GetSIMD(I);
					auto b = V_b.GetSIMD(I);
					auto c = simde_mm_fmadd_ps(a, b, v);

					SetSIMD(c, I);
				}
				return *this;
			}
			else
			{
				for (size_t i = 0; i < SIZE; i++)
					vector[i] += V_a[i] * V_b[i];

				return *this;
			}
		}


		THISTYPE& MAdd(const VectorSIMDDouble_t auto& V_a, const VectorSIMDDouble_t auto& V_b) noexcept
		{
			if (PAD || (SIZE % 4 == 0))
			{
				const size_t end = SIZE / 4 + ((SIZE % 4 == 0) ? 0 : 1);
				for (auto I = 0; I < end; ++I)
				{
					simde__m256d v = THISTYPE::GetSIMD256(I);
					simde__m256d a = V_a.GetSIMD256(I);
					simde__m256d b = V_b.GetSIMD256(I);
					auto c = simde_mm256_fmadd_pd(a, b, v);

					SetSIMD(c, I);
				}
				return *this;
			}
			else
			{
				for (size_t i = 0; i < SIZE; i++)
					vector[i] += V_a[i] * V_b[i];

				return *this;
			}
		}
		

		constexpr THISTYPE& MAdd(const Vector_t auto& V_a, const Vector_t auto& V_b) noexcept
		{
			for (size_t i = 0; i < SIZE; i++)
				vector[i] += V_a[i] * V_b[i];

			return *this;
		}

		constexpr THISTYPE& MAdd(const VectorSIMD_t auto& V_a, const VectorSIMD_t auto& V_b) noexcept requires (VectorSIMD_t<THISTYPE>)
		{
			SetSIMD(simde_mm_fmadd_ps(V_a.GetSIMD(), V_b.GetSIMD(), this->GetSIMD()), 0);

			return *this;
		}

		constexpr THISTYPE& MAdd(const VectorSIMD_t auto& V_a, const VectorSIMDDouble_t auto& V_b) noexcept requires (VectorSIMDDouble_t<THISTYPE>)
		{
			SetSIMD(simde_mm256_fmadd_pd(simde_mm256_cvtps_pd(V_a.GetSIMD()), V_b.GetSIMD256(), this->GetSIMD256()), 0);

			return *this;
		}


		constexpr THISTYPE operator - () const noexcept
		{
			THISTYPE temp = *this * THISTYPE(-1);

			return temp;
		}


		constexpr THISTYPE operator - (const Vector_t auto& in) const noexcept
		{
			THISTYPE temp = *this;
			for (auto I = 0; I < SIZE; ++I)
				temp[I] -= in[I];

			return temp;
		}


		constexpr THISTYPE operator - (const Scaler_t auto& rhs) const noexcept
		{
			THISTYPE temp = *this;
			for (auto I = 0; I < SIZE; ++I)
				temp[I] -= rhs;

			return temp;
		}


		template<typename TY_2>
		constexpr THISTYPE& operator -= (const Vect<SIZE, TY_2>& in) noexcept
		{
			for (auto I = 0; I < SIZE; ++I)
				vector[I] -= in[I];

			return *this;
		}


		template<typename TY_2, bool RHS_PAD>
		constexpr THISTYPE& operator = (const Vect<SIZE, TY_2, RHS_PAD>& in) noexcept
		{
			for (auto I = 0; I < SIZE; ++I)
				vector[I] = in[I];

			return *this;
		}


		constexpr THISTYPE operator / (Vector_t auto& in) const noexcept
		{
			THISTYPE temp = *this;
			for (auto I = 0; I < SIZE; ++I)
				temp[I] /= in[I];

			return temp;
		}


		constexpr THISTYPE operator / (Scaler_t auto in) const noexcept
		{
			THISTYPE temp = *this;
			for (auto I = 0; I < SIZE; ++I)
				temp[I] /= in;

			return temp;
		}


		constexpr THISTYPE operator * (const Scaler_t auto rhs) const noexcept
		{
			THISTYPE temp = *this;

			return temp * static_cast<THISTYPE>(rhs);
		}


		friend constexpr THISTYPE operator * (const Scaler_t auto lhs, const THISTYPE& rhs) noexcept
		{
			return rhs * lhs;
		}

		template<Scaler_t TY_2, bool Packed>
		constexpr THISTYPE operator * (const Vect<SIZE, TY_2, Packed>& rhs) const noexcept
		{
			if (PAD || (SIZE % 4 == 0))
			{
				THISTYPE result = *this;

				const size_t end = SIZE / 4 + ((SIZE % 4 == 0) ? 0 : 1);
				for (auto I = 0; I < end; ++I)
				{
					if constexpr (std::is_same_v<TY, TY_2> && std::is_same_v<TY, float>)
					{
						auto a = result.GetSIMD(I);
						auto b = rhs.GetSIMD(I);
						auto c = simde_mm_mul_ps(a, b);
						result.SetSIMD(c, I);
					}

					if constexpr (std::is_same_v<TY, TY_2> && std::is_same_v<TY, double>)
					{
						auto a = result.GetSIMD256(I);
						auto b = rhs.GetSIMD256(I);
						auto c = simde_mm256_mul_pd(a, b);
						result.SetSIMD(c, I);
					}

					if constexpr (std::is_same_v<TY, float> && std::is_same_v<TY_2, double>)
					{
						auto a = result.GetSIMD(I);
						auto b = simde_mm256_cvtpd_ps(rhs.GetSIMD256(I));
						auto c = simde_mm_mul_ps(a, b);
						result.SetSIMD(c, I);
					}

					if constexpr (std::is_same_v<TY, double> && std::is_same_v<TY_2, float>)
					{
						auto a = result.GetSIMD256(I);
						auto b = simde_mm256_cvtps_pd(rhs.GetSIMD256(I));
						auto c = simde_mm256_mul_pd(a, b);
						result.SetSIMD(c, I);
					}
				}
				return result;
			}
			else
			{
				THISTYPE result = *this;

				for (size_t i = 0; i < SIZE; i++)
					result[i] *= rhs[i];

				return result;
			}
		}

		template<typename TY_2>
		constexpr THISTYPE operator % (TY_2 in) const noexcept
		{
			THISTYPE temp = *this;
			for (auto I = 0; I < SIZE; ++I)
				temp[I] %= in;

			return temp;
		}


		template<Scaler_t TY_2>
		constexpr THISTYPE& operator /= (const TY_2 in) noexcept
		{
			for (auto I = 0; I < SIZE; ++I)
				vector[I] /= in;

			return *this;
		}

		template<Scaler_t TY_2>
		constexpr THISTYPE& operator *= (const TY_2 in) noexcept
		{
			for (auto I = 0; I < SIZE; ++I)
				this->vector[I] *= in;

			return *this;
		}


		template<typename TY_2>
		constexpr THISTYPE& operator *= (const Vect<SIZE, TY_2, PAD>& rhs) noexcept
		{
			*this = *this * rhs;
			return *this;
		}


		template<typename TY_2>
		constexpr THISTYPE operator /= (const Vect<SIZE, TY_2, PAD> in) noexcept
		{
			THISTYPE Out;
			for (auto I = 0; I < SIZE; ++I)
				Out[I] = vector[I] / in[I];

			return Out;
		}


		template<typename TY_2>
		constexpr bool operator == (const Vect<SIZE, TY_2>& in) const noexcept
		{
			bool res = true;
			for (auto I = 0; I < SIZE; ++I)
				res = res & (vector[I] == in[I]);

			return res;
		}


		template<typename TY_2>
		constexpr THISTYPE& operator = (const std::initializer_list<TY_2>& il) noexcept
		{
			size_t itr = 0;
			for (auto n : il)
			{
				vector[itr++] = static_cast<TY>(n);
				if (itr > SIZE)
					break;
			}
			return *this;
		}


		template<Vector_t TY_2>
		constexpr THISTYPE& operator = (const TY_2& rhs) noexcept requires(TY_2::size() >= SIZE)
		{
			size_t itr = 0;
			for (auto n : rhs)
			{
				vector[itr++] = n;
				if (itr > SIZE)
					break;
			}
			return *this;
		}

		template<Scaler_t TY_2>
		constexpr THISTYPE& operator = (const TY_2& rhs) noexcept
		{
			size_t itr = 0;
			for (auto& n : vector)
				n = static_cast<TY>(rhs);
			return *this;
		}

		constexpr THISTYPE operator << (const Scaler_t auto sl) const noexcept
		{
			THISTYPE v = *this;

			for (auto& n : v)
				n = v << sl;

			return v;
		}


		constexpr THISTYPE operator >> (const Scaler_t auto sr) const noexcept
		{
			THISTYPE v = *this;

			for (auto& n : v)
				n = n >> sr;

			return v;
		}


		constexpr THISTYPE operator << (const THISTYPE sl) const noexcept
		{
			THISTYPE v = *this;

			size_t i = 0;
			for (auto& n : v)
				v[i] = v[i] << sl[i];

			return v;
		}


		constexpr THISTYPE operator >> (const THISTYPE sr) const noexcept requires((SIZE != 4) && !std::is_same_v<float, TY>)
		{
			THISTYPE v = *this;

			size_t i = 0;
			for (auto& n : v)
				v[i] = v[i] >> sr[i];

			return v;
		}


		constexpr THISTYPE operator >> (const THISTYPE sr) const noexcept requires((SIZE == 4) && std::is_same_v<float, TY>)
		{
			THISTYPE v = *this;

			size_t i = 0;
			for (auto& n : v)
				v[i] = v[i] >> sr[i];

			return v;
		}


		constexpr THISTYPE Normal() const noexcept
		{
			if consteval
			{
				THISTYPE out = *this;

				TY m = (TY)0.0;
				for (auto s : out)
					m += s * s;

				m = sqrt(m);

				for (auto& s : out)
					s /= m;

				return out;
			}
			else
			{
				if constexpr ((SIZE == 4) && std::is_same_v<float, TY>)
				{
					auto v		= GetSIMD();
					auto sq		= simde_mm_mul_ps(v, v);
					auto a		= simde_mm_hadd_ps(sq, sq);
					auto vsum	= simde_mm_hadd_ps(a, a);
					auto rsqr	= simde_mm_rsqrt_ps(vsum);
					auto res	= simde_mm_mul_ps(v, simde_mm_shuffle_ps(rsqr, rsqr, _MM_SHUFFLE(0, 0, 0, 0)));

					return res;
				}
				else
				{
					THISTYPE out = *this;
					TY m = (TY)0.0;
					for (auto s : out)
						m += s * s;

					m = sqrt(m);

					for (auto& s : out)
						s /= m;

					return out;
				}
			}
		}


		constexpr THISTYPE& Normalize() noexcept
		{
			if consteval
			{
				TY m = (TY)0.0;
				for (auto s : *this)
					m += s * s;

				m = sqrt(m);

				for (auto& s : *this)
					s /= m;
			}
			else
			{
				if constexpr ((SIZE == 4) && std::is_same_v<float, TY>)
				{
					auto v		= GetSIMD();
					auto sq		= simde_mm_mul_ps(v, v);
					auto a		= simde_mm_hadd_ps(sq, sq);
					auto vsum	= simde_mm_hadd_ps(a, a);
					auto rsqr	= simde_mm_rsqrt_ps(vsum);
					auto res	= simde_mm_mul_ps(v, simde_mm_shuffle_ps(rsqr, rsqr, _MM_SHUFFLE(0, 0, 0, 0)));

					SetSIMD(res);
				}
				else
				{
					TY m = (TY)0.0;
					for (auto s : *this)
						m += s * s;

					m = sqrt(m);

					for (auto& s : *this)
						s /= m;
				}
			}

			return *this;
		}


		CONST_ALIAS_TEMPLATE_FUNCTION(normal, Normal);
		ALIAS_TEMPLATE_FUNCTION(normalize, Normalize);

		Vect<2, TY, false> xy() const noexcept requires(SIZE >= 2) { return { at(0), at(1) }; }
		Vect<2, TY, false> yz() const noexcept requires(SIZE >= 3) { return { at(1), at(2) }; }
		Vect<2, TY, false> yx() const noexcept requires(SIZE >= 2) { return { at(1), at(0) }; }
		Vect<2, TY, false> xz() const noexcept requires(SIZE >= 3) { return { at(0), at(2) }; }
		Vect<2, TY, false> zy() const noexcept requires(SIZE >= 2) { return { at(2), at(1) }; }
		Vect<2, TY, false> zx() const noexcept requires(SIZE >= 2) { return { at(2), at(0) }; }

		Vect<3, TY, true> zxy() const noexcept requires(SIZE >= 3) { return { at(2), at(0), at(1) }; }
		Vect<3, TY, true> yzx() const noexcept requires(SIZE >= 3) { return { at(1), at(2), at(0) }; }
		Vect<3, TY, true> zyx() const noexcept requires(SIZE >= 3) { return { at(2), at(1), at(0) }; }


		template<size_t ... TY_ARGS>
		constexpr auto Swizzle()
		{
			static_assert(sizeof ... (TY_ARGS) == size(), "Incorrect number of arguments");
			return THISTYPE{ at(TY_ARGS) ... };
		}


		constexpr static THISTYPE Zero() noexcept
		{
			THISTYPE zero{ 0 };

			return zero;
		}

		constexpr static THISTYPE One() noexcept
		{
			THISTYPE zero;

			for (auto& element : zero.vector)
				element = static_cast<TY>(1);

			return zero;
		}

		constexpr static THISTYPE BroadcastScaler(TY s) noexcept
		{
			THISTYPE zero;

			for (auto& element : zero.vector)
				element = s;

			return zero;
		}

		template<int ... ints>
		static constexpr void simdhelper_internal(const std::index_sequence<ints...>&, const auto& action) noexcept
		{
			(action.template operator()<ints>(), ...);
		}

		static constexpr void simdhelper(const auto& action) noexcept
		{
			constexpr size_t end = SIZE / 4 + (PAD && (SIZE % 4 != 0) ? 1 : 0);
			simdhelper_internal(std::make_index_sequence<end>(), action);
		}


		template<int ... ints>
		static constexpr void bc_helper_internal(const std::index_sequence<ints...>&, const auto& action) noexcept
		{
			(action.template operator() < ints > (), ...);
		}

		template<size_t OutSize, size_t Width>
		static constexpr void bchelper(const auto& action) noexcept
		{
			constexpr size_t end = OutSize / Width + (PAD && (OutSize % Width != 0) ? 1 : 0);
			bc_helper_internal(std::make_index_sequence<end>(), action);
		}

		template<uint32_t idx, uint32_t OutSize>
		auto BroadcastElement() const noexcept requires(std::is_floating_point_v<TY>)
		{
			if (PAD || SIZE % this->GetSIMDWidth() == 0)
			{
				auto shuffle = [&](auto& a)
				    {
						if constexpr (std::is_same_v < simde__m128, std::decay_t<decltype(a)>>)
						{
							constexpr auto m = SIMDE_MM_SHUFFLE(idx % 4, idx % 4, idx % 4, idx % 4);
							a = simde_mm_shuffle_ps(a, a, m);
						}
						else if constexpr (std::is_same_v<simde__m128d, std::decay_t<decltype(a)>>)
						{
							constexpr uint32_t m = (idx & 0x1) | ((idx & 0x1) << 1);
							a = simde_mm_shuffle_pd(a, a, m);
						}
						else if constexpr (std::is_same_v<simde__m256d, std::decay_t<decltype(a)>>)
						{
							constexpr auto m = SIMDE_MM_SHUFFLE(idx % 4, idx % 4, idx % 4, idx % 4);
							a = simde_mm256_shuffle_pd(a, a, m);
						}
					};

				Vect<OutSize, TY, PAD> result;
				auto a	= GetSIMD(idx / this->GetSIMDWidth());
				shuffle(a);

				auto helper = 
					[&]<int i>() 
					{
						 result.SetSIMD(a, i);
					};

				bchelper<OutSize, this->GetSIMDWidth()>(helper);

				return result;
			}
			else
			{
				auto x = at(idx);
				Vect<OutSize, TY, PAD> out;

				for (auto& e : out)
					e = x;

				return out;
			}
		}

		template<uint32_t idx, uint32_t OutSize>
		auto BroadcastElement256() const noexcept requires(std::is_floating_point_v<TY> && VectorData::simd256Enabled)
		{
			if (PAD || SIZE % 8 == 0)
			{
				auto shuffle = [&](const auto& a)
					{
						constexpr auto m = ((idx & 3) << 0) | ((idx & 3) << 2) | ((idx & 3) << 4) | ((idx & 3) << 6);

						if constexpr (std::is_same_v<float, TY>)
						{
							return simde_mm256_permute_x8x32_ps(a, m);
						}
						else if constexpr (std::is_same_v<double, TY>)
						{
							return simde_mm256_permute4x64_pd(a, m);
						}
					};

				Vect<OutSize, TY, PAD> result;
				const auto a	= this->GetSIMD256(idx / 4);
				auto shuffled	= shuffle(a);

				auto helper =
					[&]<int i>()
				{
					result.SetSIMD(shuffled, i);
				};

				bchelper<OutSize>(helper);

				return result;
			}
			else
			{
				return Vect<OutSize, TY, PAD>{ (*this)[idx] };
			}
		}

		template<uint32_t idx>
		THISTYPE BroadcastElement() const noexcept
		{
			THISTYPE out;

			for (auto& v : out)
				v = (*this)[idx];
			
			return out;
		}

		constexpr THISTYPE Abs() const noexcept
		{
			if (std::is_constant_evaluated() || std::is_integral_v<TY>)
			{
				THISTYPE out;

				for (size_t i = 0; i < SIZE; i++)
					out[i] = std::abs(at(i));

				return out;
			}
			else if (PAD || SIZE % 4 == 0)
			{
				THISTYPE result;
				const auto zero = simde_mm_set1_ps(0.0f);

				auto helper = [&]<int i>()
				{
					const auto a		= GetSIMD(i);
					constexpr auto m	= this->template GetSIMD128Mask<i>();
					const auto a_abs	= simde_x_mm_abs_ps(a);
					const auto blended	= simde_mm_blend_ps(a_abs, zero, m);

					result.SetSIMD(blended, i);
				};

				bchelper<SIZE, this->GetSIMDWidth()>(helper);

				return result;
			}
		}

		constexpr THISTYPE Compare(const THISTYPE& rhs, const TY e) const noexcept
		{
			const auto diff = (*this - rhs).Abs();

			if (std::is_constant_evaluated() || std::is_integral_v<TY> || !PAD)
			{
				THISTYPE out;

				for (size_t i = 0; i < SIZE; i++)
					out[i] = std::abs(at(i)) < e ? 0 : 1;

				return out;
			}
			else if (PAD || SIZE % 4 == 0)
			{
				THISTYPE result;

				const auto v_e	= simde_mm_set1_ps(e);
				const auto one  = simde_mm_set1_ps(1.0f);
				const auto z	= simde_mm_set1_ps(0.0f);

				auto helper = [&]<int i>()
				{
					const		auto a			= diff.GetSIMD(i);
					constexpr	auto m			= this->template GetSIMD128Mask<i>();
					const		auto a_cmp		= simde_mm_cmp_ps(a, v_e, SIMDE_CMP_LE_OS);

					if ((i + 1) * 4 <= SIZE)
					{
						const auto b = simde_mm_blendv_ps(z, one, a_cmp);
						result.SetSIMD(b, i);
					}
					else
					{
						const auto b0 = simde_mm_blendv_ps(z, one, a_cmp);
						const auto b1 = simde_mm_blend_ps(b0, z, m);
						result.SetSIMD(b1, i);
					}
				};

				bchelper<SIZE, this->GetSIMDWidth()>(helper);

				return result;
			}
		}

		constexpr static const size_t size() noexcept
		{
			return SIZE;
		}


		constexpr static bool Padded() { return PAD; }

		template<size_t beginIdx, size_t endIdx>
		constexpr auto Slice() const noexcept
		{
			static_assert(beginIdx < endIdx, "Invalid Arguments");
			static_assert(beginIdx < size(), "Invalid Arguments");
			static_assert(endIdx   < size(), "Invalid Arguments");
			constexpr size_t count = endIdx - beginIdx;

			Vect<count, TY> out;

			for (size_t i = beginIdx; i < endIdx; i++)
				out[i] = vector[i + beginIdx];

			return out;
		}

		auto xyz() const noexcept requires (std::is_same_v<THISTYPE, Vect<4, float, true>>)
		{
			Vect<3, float, true> out;
		    out.SetSIMD(GetSIMD());
			return out;
		}

		auto xyz() const noexcept requires (std::is_same_v<THISTYPE, Vect<4, double, true>>)
		{
			Vect<3, double, true> out;
			out.SetSIMD(GetSIMD(0), 0);
			out.SetSIMD(GetSIMD(1), 1);
			return out;
		}

		operator TY* () noexcept { return vector; }


		operator simde__m128 () const noexcept requires(std::is_same_v<decltype(GetSIMD()), simde__m128> && PAD)
		{
			return GetSIMD();
		}

		operator simde__m128d () const noexcept requires(std::is_same_v<decltype(GetSIMD()), simde__m128d> && PAD)
		{
			return GetSIMD();
		}
	};


	/************************************************************************************************/


	template<Scaler_t TY_S, typename TY_Vs, size_t ELEMENT_COUNT = 1>
	[[nodiscard]] constexpr Vect<ELEMENT_COUNT, TY_Vs> operator * (TY_S scaler, const Vect<ELEMENT_COUNT, TY_Vs>& v)// scaler multiply
	{
		auto V_out = v;

		for (auto& Vs : V_out)
			Vs *= scaler;

		return V_out;
	}


	/************************************************************************************************/


	template<Scaler_t TY_S, Vector_t TY_Vs, bool TY_pad, size_t ELEMENT_COUNT = 1>
	[[nodiscard]] constexpr Vect<ELEMENT_COUNT, TY_Vs> operator * (const Vect<ELEMENT_COUNT, TY_Vs, TY_pad>& v, TY_S scaler)// scaler multiply
	{
		auto V_out = v;

		for (auto& Vs : V_out)
			Vs = (TY_Vs)(Vs * scaler);

		return V_out;
	}

	/************************************************************************************************/


	template<Scaler_t TY_S, Vector_t TY_Vs, size_t ELEMENT_COUNT = 1>
	[[nodiscard]] constexpr Vect<ELEMENT_COUNT, TY_Vs> operator / (const Vect<ELEMENT_COUNT, TY_Vs>& v, TY_S scaler)// scaler multiply
	{
		auto V_out = v;

		for (auto& Vs : V_out)
			Vs /= scaler;

		return V_out;
	}


	/************************************************************************************************/


	template<typename TY_Vs, size_t ELEMENT_COUNT = 1>
	[[nodiscard]] constexpr Vect<ELEMENT_COUNT, TY_Vs> operator / (const Vect<ELEMENT_COUNT, TY_Vs>& v1, const Vect<ELEMENT_COUNT, TY_Vs>& v2)// scaler multiply
	{
		auto V_out = v1;

		for (size_t I = 0; I < ELEMENT_COUNT; ++I)
			V_out[I] = v1[I] / v2[I];

		return V_out;
	}

	template<typename TY_Vs, size_t ELEMENT_COUNT = 1>
	[[nodiscard]] constexpr Vect<ELEMENT_COUNT, TY_Vs> operator - (const Vect<ELEMENT_COUNT, TY_Vs>& v)// 
	{
		TY_Vs V_out;

		for (size_t I = 0; I < ELEMENT_COUNT; ++I)
			V_out[I] = -v[I];

		return V_out;
	}


	/************************************************************************************************/


	typedef Vect<2>					Vect2;
	typedef Vect<3, float, true>	Vect3;
	typedef Vect<4>					Vect4;
	
	typedef Vect<2, double, true>	double2;
	typedef Vect<3, double, true>	double3;
	typedef Vect<4, double, true>	double4;
	
	typedef Vect<2, uint32_t>	uint2;
	typedef Vect<2, uint64_t>	uint2_64;

	typedef Vect<3, uint32_t>	uint3;
	typedef Vect<4, uint32_t>	uint4;
	typedef Vect<4, uint8_t>	uint4_8;
	typedef Vect<4, uint16_t>	uint4_16;
	typedef Vect<4, uint64_t>	uint4_32;

	typedef Vect<2, int> int2;
	typedef Vect<3, int> int3;
	typedef Vect<4, int> int4;


	typedef std::uint32_t uint;


	/************************************************************************************************/


	template<typename TY_> float2 operator + (const float2& LHS, const Vect<2, TY_>& RHS) noexcept { return{ (float)(LHS.x + RHS[0]), (float)(LHS.y + RHS[1]) };}
	template<typename TY_> float2 operator - (const float2& LHS, const Vect<2, TY_>& RHS) noexcept { return{ (float)(LHS.x - RHS[0]), (float)(LHS.y - RHS[1]) };}
	template<typename TY_> float2 operator * (const float2& LHS, const Vect<2, TY_>& RHS) noexcept { return{ (float)(LHS.x * RHS[0]), (float)(LHS.y * RHS[1]) };}
	template<typename TY_> float2 operator / (const float2& LHS, const Vect<2, TY_>& RHS) noexcept { return{ (float)(LHS.x / RHS[0]), (float)(LHS.y / RHS[1]) };}

	template<typename TY_> Vect<2, TY_> operator * (const Vect<2, TY_> LHS, const float2 RHS) noexcept { return{ (TY_)(LHS[0] * RHS[0]), (TY_)(LHS[1] * RHS[1]) }; }

	inline float2 operator * (const float   lhs, const float2 rhs) noexcept  { return float2{ lhs, lhs } * rhs; }


	/************************************************************************************************/

	
    using float3 = Vect<3, float, true>;
	using float4 = Vect<4, float, true>;

	const float3 BLACK	= float3(0.0f, 0.0f, 0.0f);
	const float3 WHITE	= float3(1.0f, 1.0f, 1.0f);
	const float3 RED	= float3(1.0f, 0.0f, 0.0f);
	const float3 BLUE	= float3(0.0f, 0.0f, 1.0f);
	const float3 GREEN	= float3(0.0f, 1.0f, 0.0f);
	const float3 PURPLE	= float3(1.0f, 0.0f, 1.0f);


	template<typename TY>
	TY clamp(const TY& min, const TY& v, const TY& max) noexcept
	{
		return Min(Max(min, v), max);
	}


	float saturate(float x) noexcept;
	float3 saturate(float3 v) noexcept;
	float3 TripleProduct(const float3 A, const float3 B, const float3 C) noexcept;
	float3 operator* (float s, float3 V) noexcept;
	float3 RotateVectorAxisAngle(float3 N, float a, float3 V) noexcept;


	/************************************************************************************************/


	union alignas(16) Quaternion
	{
	public:
		constexpr Quaternion() {}
		constexpr Quaternion(simde__m128 in) { floats = in; }


		constexpr explicit Quaternion(const float3& vector, float scaler)
		{
			if (std::is_constant_evaluated())
			{
				x = vector.x;
				y = vector.y;
				z = vector.z;
				w = scaler;
			}
			else
				floats = simde_mm_set_ps(scaler, vector[2], vector[1], vector[0]);
		}


		constexpr Quaternion(float X, float Y, float Z, float W)
		{
			if consteval
			{
				x = X;
				y = Y;
				z = Z;
				w = W;
			}
			else
			{
				floats = simde_mm_set_ps(W, Z, Y, X);
			}
		}


		explicit Quaternion(float* in)
		{
			simde_mm_store_ps( in, floats );
		}


		constexpr explicit Quaternion(const Quaternion& in) :
			floats( in.floats )	{}


		constexpr explicit Quaternion( float dX, float dY, float dZ ) // Degrees to Quat
		{
			Quaternion X, Y, Z;
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


		constexpr Quaternion& operator *= (const Quaternion& rhs ) noexcept
		{
			(*this) = GrassManProduct(*this, rhs);
			return (*this);
		}


		constexpr Quaternion& operator = (const Quaternion& rhs) noexcept
		{
			floats = rhs.floats;
			return (*this);
		}


		constexpr Quaternion& operator = (const  simde__m128& rhs ) noexcept
		{
			floats = rhs;
			return (*this);
		}


		Quaternion operator * (const Quaternion& q) const noexcept
		{
			return GrassManProduct(q, *this);
		}


		float& operator [] (const size_t index)		{ return GetElement_ref	(floats, index); }
		float operator  [] (const size_t index) const	{ return GetElement		(floats, index); }

		float& operator [] (const int index)			{ return GetElement_ref	(floats, index); }
		float operator  [] (const int index) const	{ return GetElement		(floats, index); }


		constexpr operator		  float* ()				{ return (float*)&floats; }
		constexpr operator const  float* () const		{ return (float*)&floats; }
		constexpr operator		  simde__m128 ()		{ return floats; }
		constexpr operator const simde__m128 () const	{ return floats; }


		template< typename Ty_2 >
		constexpr Quaternion& operator = (const float4& rhs)
		{
			x = rhs.x;
			y = rhs.y;
			z = rhs.z;
			w = rhs.w;

			return (*this);
		}


		constexpr Quaternion Conjugate() const noexcept
		{
			Quaternion conjugate;
			if consteval
			{
				conjugate.x = -x;
				conjugate.y = -y;
				conjugate.z = -z;
				conjugate.w = w;
			}
			else
			{
				conjugate = simde_mm_mul_ps(simde_mm_set_ps(1, -1, -1, -1), floats);
			}

			return conjugate;
		}

		constexpr Quaternion Inverse() const { return  Conjugate(); }

		float dot(Quaternion rhs) const noexcept
		{ 
			return V().dot(rhs.V()) + w * rhs.w; 
		}


		constexpr Quaternion operator	 / (float denom) noexcept { return { x / denom , y / denom , z / denom , w / denom }; }
		constexpr Quaternion operator + (Quaternion RHS) noexcept { return { x + RHS.x, y + RHS.y, z + RHS.z, w + RHS.w }; }


		constexpr float3 XYZ() const noexcept { return float3(x, y, z); }
		constexpr float3 V()   const noexcept { return float3(x, y, z); }


		//constexpr 
	    float Magnitude() const noexcept
		{
			if consteval
			{
				auto m = sqrt(x * x + y * y + z * z + w * w);
				return m;
			}
			else
			{
				simde__m128 q2 = simde_mm_mul_ps(floats, floats);
				q2 = simde_mm_hadd_ps(q2, q2);
				q2 = simde_mm_hadd_ps(q2, q2);
				return GetLast(q2);
			}
		}


		//constexpr 
	    Quaternion& normalize() noexcept
		{
			float mag2 = Magnitude();
			if( mag2 != 0 && ( fabs( mag2 - 1.0f ) > .00001f ) )
			{
				if consteval
				{
					float mag = sqrt(mag2);
					w = w / mag;
					x = x / mag;
					y = y / mag;
					z = z / mag;
				}
				else
				{
					simde__m128 rsq = simde_mm_rsqrt_ps(simde_mm_set1_ps(mag2));
					floats = simde_mm_mul_ps(rsq, floats);
				}
				
			}
			return *this;
		}


		//constexpr 
	    Quaternion normal() const noexcept
		{
			float mag2 = Magnitude();
			if (mag2 != 0 && (fabs(mag2 - 1.0f) > .00001f))
			{
#if 0
				if consteval
				{
					const float mag = sqrt(mag2);
					return Quaternion{
						.x = x / mag,
						.y = y / mag,
						.z = z / mag,
						.w = w / mag
					};
				}
				else
#endif
				{
					simde__m128 rsq = simde_mm_rsqrt_ps(simde_mm_set1_ps(mag2));
					return simde_mm_mul_ps(rsq, floats);
				}
			}
			else
				return *this;
		}


		constexpr void Zero() 
		{
			if (std::is_constant_evaluated())
			{
				x = 0;
				y = 0;
				z = 0;
				w = 0;
			}
			else
				floats = simde_mm_set1_ps(0);
		}

		constexpr static Quaternion Identity()
		{
			Quaternion Q(0.0f, 0.0f, 0.0f, 1.0f);
			return Q;
		}

		constexpr void Serialize(auto& ar)
		{
			ar& floats;
		}

		constexpr static size_t Size() { return 4; }

		struct
		{
			float x, y, z, w;
		};

		 simde__m128 floats;
	};


	/************************************************************************************************/


	inline float3		operator * (const Quaternion P, const float3 V) noexcept
	{
		auto v		= -1 * P.XYZ();
		auto vXV	= v.cross(V);
		auto ret	= float3(V + (vXV * (2 * P.w)) + (v.cross(vXV) * 2));

		return ret;
	}


	inline Quaternion operator * (const Quaternion Q, const float scaler) noexcept
	{
		simde__m128 r = Q;
		simde__m128 s = simde_mm_set1_ps(scaler);
		return simde_mm_mul_ps(r, s);
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
	

	inline float3	Vect3ToFloat3(const Vect3 R3)  noexcept { return {R3[0], R3[1], R3[2]}; }
	inline Vect3	Float3ToVect3(const float3 R3) noexcept { return {R3[0], R3[1], R3[2]}; }
	
	inline float4	Vect4ToFloat4(const Vect4  R4) noexcept { return {R4[0], R4[1], R4[2], R4[3]}; }
	inline Vect4	Float4ToVect4(const float4 R4) noexcept { return {R4[0], R4[1], R4[2], R4[3]}; }


	/************************************************************************************************/


	inline float Vect3FDot(const Vect3& lhs, const Vect3& rhs) noexcept
	{
		auto temp1 = simde_mm_set_ps(0, lhs.vector[2], lhs.vector[1], lhs.vector[0]);
		auto temp2 = simde_mm_set_ps(0, rhs.vector[2], rhs.vector[1], rhs.vector[0]);

		simde__m128 res = simde_mm_dp_ps(temp1, temp2, 0xFF);

		return GetFirst(res);
	}


	/************************************************************************************************/


	constexpr inline float Vect4FDot(const Vect4 lhs, const Vect4 rhs)
	{
		if (std::is_constant_evaluated())
		{
			Vect4::Type sum = static_cast<Vect4::Type>(0);

			for (size_t i = 0; i < 4; i++)
				sum += lhs[i] * rhs[i];
			
			return sum;
		}
		else
		{
			const simde__m128 temp1 = simde_mm_loadu_ps(lhs.vector);
			const simde__m128 temp2 = simde_mm_loadu_ps(rhs.vector);
			const simde__m128 res	= simde_mm_dp_ps(temp1, temp2, 0xFF);

			return GetFirst(res);
		}
	}


	/************************************************************************************************/


	template<typename TY, size_t Size>
	union MatrixOptionalVectorData
	{
		static inline constexpr bool Enabled = false;
		static inline constexpr bool SIMDEnabled = false;
		static inline constexpr bool SIMD256Enabled = false;



		const auto	At128(size_t idx)const noexcept
		{
			if constexpr (std::is_same_v<float, TY>)
			{
				return simde_mm_blend_ps(
					simde_mm_set1_ps(0.0),
					simde_mm_load_ps(this + idx),
					(0b1111) << Size);
			}
			if constexpr (std::is_same_v<double, TY>)
			{
				return simde_mm_blend_ps(
					simde_mm_set1_pd(0.0),
					simde_mm_load_pd(this + idx),
					(0b1111) << Size);
			}
		}

		//auto			At256(size_t idx)		noexcept { return simde__m256{}; }
		//const	auto	At256(size_t idx) const	noexcept { return simde__m256{}; }

		auto			At(size_t idx)			noexcept { return At128(idx); }
		const	auto	At(size_t idx) const	noexcept { return At128(idx); }

		constexpr size_t size()		const noexcept { return 0; }
		constexpr size_t size256()	const noexcept { return 0; }
	};

	template<std::size_t Size>
	union MatrixOptionalVectorData<int, Size>
	{
		static inline constexpr bool Enabled = false;
		static inline constexpr bool SIMDEnabled = false;
		static inline constexpr bool SIMD256Enabled = false;

		constexpr size_t size()		const noexcept { return 0; }
		constexpr size_t size256()	const noexcept { return 0; }
	};

	template<size_t Size> requires (Size > 4 && Size % 8 == 0)
    union MatrixOptionalVectorData<float, Size>
	{
		simde__m128 vectors[Size / 4];
		simde__m256 vectors256[Size / 8];

		static inline constexpr bool Enabled = true;
		static inline constexpr bool SIMDEnabled = true;
		static inline constexpr bool SIMD256Enabled = true;

		        simde__m128& F128At(size_t idx)			noexcept { return vectors[idx]; }
		const	simde__m128& F128At(size_t idx) const	noexcept { return vectors[idx]; }

		        simde__m256& F256At(size_t idx)			noexcept { return vectors256[idx]; }
		const	simde__m256& F256At(size_t idx) const	noexcept { return vectors256[idx]; }

		auto&			At(size_t idx)			noexcept { return At128(idx); }
		const	auto&	At(size_t idx) const	noexcept { return At128(idx); }

		auto&			At128(size_t idx)		noexcept { return vectors[idx]; }
		const	auto&	At128(size_t idx) const	noexcept { return vectors[idx]; }

		        auto& At256(size_t idx)			noexcept { return vectors256[idx]; }
		const	auto& At256(size_t idx) const	noexcept { return vectors256[idx]; }


		constexpr size_t size()		const noexcept { return Size / 4; }
		constexpr size_t size256()	const noexcept { return Size / 8; }
	};

	template<size_t Size> requires (Size >= 4 && Size % 4 == 0)
    union MatrixOptionalVectorData<double, Size>
	{
		static inline constexpr bool Enabled = true;
		static inline constexpr bool SIMDEnabled = true;
		static inline constexpr bool SIMD256Enabled = true;

		simde__m128d vectors[Size / 2];
		simde__m256d vectors256[Size / 4];

		        simde__m256d& D256At(size_t idx)		noexcept { return vectors256[idx]; }
		const	simde__m256d& D256At(size_t idx) const	noexcept { return vectors256[idx]; }

		auto&			At128(size_t idx)		noexcept { return vectors[idx]; }
		const	auto&	At128(size_t idx) const	noexcept { return vectors[idx]; }

		auto&			At256(size_t idx)		noexcept { return vectors[idx]; }
		const	auto&	At256(size_t idx) const	noexcept { return vectors[idx]; }



		constexpr size_t size()		const noexcept { return Size / 2; }
		constexpr size_t size256()	const noexcept { return Size / 4; }
	};


#pragma warning(push)
#pragma warning(disable : 4324)
	// Row Major
	template<const int Columns, const int Rows, Scaler_t Ty = float, bool Pad = false>
	union Matrix
	{
	private:
		template<typename TY_tuple, int ... seq>
		constexpr void helper_S(const TY_tuple& tuple, const std::integer_sequence<int, seq...> x) noexcept
		{
			auto setScaler = [&](size_t idx, const auto s)
				{
					const size_t x = idx % Columns;
					const size_t y = idx / Columns;

					SetAt(x, y, s);
				};

			(setScaler(seq, std::get<seq>(tuple)), ...);
		}

		template<typename TY_tuple, int ... ints>
		constexpr void helper_V(TY_tuple&& tuple, const std::integer_sequence<int, ints...> x) noexcept
		{
			auto setVector = [&](size_t idx, const auto& v)
				{
					Row(idx) = v;
				};

			(setVector(ints, std::get<ints>(tuple)), ...);
		}

		template<typename TY_vect, int ... ints>
		constexpr static auto extractVect(const TY_vect& vect, const std::integer_sequence<int, ints...> x) noexcept
		{
			return std::forward_as_tuple(vect[ints]...);
		}


		template<typename TY>
		constexpr static auto BuildTuple(const TY& value) noexcept
		{
			return std::forward_as_tuple(value);
		}


		template<size_t vectorSize>
		constexpr static auto BuildTuple(const Vect<vectorSize, Ty>& vect)
		{
			return extractVect(vect, std::make_integer_sequence<int, vectorSize>());
		}

		constexpr static auto BuildTuple(const float2& vect) noexcept
		{
			return std::forward_as_tuple(vect[0], vect[1]);
		}


		constexpr static auto BuildTuple(const float3& vect) noexcept
		{
			return std::forward_as_tuple(vect[0], vect[1], vect[2]);
		}


		constexpr static auto BuildTuple(const float4& vect) noexcept
		{
			return std::forward_as_tuple(vect[0], vect[1], vect[2], vect[3]);
		}


		constexpr static auto BuildTuple(const double2& vect) noexcept
		{
			return std::forward_as_tuple(vect[0], vect[1]);
		}


		constexpr static auto BuildTuple(const double3& vect) noexcept
		{
			return std::forward_as_tuple(vect[0], vect[1], vect[2]);
		}

		constexpr static auto BuildTuple(const double4& vect) noexcept
		{
			return std::forward_as_tuple(vect[0], vect[1], vect[2], vect[4]);
		}


		template<typename TY, size_t C, bool pad>
		constexpr static auto BuildTuple(const Vect<C, TY, pad>& vect) noexcept requires(C == 3)
		{
			return std::forward_as_tuple(vect[0], vect[1], vect[2]);
		}

		template<typename TY, size_t C, bool pad>
		constexpr static auto BuildTuple(const Vect<C, TY, pad>& vect) noexcept requires(C == 4)
		{
			return std::forward_as_tuple(vect[0], vect[1], vect[2], vect[3]);
		}


		template<typename ... TY_args>
		constexpr static auto BuildTuple(const TY_args& ... args) noexcept
		{
			return std::tuple_cat(BuildTuple(args)...);
		}


	public:
		using VectorType = Vect<Columns, Ty, Pad>;

		static constexpr size_t PaddingSize() noexcept
		{
			if (Pad)
			{
				return (sizeof(VectorType) == (sizeof(Ty) * Columns)) ?
					0 :
					(sizeof(VectorType) - (sizeof(Ty) * Columns)) / sizeof(Ty);
			}
			else
				return 0;
		}

		constexpr static size_t PadSize = PaddingSize();
		using THIS_TYPE		= Matrix<Columns, Rows, Ty, Pad>;
		using VectorView	= MatrixOptionalVectorData<Ty, Columns * Rows + PadSize>;

		constexpr Matrix() = default;
		constexpr Matrix(const THIS_TYPE& initial) = default;

		~Matrix() = default;

		template<typename ... TY_ARGS>
		constexpr explicit Matrix(const TY_ARGS& ... args) noexcept
		{
			const std::tuple tuple = BuildTuple(args...);
			constexpr size_t valueCount = std::tuple_size_v<decltype(tuple)>;
			constexpr auto indexes = std::make_integer_sequence<int, valueCount>{};

			static_assert(valueCount <= Columns * Rows, "Input value count must be less than container size!");

			helper_S(tuple, indexes);

			auto setValue = [&](size_t idx, auto v)
				{
					const size_t x = idx % Columns;
					const size_t y = idx / Rows;

					SetAt(x, y, v);
				};

			for (size_t I = valueCount; I < Columns * Rows; I++)
				setValue(I, 0.0f);
		}

		template<Scaler_t ... TY_ARGS>
		constexpr explicit Matrix(TY_ARGS&& ... args) noexcept requires (sizeof ... (TY_ARGS) == Columns * Rows)
		{
			auto forwarded_args = std::forward_as_tuple(args...);
			helper_S(forwarded_args, std::make_integer_sequence<int, sizeof ... (TY_ARGS)>());
		}

		template<Vector_t ... TY_ARGS>
		constexpr explicit Matrix(TY_ARGS&& ... args) noexcept requires (sizeof ... (TY_ARGS) == Rows)
		{
			auto forwarded_args = std::forward_as_tuple(args...);
			helper_V(forwarded_args, std::make_integer_sequence<int, sizeof ... (TY_ARGS)>());
		}

		template<Scaler_t TY_RHS, bool Pad_RHS>
		constexpr explicit Matrix(const Matrix<Columns, Rows, TY_RHS, Pad_RHS>& rhs) noexcept
		{
			if constexpr (std::is_same_v<TY_RHS, Ty>)
			{
				memcpy(Data(), rhs.Data(), sizeof(THIS_TYPE));
			}
			if constexpr (std::is_same_v<double, Ty> && std::is_same_v<float, TY_RHS>)
			{
				for (size_t i = 0; i < vectorView.size256(); i++)
					vectorView.D256At(i) = simde_mm256_cvtps_pd(rhs.vectorView.At(i));
			}
			if constexpr (std::is_same_v<float, Ty> && std::is_same_v<double, TY_RHS>)
			{
				for (size_t i = 0; i < vectorView.size(); i++)
				{
					auto a = simde_mm_cvtpd_ps(rhs.vectorView.At128(i * 2 + 0));
					auto b = simde_mm_cvtpd_ps(rhs.vectorView.At128(i * 2 + 1));
					const auto t = simde_mm_shuffle_ps(
						a, b,
						0b00000000 | 0b00000100 | 0b00000000 | 0b01000000);
					
				    vectorView.At(i) = t;
				}
			}
		}

		constexpr explicit Matrix(Scaler_t auto s) noexcept
		{
			for (auto& r : matrix)
				for (auto c : r)
					c = s;
		}

		constexpr Matrix operator * (const float rhs) const
		{
			Matrix out = *this;

			for (auto& r : out.rows)
				r *= rhs;

			return out;
		}


		template<const int RHS_Columns, const int RHS_Rows, bool RHS_Pad, typename RHS_TY>
		constexpr auto operator * (const Matrix<RHS_Columns, RHS_Rows, RHS_TY, RHS_Pad>& rhs) const noexcept 
		{
			static_assert(Columns == RHS_Rows, "Columns and Rows incompatible");
			Matrix<RHS_Columns, Rows, Ty, RHS_Pad | Pad> out;

			auto helper_internal = []<int ... ints>(const std::index_sequence<ints...>&, const auto& action) noexcept
			{
				(action.template operator() < ints > (), ...);
			};

			auto helper = [&](const auto& action) noexcept
				{
					constexpr size_t end = Columns;
					helper_internal(std::make_index_sequence<end>(), action);
				};

			for (size_t i = 0; i < Rows; ++i)
			{
				auto v = Vect<RHS_Columns, Ty, RHS_Pad | Pad>::Zero();
				const auto& r = Row(i);

				auto calculateRow = [&]<uint32_t i2>
				{
					auto asdf = i2;
					const auto& t = rhs.Row(i2);
					auto rb = r.template BroadcastElement<i2, RHS_Columns>();

					v.MAdd(t, rb);
				};

				helper(calculateRow);

				out[i] = v;
				int x = 0;
			}

			return out;
		}

		template<const int RHS_Columns, const int RHS_Rows, bool RHS_Pad, typename RHS_TY>
		constexpr auto operator * (const Matrix<RHS_Columns, RHS_Rows, RHS_TY, RHS_Pad>& rhs) const noexcept requires(VectorView::SIMD256Enabled && decltype(rhs)::VectorView::SIMD256Enabled)
		{
			static_assert(Columns == RHS_Rows, "Columns and Rows incompatible");
			Matrix<RHS_Columns, Rows, Ty, RHS_Pad | Pad> out;

			auto helper_internal = []<int ... ints>(const std::index_sequence<ints...>&, const auto& action) noexcept
			{
				(action.template operator() < ints > (), ...);
			};

			auto helper = [&](const auto& action) noexcept
				{
					constexpr size_t end = Columns;
					helper_internal(std::make_index_sequence<end>(), action);
				};

			for (size_t i = 0; i < Rows; ++i)
			{
				auto v = Vect<RHS_Columns, Ty, RHS_Pad | Pad>::Zero();
				const auto& r = Row(i);

				auto calculateRow = [&]<uint32_t i2>
				{
					const auto& t = rhs.Row(i2);
					auto rp = r.template BroadcastElement256<i2, RHS_Columns>();

					v.MAdd(t, rp);
				};

				helper(calculateRow);

				out[i] = v;
			}

			return out;
		}


		friend constexpr THIS_TYPE operator * (const Scaler_t auto lhs, const THIS_TYPE& rhs) noexcept
		{
			return rhs * lhs;
		}

		constexpr THIS_TYPE& operator = (const THIS_TYPE& rhs) noexcept
		{
			memcpy(&matrix, &rhs.matrix, sizeof(THIS_TYPE));
			return *this;
		}

		constexpr 		Ty& At(const size_t column, const size_t row)		noexcept { return matrix[row][column]; }
		constexpr const	Ty& At(const size_t column, const size_t row) const	noexcept { return matrix[row][column]; }


		constexpr void		SetAt(const size_t c, const size_t r, const Ty val)	noexcept { matrix[r][c] = val; }

#ifdef __cpp_multidimensional_subscript
		constexpr		Ty& operator[] (const size_t c, const size_t r)			noexcept { return At(c, r); }
		constexpr const	Ty& operator[] (const size_t c, const size_t r) const	noexcept { return At(c, r); }
#endif

		constexpr 		Ty& operator() (const size_t c, const size_t r)			noexcept { return At(c, r); }
		constexpr const	Ty& operator() (const size_t c, const size_t r) const	noexcept { return At(c, r); }


		constexpr		VectorType& operator[] (const int r)			{ return rows[r]; }
		constexpr const VectorType& operator[] (const int r) const		{ return rows[r]; }

		constexpr		VectorType& operator[] (const size_t r)			{ return rows[r]; }
		constexpr const VectorType& operator[] (const size_t r) const	{ return rows[r]; }

		operator Ty* ()			noexcept { return (Ty*)matrix; }
		operator const  Ty* () const	noexcept { return (Ty*)matrix; }

		Ty* Data()			noexcept { return (Ty*)matrix; }
		const	Ty* Data() const	noexcept { return (Ty*)matrix; }

		void Serialize(auto& ar)
		{
			ar.SerializeBuffer((char*)this, sizeof(THIS_TYPE));
		}

		constexpr static Matrix Identity() noexcept requires (Columns == Rows)
		{
			Matrix m = Zero();

			for (size_t i = 0u; i < Columns; i++)
				m(i, i) = static_cast<Ty>(1);

			return m;
		}


		constexpr static THIS_TYPE Zero() noexcept
		{
			if consteval
			{
				return THIS_TYPE{ 0 };
			}
			else
			{
				THIS_TYPE m;
				memset(&m, 0, sizeof(m));

				return m;
			}
		}


		VectorType& Row(const size_t columnIdx)	noexcept
		{
			return rows[columnIdx];
		}

		const VectorType& Row(const size_t rowIdx) const noexcept
		{
			return rows[rowIdx];
		}

		constexpr const VectorType	GetRow(const size_t rowIdx)	const noexcept
		{
			Vect<Columns, Ty> out;
			for (auto [idx, s] : zip(iota(0), matrix[rowIdx]))
				out[idx] = s;

			return out;
		}

		constexpr void SetRow(const size_t rowIdx, Vector_t auto&& v) noexcept
		{
			if (std::is_constant_evaluated())
			{
				for (auto [idx, s] : zip(iota(0), v))
					matrix[rowIdx][idx] = s;
			}
			else
				rows[rowIdx] = v;
		}


		constexpr VectorType Column(const size_t columnIdx) noexcept
		{
			auto gatherVector = [&]<size_t ... Sequence>(std::integer_sequence<size_t, Sequence...> sequence) -> Vect<Rows>
			{
				return VectorType{ rows[Sequence][columnIdx]... };
			};

			return gatherVector(std::make_index_sequence<Rows>{});
		}

		constexpr void SetColumn(const size_t columnIdx, const VectorType& col) noexcept
		{
			auto gatherVector = [&]<size_t ... Sequence>(std::integer_sequence<size_t, Sequence...> sequence)
			{
				auto set = [&]<size_t idx>{ rows[idx][columnIdx] = col[idx]; };

				(set.template operator()<Sequence>(), ...);
			};

			gatherVector(std::make_index_sequence<Rows>{});
		}


		template<uint2 xy1, uint2 xy2>
		constexpr auto Slice() const noexcept
		{
			static constexpr uint2 wh = (xy2 - xy1 + uint2{ 1, 1 });
			constexpr bool PAD = (bool)(sizeof(Ty) * wh[0] >= 16u ? Pad : false);
			Matrix<wh[0], wh[1], Ty, PAD> out;

			for (uint32_t y = 0; y < wh[1]; y++)
			{
				for (uint32_t x = 0; x < wh[0]; x++)
					out[x, y] = At(xy1[0] + x, xy1[1] + y);
			}

			return out;
		}

		constexpr void SwapRows(size_t a, size_t b) noexcept
		{
			std::swap(rows[a], rows[b]);
		}

		constexpr void ScaleRow(size_t a, const Scaler_t auto s) noexcept
		{
			rows[a] *= s;
		}

		constexpr void SwapColumns(size_t a, size_t b) noexcept
		{
			const auto colA = Column(a);
			const auto colB = Column(b);

			SetColumn(a, colB);
			SetColumn(b, colA);
		}

		constexpr void ScaleColumn(size_t a, const Scaler_t auto s) noexcept
		{
			const auto colA = Column(a) * s;

			SetColumn(a, colA);
		}


		[[nodiscard]] constexpr Ty Det() const noexcept
		{
			float out = (float)0;

			return out;
		}

		constexpr THIS_TYPE Compare(const THIS_TYPE& rhs, const Ty epsilon) const noexcept
		{
			THIS_TYPE out;

			for (size_t i = 0; i < RowCount(); i++)
				out[i] = Row(i).Compare(rhs[i], epsilon);

			return out;
		}

		constexpr Matrix<Rows, Columns, Ty, Pad> Transpose() const noexcept
		{
			Matrix<Rows, Columns, Ty, Pad> m_transposed;

			for (size_t y = 0; y < Rows; ++y)
				for (size_t x = 0; x < Columns; ++x)
					m_transposed(x, y) = At(y, x);

			return m_transposed;
		}


		constexpr static size_t ColumnCount() noexcept
		{
			return Columns;
		}

		constexpr static size_t RowCount() noexcept
		{
			return Rows;
		}

		NO_UNIQUE_ADDRESS Ty				matrix[Rows][Columns + PaddingSize()];	// Row Major
		NO_UNIQUE_ADDRESS VectorType		rows[Rows];
		NO_UNIQUE_ADDRESS VectorView		vectorView;	// Optionally Exists, SIMD View
	};

#pragma warning(pop)


	template<typename Internal_TY>
	struct Matrix_GPU
	{
		Matrix_GPU() = default;
		Matrix_GPU(const Internal_TY& rhs) :
			m{ rhs.Transpose() } {}


		Matrix_GPU& operator = (const Internal_TY& rhs) noexcept
		{
			m = rhs.Transpose();
			return *this;
		}

		Internal_TY operator * (const Internal_TY& rhs) const noexcept
		{
			return m.Transpose() * rhs;
		}

		Internal_TY ToCPU() const noexcept
		{
			return m.Transpose();
		}

		Matrix_GPU Transpose() const noexcept
		{
			Matrix_GPU out{};
			out.m = m.Transpose();

			return out;
		}

		operator float*				()			noexcept  { return m; }
		operator const float*		()	const	noexcept  { return m; }

		operator Internal_TY () noexcept		{ return ToCPU(); }
		operator Internal_TY () const noexcept	{ return ToCPU(); }

		Internal_TY m;
	};


	/************************************************************************************************/


	inline float2 Mul(const Matrix<2, 2, float>& LHS, const float2& RHS)
	{
		float2 Out;

		for (size_t I = 0; I < 2; ++I)
			Out[I] = DotProduct2(LHS.matrix[I], RHS);

		return Out;
	}


	inline float3 Mul(const Matrix<3, 3, float>& LHS, const float3& RHS)
	{
		float3 Out;
		simde__m128 Temp;

		for (size_t I = 0; I < 2; ++I) {
			Temp = simde_mm_set_ps(0, LHS.matrix[I][2], LHS.matrix[I][1], LHS.matrix[I][0]);
			Out[I] = DotProduct3(Temp, RHS);
		}

		return Out;
	}


	Matrix<4, 4> InverseFast(const Matrix<4, 4>& in) noexcept;


	/************************************************************************************************/


	using float2x2 = Matrix<2,2>;
	using float3x3 = Matrix<3,3>;
	using float4x4 = Matrix<4,4, float, true>;

	using double2x2 = Matrix<2, 2, double, true>;
	using double3x3 = Matrix<3, 3, double, true>;
	using double4x4 = Matrix<4, 4, double, true>;

	using float3x3_GPU = Matrix_GPU<float3x3>;
	using float4x4_GPU = Matrix_GPU<float4x4>;


	double4x4	TranslationMatrix(const double3& POS);
	double4x4	TranslationMatrix(const double4& POS);
	float4x4	TranslationMatrix(const float3& POS);
	float4x4	ScaleMatrix(float3 POS);

	double4x4	ExtractScaleRotationMatrix(const double4x4&);
	float3		ExtractTranslationVector(const float4x4& m);
	double3		ExtractTranslationVector(const double4x4& m);

	float4x4	Quaternion2Matrix(const Quaternion q);
	Quaternion	Matrix2Quat(const float4x4& M);
	float4x4	Vector2RotationMatrix(const float3& Forward, const float3& Up, const float3& Right);
	Quaternion	Vector2Quaternion(const float3& Forward, const float3& Up, const float3& Right);
	Quaternion	PointAt(float3 A, float3 B, const float3 UpV = { 0.0f, 1.0f, 0.0f });
	float4x4	PerspectiveRH(const float FOV, const float minZ, const float maxZ, const float aspectRatio);



	/************************************************************************************************/


	float2x2 Inverse(const float2x2& m) noexcept;
	float4x4 Inverse(const float4x4& m) noexcept;

	double2x2 Inverse(const double2x2& m) noexcept;
	double4x4 Inverse(const double4x4& m) noexcept;

	float4x4 FastInverseNoScale(const float4x4 m);
	double4x4 FastInverseNoScale(const double4x4 m);


	/************************************************************************************************/


	inline float CalcMatrixTrace( float in[Matrix_Size] )
	{
		float sum = 0.0f;
		for( size_t itr = 0; itr < Matrix_Size; itr++ )
			for( size_t itr_2 = 0;itr_2 < 3; itr_2+= 4, itr += itr_2 )
				sum += in[itr];
		return sum;
	}



	struct DF
	{
		float3 high;
		float3 low;
	};

	DF GetDF(double3 xyz);


	/************************************************************************************************/


	template<typename TY>
	inline TY Lerp(const TY A, const TY B, float t){ return A * (1.0f - t) + t*B; }

	template<typename TY>
	inline TY lerp(const TY A, const TY B, float t) { return A * (1.0f - t) + t * B; }

	namespace Conversion
	{
		template< typename Ty >
		static Vect3 toVector3( Ty& Convert ) { return Vect3( Convert.x, Convert.y, Convert.z ); }

		template<typename Ty> Ty Vect3To( const Vect3& Convert )	{ return Ty{ Convert[0], Convert[1], Convert[2]}; }
		template<typename Ty> Ty Vect4To( const Vect4& Convert )	{ return Ty{ Convert[0], Convert[1], Convert[2], Convert[3]}; }

		template<typename Ty_Out, typename Ty_2>	Ty_Out Vect2To( const Ty_2& Convert) { return Ty_Out{ Convert[0], Convert[1] }; }

		template<typename TY>
		float2 Vect2TOfloat2(Vect<2, TY> Convert) { return float2{ (float)Convert[0], (float)Convert[1] }; }
	}


	/************************************************************************************************/


	template<const int Columns, const int Rows, bool Pad, typename TY>
	Matrix<Columns, Rows, TY, Pad> operator - (const Matrix<Columns, Rows, TY, Pad>& lhs, const Matrix<Columns, Rows, TY, Pad>& rhs) noexcept
	{
		static_assert(Columns == Rows, "Columns and Rows incompatible");
		Matrix<Columns, Rows, TY, Pad> out;

		auto helper_internal = []<int ... ints>(const std::index_sequence<ints...>&, const auto& action) noexcept
		{
			(action.template operator() < ints > (), ...);
		};

		auto helper = [&](const auto& action) noexcept
			{
				constexpr size_t end = Columns;
				helper_internal(std::make_index_sequence<end>(), action);
			};

		for (size_t i = 0; i < Rows; ++i)
		{
			auto v = Vect<Columns, TY, Pad>::Zero();

			const auto& r = lhs.Row(i);
			const auto& l = rhs.Row(i);

			out[i] = r - l;
		}

		return out;
	}


	/************************************************************************************************/


	inline auto operator * (const float4x4& lhs, const Vector_t auto& rhs)
	{
		Vect<rhs.size(), float, rhs.Padded()> out;
		
		for (size_t i = 0; i < rhs.size(); i++)
			out[i] = rhs.Dot(lhs[i]);

		return out;
	}


	inline float3 operator * (const float3x3& lhs, const float3 rhs)
	{// TODO: FAST PATH
		const Vect3   temp    = rhs;

		return Conversion::Vect3To<float3>({
				lhs[0].Dot(temp),
				lhs[1].Dot(temp),
				lhs[2].Dot(temp),
			});
	}


	/************************************************************************************************/


	inline auto operator * (const double4x4& lhs, const Vector_t auto& rhs)
	{
		double4 out{ 0, 0, 0, 0 };

		for (size_t i = 0; i < rhs.size(); i++)
			out[i] = rhs.Dot(lhs[i]);

		return out;
	}

	/************************************************************************************************/


	int			Exp( int32_t Number, uint32_t exp );
	Quaternion	MatrixToQuat(const float4x4& );


	float3	GetTranslation(const float4x4&);
	double3	GetTranslation(const double4x4&);
	float	dot(const float3 lhs, const float3 rhs);
	float	dot(const float4 lhs, const float4 rhs);

	auto mul(const auto& a, const auto& b)
	{
		return a * b;
	}


	/************************************************************************************************/


	void NumberToString(int32_t n, std::string& _Dest);


	void printfloat2(const float2& in);
	void printfloat3(const float3& in);
	void printfloat4(const float4& in);
	void printQuaternion(const Quaternion in);

	std::ostream& operator << (std::ostream& stream, float2 xyz);
	std::ostream& operator << (std::ostream& stream, float3 xyz);
	std::ostream& operator << (std::ostream& stream, float4 xyz);
	std::ostream& operator << (std::ostream& stream, Quaternion q);


	/************************************************************************************************/
}


/**********************************************************************

Copyright (c) 2015 - 2022 Robert May

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
