#pragma once
#pragma warning(disable : 4201)

// Includes

#include "BuildSettings.hpp"

#include <bit>
#include <bitset>
#include <cstring>
#include <initializer_list>
#include <limits>
#include <cmath>
#include <ostream>
#include <ranges>
#include <tuple>
#include <type_traits>

#include <simde/simde-common.h>
#if defined(__x86_64__) || defined(_M_X64)
#include <immintrin.h>
#include <simde/x86/avx2.h>
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
#define _MM_SHUFFLE(A, B, C, D) (A | (B<<2) | (C<<4) | (D<<6))
#endif

#ifdef max
#undef max
#endif

#ifdef min
#undef min
#endif

namespace FlexKit
{   /************************************************************************************************/

	//using std::views::iota;
	//using std::views::zip;

	template<class T>
	concept Scaler_t = std::is_integral_v<T> || std::is_floating_point_v<T> || std::is_convertible_v<T, int> || std::is_convertible_v<T, float> || std::is_convertible_v<T, double>;

	template<class T>
	concept Vector_t = !Scaler_t<T> && requires(T t)
	{
		t[0];
		t.size();
	};

	template<Scaler_t TY_1, Scaler_t TY_2> [[nodiscard]] constexpr auto Floor(const TY_1 x, const TY_2 y) noexcept { return (((TY_1)x > (TY_1)y) ? y : x); }
	template<Scaler_t TY_1, Scaler_t TY_2> [[nodiscard]] constexpr auto Min(const TY_1 x, const TY_2 y) noexcept { return (((TY_1)x > (TY_1)y) ? y : x); }
	template<Scaler_t TY_1, Scaler_t TY_2> [[nodiscard]] constexpr auto Max(const TY_1 x, const TY_2 y) noexcept { return (((TY_1)x > (TY_1)y) ? x : y); }
	template<Scaler_t TY_1, Scaler_t TY_2> [[nodiscard]] constexpr auto Fastmod(const TY_1 x, const TY_2 y) noexcept { return (((TY_1)x < (TY_1)y) ? x : x % y); }

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
	FLEXKITAPI template <typename T>
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

	FLEXKITAPI template <typename T>
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


	FLEXKITAPI inline static const double	pi = 3.141592653589793;
	FLEXKITAPI inline static const unsigned int Matrix_Size = 16;

	FLEXKITAPI template<size_t N>	inline int Factorial() { return N * Factorial<N - 1>(); }
	FLEXKITAPI template<>			inline int Factorial<1>() { return 1; }

	FLEXKITAPI template<typename Ty> constexpr Ty DegreetoRad(Ty deg) noexcept { return (Ty)(deg * pi) / 180; }
	FLEXKITAPI template<typename Ty> constexpr Ty RadToDegree(Ty deg) noexcept { return (Ty)(deg * 180.0f) / (float)pi; }


	FLEXKITAPI template<typename TY, typename TY_C, typename FN>
		TY GetMax(TY_C C, FN READ)
	{
		TY M = 0;
		for (auto& c : C)
			M = MAX(M, READ(c));

		return M;
	}

	FLEXKITAPI template<typename TY, typename TY_C, typename FN>
		TY GetMin(TY_C C, FN READ)
	{
		TY M = 0;
		for (auto& c : C)
			M = MIN(M, READ(c));

		return M;
	}

	FLEXKITAPI template<typename TY, typename FN>
		TY Simpson_Integrator(TY A, TY B, int N, FN F_X)
	{
		TY Acc = 0;
		TY S = (B - A) / N;

		TY S_1 = 0;
		TY S_2 = 0;

		for (int I = 1; I < N; I += 2)
			S_1 += F_X(A + I * S);
		for (int I = 2; I < N; I += 2)
			S_2 += F_X(A + I * S);

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


	FLEXKITAPI inline simde__m128 SSE_CopySign(simde__m128 sign, simde__m128 abs) noexcept
	{
		const uint32_t M1 = (1u << 31);
		const uint32_t M2 = ~(1u << 31);

		const simde__m128 Sgn = simde_mm_and_ps(sign, simde_mm_castsi128_ps(simde_mm_set1_epi32(M1)));
		const simde__m128 Abs = simde_mm_and_ps(abs, simde_mm_castsi128_ps(simde_mm_set1_epi32(M2)));
		const simde__m128 res = simde_mm_or_ps(Sgn, Abs);

		return res;
	}


	/************************************************************************************************/


	FLEXKITAPI inline simde__m128 SSE_ABS(simde__m128 abs) noexcept
	{
		return SSE_CopySign(simde_mm_set_ps1(1), abs);
	}


	/************************************************************************************************/


	FLEXKITAPI template<typename TY>
		inline TY Saturate(TY A) { return Max(0.0f, Min(1.0f, A)); }


	FLEXKITAPI template<typename TY>
		inline bool CompareFloats(TY A, TY B, TY E) { return fabs(A - B) <= E; }


	FLEXKITAPI template<typename TY>
		bool VectorCompare(TY A, TY B, float E) noexcept { return (A - B).magnitudeSq() < E * E; }


	FLEXKITAPI inline const	float* GetArray_ptr_const(const simde__m128& V)  noexcept { return reinterpret_cast<const float*>(&V); }
	FLEXKITAPI inline		float* GetArray_ptr(simde__m128& V)              noexcept { return reinterpret_cast<float*>(&V); }

	FLEXKITAPI inline const float& GetElement(const simde__m128& V, const size_t idx) noexcept { return GetArray_ptr_const(V)[idx]; }

	FLEXKITAPI inline float* GetElement_ptr(simde__m128& V, const size_t idx) noexcept { return GetArray_ptr(V) + idx; }
	FLEXKITAPI inline float& GetElement_ref(simde__m128& V, const size_t idx)  noexcept { return GetArray_ptr(V)[idx]; }

	FLEXKITAPI inline void SetElement(simde__m128& V, float X, const size_t idx) noexcept { GetArray_ptr(V)[idx] = X; }

	FLEXKITAPI inline float GetFirst(const simde__m128& V) noexcept { return GetElement(V, 0); } // Should Return the X Component
	FLEXKITAPI inline float GetLast(const simde__m128& V) noexcept { return GetElement(V, 2); } // SHould Return the W Component

	FLEXKITAPI inline void SetFirst(simde__m128& V, const float X) noexcept { return SetElement(V, X, 0); }
	FLEXKITAPI inline void SetLast(simde__m128& V, const float W) noexcept { return SetElement(V, W, 3); }


	/************************************************************************************************/


	FLEXKITAPI struct float2
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


		constexpr bool		operator == (const float2& rhs) const { return (rhs.x == x && rhs.y == y) ? true : false; }

		float& operator[] (const size_t i) noexcept
		{
			return i ? y : x;
		}

		constexpr const float operator[] (const size_t i) const noexcept
		{
			return i ? y : x;
		}

		constexpr float2 operator + (const float2& a) const noexcept { return float2(x + a.x, y + a.y); }
		constexpr float2 operator + (const float   a) const noexcept { return float2(x + a, y + a); }
		constexpr float2 operator - (const float2& a) const noexcept { return float2(x - a.x, y - a.y); }
		constexpr float2 operator - (const float   a) const noexcept { return float2(x - a, y - a); }
		constexpr float2 operator * (const float2& a) const noexcept { return float2(x * a.x, y * a.y); }
		constexpr float2 operator * (const float   a) const noexcept { return float2(x * a, y * a); }
		constexpr float2 operator / (const float2& a) const noexcept { return float2(x / a.x, y / a.y); }
		constexpr float2 operator / (const float   a) const noexcept { return float2(x / a, y / a); }
		constexpr float2 operator % (const float2& a) const noexcept { return float2(std::fmod(x, a.x), std::fmod(y, a.y)); }

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

		constexpr float2& operator -= (const float2& rhs) noexcept
		{
			*this = *this - rhs;
			return *this;
		}

		constexpr float2& operator -= (const float rhs) noexcept
		{
			*this = *this - rhs;
			return *this;
		}


		constexpr float2& operator += (const float2& rhs) noexcept
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


		constexpr void Add(const float2& lhs, const float2& rhs) noexcept
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


		constexpr operator float* () noexcept { return &x; }


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


		float2& normal() noexcept
		{
			const float m = magnitude();
			x /= m;
			y /= m;

			return *this;
		}

		float2 normalize() const noexcept
		{
			const float		m = magnitude();
			const float2	self = *this;

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

		constexpr static float2 Zero() noexcept
		{
			return { 0, 0 };
		}

		float x, y;

		constexpr static size_t Size() { return 2; }
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

	template<unsigned int SIZE, typename TY = float>
	class Vect
	{
		typedef Vect<SIZE, TY> THISTYPE;

		template<typename TY_tuple, int ... ints>
		constexpr void helper(const TY_tuple& tuple, const std::integer_sequence<int, ints...>&) noexcept
		{
			auto halp =
				[&]<int i = 0>
			{
				Vector[i] = static_cast<TY>(std::get<i>(tuple));
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


		constexpr Vect(TY n) noexcept
		{
			for (auto& e : Vector)
				e = n;
		}


		constexpr explicit Vect(const TY* const in) noexcept
		{
			for (size_t I = 0; I < SIZE; ++I)
				Vector[I] = in[I];
		}


		template<typename TY_2>
		constexpr Vect(Vect<SIZE, TY_2> in) noexcept
		{
			for (size_t I = 0; I < SIZE; ++I)
				Vector[I] = static_cast<TY>(in[I]);
		}


		template<typename ... TY_ARGS>
		constexpr Vect(TY_ARGS ... args) noexcept requires(sizeof ... (TY_ARGS) > 0)
		{
			static_assert(sizeof ... (args) <= SIZE, "Input value count must be less than container size!");

			const auto			tuple = BuildTuple(args...);
			constexpr size_t	valueCount = std::tuple_size_v<decltype(tuple)>;
			const auto			indexes = std::make_integer_sequence<int, valueCount>{};

			helper(tuple, indexes);
		}


		constexpr TY* begin() noexcept
		{
			return Vector;
		}


		constexpr TY* end() noexcept
		{
			return Vector + SIZE;
		}


		template<typename TY_i>
		[[nodiscard]] constexpr Vect<3, TY>	Cross(Vect<3, TY_i> rhs) noexcept
		{
			Vect<SIZE, TY> out;
			for (size_t i = 0; i < SIZE; ++i)
				out[i] = (Vector[(1 + i) % SIZE] * rhs[(2 + i) % SIZE]) - (rhs[(1 + i) % SIZE] * Vector[(2 + i) % SIZE]);

			return out;
		}


		template<typename TY_i>
		[[nodiscard]] constexpr TY Dot(const Vect<SIZE, TY_i>& rhs) const noexcept
		{
			TY dotproduct = 0;
			for (size_t i = 0; i < SIZE; ++i)
				dotproduct += rhs[i] * Vector[i];

			return dotproduct;
		}


		template<typename TY_i>
		[[nodiscard]] constexpr TY Dot(const Vect<SIZE, TY_i>* rhs_ptr) const noexcept
		{
			const auto& rhs = *rhs_ptr;
			Vect<SIZE> products;
			for (size_t i = 0; i < SIZE; ++i)
				products[i] += rhs[i] * Vector[i];

			return products.Sum();
		}


		[[nodiscard]] constexpr TY Norm(unsigned int exp = 2) noexcept
		{
			TY sum = 0;
			for (auto element : Vector)
			{
				TY product = 0;
				for (size_t i = 0; i < SIZE; i++)
					product *= element;
				sum += product;
			}

			return pow(sum, 1 / SIZE);
		}


		[[nodiscard]] constexpr TY Magnitude() noexcept
		{
			TY sum = 0;
			for (auto element : Vector)
			{
				sum += element * element;
			}

			return sqrt(sum);
		}


		constexpr		TY& at(const size_t index) noexcept
		{
			return Vector[index];
		}


		constexpr		TY& operator [](const size_t index) noexcept
		{
			return Vector[index];
		}


		constexpr const	TY& operator [](const size_t index) const noexcept
		{
			return Vector[index];
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
			TY sum = 0;
			for (auto element : Vector)
				sum += element;
			return sum;
		}

		template<int ... ints>
		constexpr inline auto _ceil_helper(const std::integer_sequence<int, ints...> x) const noexcept
		{
			return Vect{ ::ceil(Vector[ints])... };
		}

		[[nodiscard]] constexpr inline auto Ceil() const noexcept requires(std::is_floating_point_v<TY>)
		{
			const auto indexes = std::make_integer_sequence<int, SIZE>{};

			return _ceil_helper(indexes);
		}

		template<int ... ints>
		constexpr inline auto _floor_helper(const std::integer_sequence<int, ints...> x) const noexcept
		{
			return Vect{ ::floor(Vector[ints])... };
		}

		[[nodiscard]] constexpr inline auto Floor() const noexcept requires(std::is_floating_point_v<TY>)
		{
			const auto indexes = std::make_integer_sequence<int, SIZE>{};

			return _floor_helper(indexes);
		}

		[[nodiscard]] constexpr inline auto Min() const noexcept
		{
			TY min = std::numeric_limits<TY>::max();

			for (const auto& v : Vector)
				min = min < v ? min : v;

			return min;
		}

		[[nodiscard]] constexpr inline auto Max() const noexcept
		{
			TY max = std::numeric_limits<TY>::min();

			for (const auto& v : Vector)
				max = (max < v) ? max : v;

			return max;
		}

		[[nodiscard]] constexpr TY Product() const noexcept
		{
			TY Product = 1;
			for (auto element : Vector)
				Product *= element;
			return Product;
		}


		template<Scaler_t TY_2>
		constexpr THISTYPE operator + (const Vect<SIZE, TY_2>& rhs) const noexcept
		{
			THISTYPE temp = *this;
			for (auto I = 0; I < SIZE; ++I)
				temp[I] += rhs[I];

			return temp;
		}

		template<typename TY_2>
		constexpr THISTYPE operator + (const TY_2 rhs) const noexcept
		{
			THISTYPE temp = *this;
			for (auto I = 0; I < SIZE; ++I)
				temp[I] += rhs;

			return temp;
		}

		template<typename TY_2>
		constexpr THISTYPE& operator += (const Vect<SIZE, TY_2>& in) noexcept
		{
			for (auto I = 0; I < SIZE; ++I)
				Vector[I] += in[I];

			return *this;
		}


		template<Scaler_t TY_2>
		constexpr THISTYPE operator - (const Vect<SIZE, TY_2>& in) const noexcept
		{
			THISTYPE temp = *this;
			for (auto I = 0; I < SIZE; ++I)
				temp[I] -= in[I];

			return temp;
		}


		template<typename TY_2>
		constexpr THISTYPE operator - (const TY_2 rhs) const noexcept
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
				Vector[I] -= in[I];

			return *this;
		}


		template<typename TY_2>
		constexpr THISTYPE& operator = (const Vect<SIZE, TY_2>& in) noexcept
		{
			for (auto I = 0; I < SIZE; ++I)
				Vector[I] = in[I];

			return *this;
		}

		constexpr THISTYPE operator / (Vector_t auto in) const noexcept
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

		constexpr THISTYPE operator * (Scaler_t auto rhs) const noexcept
		{
			THISTYPE temp = *this;
			for (auto I = 0; I < SIZE; ++I)
				temp[I] *= rhs;

			return temp;
		}

		template<typename TY_2>
		constexpr THISTYPE operator % (TY_2 in) const noexcept
		{
			THISTYPE temp = *this;
			for (auto I = 0; I < SIZE; ++I)
				temp[I] %= in;

			return temp;
		}


		template<typename TY_2>
		constexpr THISTYPE& operator /= (TY_2 in) noexcept
		{
			for (auto I = 0; I < SIZE; ++I)
				Vector[I] /= in;

			return *this;
		}


		template<typename TY_2>
		constexpr THISTYPE operator /= (const Vect<SIZE, TY_2> in) noexcept
		{
			THISTYPE Out;
			for (auto I = 0; I < SIZE; ++I)
				Out[I] = Vector[I] / in[I];

			return Out;
		}


		template<typename TY_2>
		constexpr bool operator == (const Vect<SIZE, TY_2>& in) const noexcept
		{
			bool res = true;
			for (auto I = 0; I < SIZE; ++I)
				res = res & (Vector[I] == in[I]);

			return res;
		}


		template<typename TY_2>
		constexpr THISTYPE& operator = (const std::initializer_list<TY_2>& il) noexcept
		{
			size_t itr = 0;
			for (auto n : il)
			{
				Vector[itr++] = n;
				if (itr > SIZE)
					break;
			}
			return *this;
		}


		constexpr THISTYPE operator << (const auto sl) const noexcept
		{
			THISTYPE v = *this;

			for (auto& n : v)
				n = v << sl;

			return v;
		}

		constexpr THISTYPE operator >> (const auto sr) const noexcept
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

		constexpr THISTYPE operator >> (const THISTYPE sr) const noexcept
		{
			THISTYPE v = *this;

			size_t i = 0;
			for (auto& n : v)
				v[i] = v[i] >> sr[i];

			return v;
		}

		template<size_t ... TY_ARGS>
		constexpr auto Swizzle()
		{
			static_assert(sizeof ... (TY_ARGS) == size(), "Incorrect number of arguments");
			return THISTYPE{ at(TY_ARGS) ... };
		}


		constexpr static THISTYPE Zero() noexcept
		{
			THISTYPE zero;

			for (auto& element : zero.Vector)
				element = static_cast<TY>(0);

			return zero;
		}


		constexpr static const size_t size() noexcept
		{
			return SIZE;
		}

		template<size_t beginIdx, size_t endIdx>
		constexpr auto Slice() const noexcept
		{
			static_assert(beginIdx < endIdx, "Invalid Arguments");
			static_assert(beginIdx < size(), "Invalid Arguments");
			static_assert(endIdx < size(), "Invalid Arguments");
			constexpr size_t count = endIdx - beginIdx;

			Vect<count, TY> out;

			for (size_t i = beginIdx; i < endIdx; i++)
				out[i] = Vector[i + beginIdx];

			return out;
		}


		operator TY* () noexcept { return Vector; }


		operator simde__m128 ()  noexcept
		{
			auto temp = simde_mm_loadu_ps(Vector);
			return simde_mm_loadr_ps((float*)&temp);
		}

		TY Vector[SIZE];
	};

	/************************************************************************************************/


	template<typename TY_S, unsigned int ELEMENT_COUNT = 1>
	[[nodiscard]] constexpr Vect<ELEMENT_COUNT, TY_S> operator* (const Vect<ELEMENT_COUNT, TY_S> lhs, const Vect<ELEMENT_COUNT, TY_S> rhs)// vector multiply
	{
		Vect<ELEMENT_COUNT, TY_S> V_out;

		for (std::size_t i = 0; i < rhs.size(); ++i)
			V_out[i] = lhs[i] * rhs[i];

		return V_out;
	}


	/************************************************************************************************/


	FLEXKITAPI template<typename TY_S, typename TY_Vs, size_t ELEMENT_COUNT = 1>
		[[nodiscard]] constexpr Vect<ELEMENT_COUNT, TY_Vs> operator * (TY_S scaler, const Vect<ELEMENT_COUNT, TY_Vs>& v)// scaler multiply
	{
		auto V_out = v;

		for (auto& Vs : V_out)
			Vs *= scaler;

		return V_out;
	}


	/************************************************************************************************/


	FLEXKITAPI template<typename TY_S, typename TY_Vs, size_t ELEMENT_COUNT = 1>
		[[nodiscard]] constexpr Vect<ELEMENT_COUNT, TY_Vs> operator * (const Vect<ELEMENT_COUNT, TY_Vs>& v, TY_S scaler)// scaler multiply
	{
		auto V_out = v;

		for (auto& Vs : V_out)
			Vs = (TY_Vs)(Vs * scaler);

		return V_out;
	}

	/************************************************************************************************/


	FLEXKITAPI template<typename TY_S, typename TY_Vs, size_t ELEMENT_COUNT = 1>
		[[nodiscard]] constexpr Vect<ELEMENT_COUNT, TY_Vs> operator / (const Vect<ELEMENT_COUNT, TY_Vs>& v, TY_S scaler)// scaler multiply
	{
		auto V_out = v;

		for (auto& Vs : V_out)
			Vs /= scaler;

		return V_out;
	}


	/************************************************************************************************/


	FLEXKITAPI template<typename TY_Vs, size_t ELEMENT_COUNT = 1>
		[[nodiscard]] constexpr Vect<ELEMENT_COUNT, TY_Vs> operator / (const Vect<ELEMENT_COUNT, TY_Vs>& v1, const Vect<ELEMENT_COUNT, TY_Vs>& v2)// scaler multiply
	{
		auto V_out = v1;

		for (size_t I = 0; I < ELEMENT_COUNT; ++I)
			V_out[I] = v1[I] / v2[I];

		return V_out;
	}

	FLEXKITAPI template<typename TY_Vs, size_t ELEMENT_COUNT = 1>
		[[nodiscard]] constexpr Vect<ELEMENT_COUNT, TY_Vs> operator - (const Vect<ELEMENT_COUNT, TY_Vs>& v)// 
	{
		TY_Vs V_out;

		for (size_t I = 0; I < ELEMENT_COUNT; ++I)
			V_out[I] = -v[I];

		return V_out;
	}


	/************************************************************************************************/


	typedef Vect<2> Vect2;
	typedef Vect<3> Vect3;
	typedef Vect<4> Vect4;

	typedef Vect<3, double> double3;
	typedef Vect<4, double> double4;

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



	template<typename TY_> float2 operator + (const float2& LHS, const Vect<2, TY_>& RHS) noexcept { return{ (float)(LHS.x + RHS[0]), (float)(LHS.y + RHS[1]) }; }
	template<typename TY_> float2 operator - (const float2& LHS, const Vect<2, TY_>& RHS) noexcept { return{ (float)(LHS.x - RHS[0]), (float)(LHS.y - RHS[1]) }; }
	template<typename TY_> float2 operator * (const float2& LHS, const Vect<2, TY_>& RHS) noexcept { return{ (float)(LHS.x * RHS[0]), (float)(LHS.y * RHS[1]) }; }
	template<typename TY_> float2 operator / (const float2& LHS, const Vect<2, TY_>& RHS) noexcept { return{ (float)(LHS.x / RHS[0]), (float)(LHS.y / RHS[1]) }; }

	template<typename TY_> Vect<2, TY_> operator * (const Vect<2, TY_> LHS, const float2 RHS) noexcept { return{ (TY_)(LHS[0] * RHS[0]), (TY_)(LHS[1] * RHS[1]) }; }


	inline float2 operator * (const float   lhs, const float2 rhs) noexcept { return float2{ lhs, lhs } * rhs; }


	/************************************************************************************************/

	inline float DotProduct2(const float* lhs, float* rhs) noexcept
	{
#if USING(FASTMATH)
		// Windows
		simde__m128 l = simde_mm_loadr_ps(lhs);
		simde__m128 r = simde_mm_loadr_ps(rhs);
		simde__m128 res = simde_mm_dp_ps(l, r, 0x06);
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

	FLEXKITAPI inline float DotProduct3(const simde__m128& lhs, const simde__m128& rhs) noexcept
	{
#if USING(FASTMATH)
		simde__m128 res = simde_mm_dp_ps(lhs, rhs, 0x77);
		return simde_mm_cvtss_f32(res);

#else
		return (lhs.m128_f32[0] * rhs.m128_f32[0]) + (lhs.m128_f32[1] * rhs.m128_f32[1]) + (lhs.m128_f32[2] * rhs.m128_f32[2]);
#endif
	}

	FLEXKITAPI inline float DotProduct4(const simde__m128& lhs, const simde__m128& rhs) noexcept
	{
#if USING(FASTMATH)
		simde__m128 res = simde_mm_dp_ps(lhs, rhs, 0xFF);
		return GetFirst(res);
#else
		return (lhs.m128_f32[0] * rhs.m128_f32[0]) + (lhs.m128_f32[1] * rhs.m128_f32[1]) + (lhs.m128_f32[2] * rhs.m128_f32[2]);
#endif
	}

	FLEXKITAPI inline simde__m128 CrossProductSlow(const simde__m128 lhs, const simde__m128 rhs) noexcept
	{
		simde__m128 out = simde_mm_set1_ps(0);
		SetElement(out, (GetElement(lhs, 1) * GetElement(rhs, 2)) - (GetElement(lhs, 2) * GetElement(rhs, 1)), 0);
		SetElement(out, (GetElement(lhs, 2) * GetElement(rhs, 0)) - (GetElement(lhs, 0) * GetElement(rhs, 2)), 1);
		SetElement(out, (GetElement(lhs, 0) * GetElement(rhs, 1)) - (GetElement(lhs, 1) * GetElement(rhs, 0)), 2);
		return out;
	}


	FLEXKITAPI inline simde__m128 CrossProduct(const simde__m128 a, const simde__m128 b) noexcept
	{
#if USING(FASTMATH)
		simde__m128 temp1 = simde_mm_mul_ps(simde_mm_shuffle_ps(a, a, 0x01 | 0x02 << 2 | 0x00 << 4 | 0x00 << 6), simde_mm_shuffle_ps(b, b, 0x02 | 0x00 << 2 | 0x01 << 4 | 0x00 << 6));
		simde__m128 temp2 = simde_mm_mul_ps(simde_mm_shuffle_ps(a, a, 0x02 | 0x00 << 2 | 0x01 << 4 | 0x00 << 6), simde_mm_shuffle_ps(b, b, 0x01 | 0x02 << 2 | 0x00 << 4 | 0x00 << 6));
		simde__m128 res = simde_mm_sub_ps(temp1, temp2);

		return res;
#else
		return CrossProductSlow(a, b);
#endif
	}


	union Quaternion;
	FLEXKITAPI Quaternion GrassManProduct(const Quaternion& lhs, const Quaternion& rhs) noexcept;

	/************************************************************************************************/

	FLEXKITAPI union alignas(16)  float3
	{
	public:
		constexpr float3() noexcept {}

		constexpr float3(float val)						noexcept
		{
			if (std::is_constant_evaluated())
			{
				x = val;
				y = val;
				z = val;
			}
			else
				pfloats = simde_mm_set_ps1(val);
		}

		constexpr float3(float X, float Y, float Z)		noexcept
		{
			if (std::is_constant_evaluated())
			{
				x = X;
				y = Y;
				z = Z;
			}
			else
				pfloats = simde_mm_set_ps(0.0f, Z, Y, X);
		}

		explicit constexpr float3(float2 in, float Z = 0)	noexcept
		{
			if (std::is_constant_evaluated())
			{
				x = in.x;
				y = in.y;
				z = Z;
			}
			else
				pfloats = simde_mm_setr_ps(in.x, in.y, Z, 0.0f);
		}

		constexpr float3(const float3& a)				noexcept
		{
			if (std::is_constant_evaluated())
			{
				x = a.x;
				y = a.y;
				z = a.z;
			}
			else
				pfloats = simde_mm_load_ps(a);
		}

		explicit float3(const simde__m128& in)				noexcept { pfloats = in; }

		float2 xy() const noexcept { return { x, y }; }
		float2 yz() const noexcept { return { y, z }; }
		float2 yx() const noexcept { return { y, x }; }
		float2 xz() const noexcept { return { x, z }; }
		float2 zy() const noexcept { return { z, y }; }
		float2 zx() const noexcept { return { z, x }; }

		float3 zxy() const noexcept { return { z, x, y }; }
		float3 yzx() const noexcept { return { y, z, x }; }
		float3 zyx() const noexcept { return { z, y, x }; }


		float3& operator = (const float* f) noexcept
		{
			pfloats = simde_mm_load_ps(f);
			return *this;
		}


		float3& operator = (float f) noexcept
		{
			pfloats = simde_mm_set_ps1(f);
			return *this;
		}


		float3& operator = (const float3& F) noexcept
		{
			pfloats = F.pfloats;
			return *this;
		}


		constexpr float& operator[] (const size_t index)			noexcept
		{
			if (std::is_constant_evaluated())
			{
				switch (index)
				{
				case 0:
					return x;
				case 1:
					return y;
				case 2:
					return z;
				default:
#ifdef WIN32
					std::unreachable();
#endif			
					break;
				}
			}
			else
				return *GetElement_ptr(pfloats, index);
		}

		constexpr const float& operator[]  (const size_t index)	const	noexcept
		{
			if (std::is_constant_evaluated())
			{
				switch (index)
				{
				case 0:
					return x;
				case 1:
					return y;
				case 2:
					return z;
				default:
#ifdef WIN32
					std::unreachable();
#endif	
					break;
				}
			}
			else
				return GetElement(pfloats, index);
		}

		// Operator Overloads
		float3 operator - ()			noexcept { return float3{ simde_mm_mul_ps(pfloats, simde_mm_set_ps1(-1)) }; }
		float3 operator - ()	const	noexcept { return float3{ simde_mm_mul_ps(pfloats, simde_mm_set_ps1(-1)) }; }
		float3 operator + (const float rhs)		const noexcept { return float3{ simde_mm_add_ps(pfloats, simde_mm_set_ps1(rhs)) }; }
		float3 operator + (const float3 rhs)	const noexcept { return float3{ simde_mm_add_ps(pfloats, rhs) }; }


		float3& operator += (const float3 rhs) noexcept
		{
			pfloats = simde_mm_add_ps(pfloats, rhs);

			return *this;
		}

		float3& operator += (const float rhs) noexcept
		{
			pfloats = simde_mm_add_ps(pfloats, simde_mm_set1_ps(rhs));

			return *this;
		}

		float3 operator - (const float rhs) const noexcept
		{
			return float3{ simde_mm_sub_ps(pfloats, simde_mm_set1_ps(rhs)) };
		}


		float3& operator -= (const float3 rhs) noexcept
		{
			pfloats = simde_mm_sub_ps(pfloats, rhs.pfloats);

			return *this;
		}

		float3& operator -= (const float rhs) noexcept
		{
			pfloats = simde_mm_sub_ps(pfloats, simde_mm_set1_ps(rhs));

			return *this;
		}


		bool operator == (const float3 rhs) const noexcept
		{
			if (rhs.x == x)
				if (rhs.y == y)
					if (rhs.z == z)
						return true;
			return false;
		}


		float3 operator - (const float3 a) const noexcept
		{
			return float3{ simde_mm_sub_ps(pfloats, a) };
		}

		static bool Compare(const float3 lhs, const float3 rhs, float ep = 0.001f) noexcept
		{
			float3 temp = lhs - rhs;
			return (temp.x < ep) && (temp.y < ep) && (temp.z < ep);
		}

		float3 operator *	(const float3 a) const noexcept
		{
#if USING(FASTMATH)
			return float3{ simde_mm_mul_ps(a.pfloats, pfloats) };
#else
			return float3(x * a.x, y * a.y, z * a.z);
#endif
		}


		float3 operator *	(const float a) const noexcept
		{
#if USING(FASTMATH)
			return float3{ simde_mm_mul_ps(simde_mm_set1_ps(a), pfloats) };
#else
			return float3(x * a, y * a, z * a);
#endif

		}


		float3& operator *=	(const float3 a) noexcept
		{
			pfloats = simde_mm_mul_ps(pfloats, a);

			return *this;
		}


		float3& operator *=	(float a) noexcept
		{
			pfloats = simde_mm_mul_ps(pfloats, simde_mm_set1_ps(a));

			return *this;
		}


		float3 operator / (const float a) const noexcept
		{
			return float3{ simde_mm_div_ps(pfloats, simde_mm_set1_ps(a)) };
		}


		float3 operator / (const float3 a) const noexcept
		{
			return float3{ simde_mm_div_ps(pfloats, a) };
		}


		float3& operator /= (const float a) noexcept
		{
			pfloats = simde_mm_div_ps(pfloats, simde_mm_set1_ps(a));
			return *this;
		}


		float3& operator /= (const float3 a) noexcept
		{
			pfloats = simde_mm_div_ps(pfloats, a);
			return *this;
		}


		float3& Scale(float S) noexcept
		{
			pfloats = simde_mm_mul_ps(pfloats, simde_mm_set1_ps(S));
			return *this;
		}


		const float3 inverse() const noexcept
		{
			return float3{ simde_mm_mul_ps(pfloats, simde_mm_set1_ps(-1)) };
		}

		// Identities
		const float3 cross(const float3 rhs) const noexcept
		{
			return float3{ CrossProduct(pfloats, rhs.pfloats) };
		}


		const float3 distance(const float3 b) const noexcept
		{
			return (*this - b).magnitude();
		}


		float dot(const float3 b) const noexcept
		{
			simde__m128 res = simde_mm_dp_ps(pfloats, b.pfloats, 0b01110001);

			return simde_mm_cvtss_f32(res);
		}


		float3 abs() const noexcept
		{
			return { (float)fabs(x), (float)fabs(y), (float)fabs(z) };
		}


		float magnitude() const noexcept
		{
			auto sq = simde_mm_mul_ps(pfloats, pfloats);
			auto temp1 = simde_mm_movehdup_ps(sq);
			auto temp2 = simde_mm_add_ps(sq, temp1);
			auto temp3 = simde_mm_movehl_ps(sq, sq);
			auto temp4 = simde_mm_add_ps(temp2, temp3);

			return simde_mm_cvtss_f32(simde_mm_sqrt_ps(temp4));
		}


		inline float magnitudeSq() const noexcept
		{
			auto sq = simde_mm_mul_ps(pfloats, pfloats);
			auto temp1 = simde_mm_movehdup_ps(sq);
			auto temp2 = simde_mm_add_ps(sq, temp1);
			auto temp3 = simde_mm_movehl_ps(sq, sq);
			auto temp4 = simde_mm_add_ps(temp2, temp3);

			return simde_mm_cvtss_f32(temp4);
		}

		bool isNaN() const noexcept
		{
			return (std::isnan(x) || std::isnan(y) || std::isnan(z));
		}

		void normalize() noexcept
		{
			pfloats = normal();
		}

		float Max() const noexcept
		{
			auto temp1 = simde_mm_movehdup_ps(pfloats);
			auto temp2 = simde_mm_max_ps(pfloats, temp1);
			auto temp3 = simde_mm_movehl_ps(pfloats, pfloats);
			auto temp4 = simde_mm_max_ps(temp2, temp3);

			return simde_mm_cvtss_f32(temp4);
		}

		float Min() const noexcept
		{
			auto temp1 = simde_mm_movehdup_ps(pfloats);
			auto temp2 = simde_mm_min_ps(pfloats, temp1);
			auto temp3 = simde_mm_movehl_ps(pfloats, pfloats);
			auto temp4 = simde_mm_min_ps(temp2, temp3);

			return simde_mm_cvtss_f32(temp4);
		}

		float3 normal() const noexcept
		{
			auto sq = simde_mm_mul_ps(pfloats, pfloats);
			auto temp1 = simde_mm_movehdup_ps(sq);
			auto temp2 = simde_mm_add_ps(sq, temp1);
			auto temp3 = simde_mm_movehl_ps(sq, sq);
			auto temp4 = simde_mm_add_ps(temp2, temp3);
			auto m = simde_mm_shuffle_ps(temp4, temp4, _MM_SHUFFLE(0, 0, 0, 0));

			return float3{ simde_mm_div_ps(pfloats, simde_mm_sqrt_ps(m)) };
		}

		float* begin()	noexcept { return &x; }
		float* end()	noexcept { return &PAD; }

		const float* begin()	const noexcept { return &x; }
		const float* end()		const noexcept { return &PAD; }

		static float3			Zero() { return float3{ 0 }; }
		static constexpr size_t size() { return 3u; }

		operator simde__m128 () const { return pfloats; }

		inline float* toFloat3_ptr()		noexcept { return reinterpret_cast<float*>(&pfloats); }
		operator float* ()			noexcept { return toFloat3_ptr(); }
		operator const	float* () const		noexcept { return reinterpret_cast<const float*>(&pfloats); }

		operator Vect3 () const { return { x, y, z }; }
		operator float2() const { return { x, y }; }

		struct
		{
			float x, y, z, PAD;
		};

		simde__m128	pfloats;

		static float3 Load(float* a) noexcept
		{
			auto temp = simde_mm_loadu_ps(a);
			return float3(simde_mm_loadr_ps((float*)&temp));
		}

		constexpr static size_t Size() { return 3; }

		void Serialize(auto& ar)
		{
			ar& x;
			ar& y;
			ar& z;
		}

	private:
		static float3 SetVector(float in)
		{
			return float3{ simde_mm_set1_ps(in) };
		}

	};


	FLEXKITAPI const float3 BLACK = float3(0.0f, 0.0f, 0.0f);
	FLEXKITAPI const float3 WHITE = float3(1.0f, 1.0f, 1.0f);
	FLEXKITAPI const float3 RED = float3(1.0f, 0.0f, 0.0f);
	FLEXKITAPI const float3 BLUE = float3(0.0f, 0.0f, 1.0f);
	FLEXKITAPI const float3 GREEN = float3(0.0f, 1.0f, 0.0f);
	FLEXKITAPI const float3 PURPLE = float3(1.0f, 0.0f, 1.0f);


	FLEXKITAPI template<typename TY>
		TY clamp(const TY min, const TY v, const TY max) noexcept
	{
		return Min(Max(min, v), max);
	}


	FLEXKITAPI inline float saturate(float x) noexcept
	{
		return clamp(0.0f, x, 1.0f);
	}


	FLEXKITAPI inline float3 saturate(float3 v) noexcept
	{
		float3 out = v;
		v.x = Min(Max(v.x, 0.0f), 1.0f);
		v.y = Min(Max(v.y, 0.0f), 1.0f);
		v.z = Min(Max(v.z, 0.0f), 1.0f);

		return out;
	}


	/************************************************************************************************/


	inline float3 TripleProduct(const float3 A, const float3 B, const float3 C) noexcept
	{
		return (B - A).cross(C - A);
	}



	/************************************************************************************************/


	inline float3 operator* (float s, float3 V)
	{
#if USING(FASTMATH)
		return float3{ simde_mm_mul_ps(V, simde_mm_set1_ps(s)) };
#else
		return V * s;
#endif
	}


	inline float3 RotateVectorAxisAngle(float3 N, float a, float3 V) { return V * cos(a) + (V.dot(N) * N * (1 - cos(a)) + (N.cross(V) * sin(a))); }

	union alignas(16) float4
	{
	public:
		constexpr float4() {
			if consteval {
				array[0] = 0.0f;
				array[1] = 0.0f;
				array[2] = 0.0f;
				array[3] = 0.0f;
			}
		}

		constexpr inline float4(float r)
		{
			if (std::is_constant_evaluated())
			{
				x = r;
				y = r;
				z = r;
				w = r;
			}
			else
				pFloats = simde_mm_set1_ps(r);
		}

		constexpr inline float4(float X, float Y, float Z, float W)
		{
			if consteval
			{
				array[0] = X;
				array[1] = Y;
				array[2] = Z;
				array[3] = W;
			}
			else {
				pFloats = simde_mm_set_ps(W, Z, Y, X);
			}
		}

		constexpr inline float4(const float3& V, const float W = 0) noexcept
		{
			if (std::is_constant_evaluated())
			{
				x = V.x;
				y = V.y;
				z = V.z;
				w = W;
			}
			else
			{
				pFloats = simde_mm_load_ps(V);
				w = W;
			}
		}

		constexpr inline float4(const Vect3& V, const float W = 0) noexcept
		{
			if (std::is_constant_evaluated())
			{
				x = V[0];
				y = V[1];
				z = V[2];
				w = W;
			}
			else
			{
				pFloats = simde_mm_load_ps(V.Vector);
				w = W;
			}
		}

		inline float4(simde__m128 in) noexcept : pFloats{ in } {}

		constexpr inline float4(const float2 V1, const float2 V2) noexcept
		{
			x = V1[0];
			y = V1[1];
			z = V2[0];
			w = V2[1];
		}


		inline operator float* () { return (float*)&pFloats; }
		inline operator const float* const	()	const { return (float*)&pFloats; }

		constexpr inline float& operator[]	(const size_t index)
		{
			if (std::is_constant_evaluated())
			{
				switch (index)
				{
				case 0:
					return x;
				case 1:
					return y;
				case 2:
					return z;
				case 3:
					return w;
				default:
					throw;
				}
			}
			else
				return GetElement_ref(pFloats, index);
		}

		constexpr inline const float& operator[]	(const size_t index)	const
		{
			if (std::is_constant_evaluated())
			{
				switch (index)
				{
				case 0:
					return x;
				case 1:
					return y;
				case 2:
					return z;
				case 3:
					return w;
				default:
					throw;
				}
			}
			else
				return GetElement(pFloats, index);
		}

		inline operator simde__m128	 ()						const { return pFloats; }

		inline float4 operator+ (const float4 rhs) const noexcept
		{
#if USING(FASTMATH)
			return float4{ simde_mm_add_ps(pFloats, rhs.pFloats) };
#else
			return float4(x + rhs.x,
				y + rhs.y,
				z + rhs.z,
				w + rhs.w);
#endif
		}

		inline float4 operator+ (const float rhs) const noexcept
		{
#if USING(FASTMATH)
			return float4{ simde_mm_add_ps(pFloats, simde_mm_set1_ps(rhs)) };
#else
			return float4(x + rhs,
				y + rhs,
				z + rhs,
				w + rhs);
#endif
		}

		inline float4& operator+= (const float4 rhs) noexcept
		{
#if USING(FASTMATH)
			(*this) = (*this) + rhs;
			return (*this);
#else
			return float4(x - rhs.x,
				y - rhs.y,
				z - rhs.z,
				w - rhs.w);
#endif
		}

		inline float4 operator- (const float4 rhs) const noexcept
		{
#if USING(FASTMATH)
			return float4{ simde_mm_sub_ps(pFloats, rhs) };
#else
			return float4(x - rhs.x,
				y - rhs.y,
				z - rhs.z,
				w - rhs.w);
#endif
		}

		inline float4& operator-= (const float4 rhs) noexcept
		{
#if USING(FASTMATH)
			(*this) = (*this) - rhs;
			return (*this);
#else
			return float4(x - rhs.x,
				y - rhs.y,
				z - rhs.z,
				w - rhs.w);
#endif
		}

		inline float4 operator- (const float rhs) const noexcept
		{
#if USING(FASTMATH)
			return float4{ simde_mm_sub_ps(pFloats, simde_mm_set1_ps(rhs)) };
#else
			return float4(x - rhs,
				y - rhs,
				z - rhs,
				w - rhs);
#endif
		}

		inline float4 operator* (const float4 a) const noexcept
		{
			return float4{ simde_mm_mul_ps(pFloats, a) };
		}

		inline float4 operator* (const float rhs) const noexcept
		{
#if USING(FASTMATH)
			return float4{ simde_mm_mul_ps(pFloats, simde_mm_set1_ps(rhs)) };
#else
			return float4(x * rhs,
				y * rhs,
				z * rhs,
				w * rhs);
#endif
		}

		inline float4 operator / (const float4 rhs) const noexcept
		{
#if USING(FASTMATH)
			return float4{ simde_mm_div_ps(pFloats, rhs) };
#else
			return float4(x / rhs.x,
				y / rhs.y,
				z / rhs.z,
				w / rhs.w);
#endif
		}

		inline float4& operator /= (const float rhs)
		{
#if USING(FASTMATH)
			pFloats = simde_mm_div_ps(pFloats, simde_mm_set1_ps(rhs));

			return *this;
#else
			return float4(x / rhs.x,
				y / rhs.y,
				z / rhs.z,
				w / rhs.w);
#endif
		}

		inline float4& operator*= (const float4 a) noexcept
		{
			pFloats = (*this) * a;
			return *this;
		}

		inline float4 operator / (const float rhs) const noexcept
		{
#if USING(FASTMATH)
			return float4{ simde_mm_div_ps(pFloats, simde_mm_set1_ps(rhs)) };
#else
			return float4(x / rhs,
				y / rhs,
				z / rhs,
				w / rhs);
#endif
		}

		inline float4& operator /= (const float4 a) noexcept
		{
			pFloats = (*this) / a;
			return *this;
		}

		inline float4 operator % (const float4 rhs) const noexcept
		{
			return float4(std::fmod(x, rhs.x),
				std::fmod(y, rhs.y),
				std::fmod(z, rhs.z),
				std::fmod(w, rhs.w));
		}

		float Max() const noexcept
		{
			return FlexKit::Max(FlexKit::Max(x, y), FlexKit::Max(z, w));
		}

		float Min() const noexcept
		{
			return FlexKit::Min(FlexKit::Min(x, y), FlexKit::Max(z, w));
		}

		struct
		{
			float x, y, z, w;
		};

		struct
		{
			float r, g, b, a;
		};

		float array[4];

		float3 xyz() const
		{
			return { x, y, z };
		}


		static float4 Zero() noexcept
		{
			return simde_mm_set1_ps(0);
		}

		operator Vect4 ()		noexcept { return { x, y, z, w }; };
		operator Vect4 () const	noexcept { return { x, y, z, w }; };
		operator float3() const	noexcept { return float3{ pFloats }; };

		constexpr static size_t Size() { return 4; }
		constexpr static size_t size() { return 4; }

		void Serialize(auto& ar)
		{
			ar& x;
			ar& y;
			ar& z;
			ar& w;
		}

		simde__m128 pFloats;
	};

	inline float  F4Dot(float4 rhs, float4 lhs) { return DotProduct4(lhs, rhs); }
	inline float4 F4MUL(const float4 lhs, const float4 rhs) { return float4{ simde_mm_mul_ps(lhs, rhs) }; }


	/************************************************************************************************/

	FLEXKITAPI union alignas(16) Quaternion
	{
	public:
		inline Quaternion() {}
		inline Quaternion(simde__m128 in) { floats = in; }


		inline explicit Quaternion(const float3& vector, float scaler)
		{
#if USING(FASTMATH)
			floats = simde_mm_set_ps(scaler, vector[2], vector[1], vector[0]);
#else
			x = vector[0];
			y = vector[1];
			z = vector[2];
			w = scaler;
#endif
		}


		inline Quaternion(float X, float Y, float Z, float W)
		{
#if USING(FASTMATH)
			floats = simde_mm_set_ps(W, Z, Y, X);
#else
			x = X;
			y = Y;
			z = Z;
			w = W;
#endif
		}


		inline explicit Quaternion(float* in)
		{
			simde_mm_store_ps(in, floats);
		}


		inline explicit Quaternion(const Quaternion& in) :
			floats(in.floats) {}


		inline explicit Quaternion(float dX, float dY, float dZ) // Degrees to Quat
		{
			FlexKit::Quaternion X, Y, Z;
			X.Zero();
			Y.Zero();
			Z.Zero();;

			float dX1_2 = DegreetoRad(dX / 2);
			float dY1_2 = DegreetoRad(dY / 2);
			float dZ1_2 = DegreetoRad(dZ / 2);

			X.x = std::sin(dX1_2);
			X.w = std::cos(dX1_2);
			Y.y = std::sin(dY1_2);
			Y.w = std::cos(dY1_2);
			Z.z = std::sin(dZ1_2);
			Z.w = std::cos(dZ1_2);

			(*this) = X * Y * Z;
		}


		inline Quaternion& operator *= (const Quaternion& rhs) noexcept
		{
			(*this) = GrassManProduct(*this, rhs);
			return (*this);
		}


		inline Quaternion& operator = (const Quaternion& rhs) noexcept
		{
			floats = rhs.floats;
			return (*this);
		}


		inline Quaternion& operator = (const  simde__m128& rhs) noexcept
		{
			floats = rhs;
			return (*this);
		}


		inline Quaternion operator * (const Quaternion& q) const noexcept
		{
			return GrassManProduct(q, *this);
		}


		inline float& operator [] (const size_t index) { return GetElement_ref(floats, index); }
		inline float operator  [] (const size_t index) const { return GetElement(floats, index); }

		inline float& operator [] (const int index) { return GetElement_ref(floats, index); }
		inline float operator  [] (const int index) const { return GetElement(floats, index); }


		inline operator float* () { return (float*)&floats; }
		inline operator const float* () const { return (float*)&floats; }
		inline operator simde__m128 () { return floats; }
		inline operator const simde__m128() const { return floats; }


		template< typename Ty_2 >
		inline Quaternion& operator = (const float4& rhs)
		{
			x = rhs.x;
			y = rhs.y;
			z = rhs.z;
			w = rhs.w;

			return (*this);
		}


		inline Quaternion Conjugate() const noexcept
		{
			Quaternion Conjugate;
#if USING(FASTMATH)
			Conjugate = simde_mm_mul_ps(simde_mm_set_ps(1, -1, -1, -1), floats);
#else
			Conjugate.x = -x;
			Conjugate.y = -y;
			Conjugate.z = -z;
			Conjugate.w = w;
#endif
			return Conjugate;
		}
		inline Quaternion Inverse() const { return  Conjugate(); }


		inline float dot(Quaternion rhs) const noexcept
		{
			return V().dot(rhs.V()) + w * rhs.w;
		}


		inline Quaternion operator / (float denom) { return { x / denom , y / denom , z / denom , w / denom }; }
		inline Quaternion operator + (Quaternion RHS) { return { x + RHS.x, y + RHS.y, z + RHS.z, w + RHS.w }; }


		inline float3 XYZ() const { return float3(x, y, z); }
		inline float3 V()   const { return float3(x, y, z); }


		inline float Magnitude() const noexcept
		{
#if USING( FASTMATH )
			simde__m128 q2 = simde_mm_mul_ps(floats, floats);
			q2 = simde_mm_hadd_ps(q2, q2);
			q2 = simde_mm_hadd_ps(q2, q2);
			return GetLast(q2);
#else
			return x * x + y * y + z * z + w * w;
#endif
		}


		inline Quaternion& normalize() noexcept
		{
			float mag2 = Magnitude();
			if (mag2 != 0 && (fabs(mag2 - 1.0f) > .00001f))
			{
#if USING(FASTMATH)

				simde__m128 rsq = simde_mm_rsqrt_ps(simde_mm_set1_ps(mag2));
				floats = simde_mm_mul_ps(rsq, floats);
#else
				float mag = sqrt(mag2);
				w = w / mag;
				x = x / mag;
				y = y / mag;
				z = z / mag;
#endif
			}
			return *this;
		}


		inline Quaternion normal() const noexcept
		{
			float mag2 = Magnitude();
			simde__m128 Res;
			if (mag2 != 0 && (fabs(mag2 - 1.0f) > .00001f))
			{
#if USING(FASTMATH)

				simde__m128 rsq = simde_mm_rsqrt_ps(simde_mm_set1_ps(mag2));
				Res = simde_mm_mul_ps(rsq, floats);
#else
				float mag = sqrt(mag2);
				w = w / mag;
				x = x / mag;
				y = y / mag;
				z = z / mag;
#endif
			}
			else
				Res = floats;

			return Res;
		}


		inline void Zero()
		{
#if USING(FASTMATH)
			floats = simde_mm_set1_ps(0);
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

		void Serialize(auto& ar)
		{
			ar& floats;
		}

		constexpr static size_t Size() { return 4; }

		struct
		{
			float x, y, z, w;
		};

		simde__m128	floats;
	};


	/************************************************************************************************/


	inline float3		operator * (const Quaternion P, const float3 V) noexcept
	{
		auto v = -1 * P.XYZ();
		auto vXV = v.cross(V);
		auto ret = float3(V + (vXV * (2 * P.w)) + (v.cross(vXV) * 2));

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


	FLEXKITAPI inline Quaternion Slerp(Quaternion P, Quaternion Q, float W)
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


	FLEXKITAPI inline float3	Vect3ToFloat3(const Vect3 R3)  noexcept { return { R3[0], R3[1], R3[2] }; }
	FLEXKITAPI inline Vect3	    Float3ToVect3(const float3 R3) noexcept { return { R3[0], R3[1], R3[2] }; }

	FLEXKITAPI inline float4	Vect4ToFloat4(const Vect4  R4) noexcept { return { R4[0], R4[1], R4[2], R4[3] }; }
	FLEXKITAPI inline Vect4	    Float4ToVect4(const float4 R4) noexcept { return { R4[0], R4[1], R4[2], R4[3] }; }


	/************************************************************************************************/


	FLEXKITAPI inline float Vect3FDot(const Vect3& lhs, const Vect3& rhs) noexcept
	{
		auto temp1 = simde_mm_set_ps(0, lhs.Vector[2], lhs.Vector[1], lhs.Vector[0]);
		auto temp2 = simde_mm_set_ps(0, rhs.Vector[2], rhs.Vector[1], rhs.Vector[0]);

		simde__m128 res = simde_mm_dp_ps(temp1, temp2, 0xFF);

		return GetFirst(res);
	}


	/************************************************************************************************/


	constexpr FLEXKITAPI inline float Vect4FDot(const Vect4 lhs, const Vect4 rhs)
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
			auto temp1 = simde_mm_loadu_ps(lhs.Vector); //_mm_set_ps(lhs.Vector[3], lhs.Vector[2], lhs.Vector[1], lhs.Vector[0]);
			auto temp2 = simde_mm_loadu_ps(rhs.Vector); //_mm_set_ps(rhs.Vector[3], rhs.Vector[2], rhs.Vector[1], rhs.Vector[0]);

			simde__m128 res = simde_mm_dp_ps(temp1, temp2, 0xFF);

			return GetFirst(res);
		}
	}


	/************************************************************************************************/

	template<typename TY, size_t size>
	union MatrixOptionalVectorData
	{
		static inline constexpr bool Enabled = false;
	};

	template<std::size_t Size>
	union MatrixOptionalVectorData<int, Size>
	{
		static inline constexpr bool Enabled = false;
	};

	template<size_t Size> requires (Size % 4 == 0 && Size % 8 != 0)
		union MatrixOptionalVectorData<float, Size>
	{
		static inline constexpr bool Enabled = true;


		const	simde__m128& V128At(size_t idx)		noexcept { return vectors[idx]; }
		simde__m128& V128At(size_t idx) const	noexcept { return vectors[idx]; }

		simde__m128 vectors[Size / 4];
	};

	template<size_t Size> requires (Size % 4 == 0 && Size % 8 == 0)
		union MatrixOptionalVectorData<float, Size>
	{
		static inline constexpr bool Enabled = true;

		simde__m128& V128At(size_t idx)		noexcept { return vector128[idx]; }
		const	simde__m128& V128At(size_t idx) const	noexcept { return vector128[idx]; }


		simde__m256& V256At(size_t idx)			noexcept { return vector256[idx]; }
		const 	simde__m256& V256At(size_t idx) const	noexcept { return vector256[idx]; }


		simde__m128 vector128[Size / 4];
		simde__m256 vector256[Size / 8];

#if defined(__x86_64__) || defined(_M_X64)
		__m128& V128AtX64(size_t idx)		noexcept { return vector128X64[idx]; }
		const	__m128& V128AtX64(size_t idx) const	noexcept { return vector128X64[idx]; }

		__m256& V256AtX64(size_t idx)		noexcept { return vector256X64[idx]; }
		const	__m256& V256AtX64(size_t idx) const	noexcept { return vector256X64[idx]; }

		__m128 vector128X64[Size / 4];
		__m256 vector256X64[Size / 8];
#endif
	};

#pragma warning(push)
#pragma warning(disable : 4324)
	// Row Major
	template<const int Width, const int Height, typename Ty = float>
	union Matrix
	{
	private:
		template<typename TY_tuple, int ... seq>
		constexpr void helper_S(const TY_tuple& tuple, const std::integer_sequence<int, seq...> x) noexcept
		{
			auto setScaler = [&](size_t idx, const auto s)
				{
					const size_t x = idx % Width;
					const size_t y = idx / Height;

					SetAt(x, y, s);
				};

			//(setScaler(seq, std::get<seq>(tuple)), ...);
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


		template<typename ... TY_args>
		constexpr static auto BuildTuple(const TY_args& ... args) noexcept
		{
			return std::tuple_cat(BuildTuple(args)...);
		}


	public:
		using THIS_TYPE = const Matrix<Width, Height, Ty>;
		using VectorView = MatrixOptionalVectorData<Ty, Width* Height>;

		constexpr Matrix() = default;
		constexpr Matrix(const THIS_TYPE& initial) = default;

		template<typename ... TY_ARGS>
		constexpr explicit Matrix(const TY_ARGS& ... args) noexcept
		{
			const std::tuple tuple = BuildTuple(args...);
			constexpr size_t valueCount = std::tuple_size_v<decltype(tuple)>;
			constexpr auto indexes = std::make_integer_sequence<int, valueCount>{};

			static_assert(valueCount <= Width * Height, "Input value count must be less than container size!");

			helper_S(tuple, indexes);

			auto setValue = [&](size_t idx, auto v)
				{
					const size_t x = idx % Width;
					const size_t y = idx / Height;

					SetAt(x, y, v);
				};

			for (size_t I = valueCount; I < Width * Height; I++)
				setValue(I, 0.0f);
		}

		template<Scaler_t ... TY_ARGS>
		constexpr explicit Matrix(TY_ARGS&& ... args) noexcept requires (sizeof ... (TY_ARGS) == Width * Height)
		{
			auto forwarded_args = std::forward_as_tuple(args...);
			helper_S(forwarded_args, std::make_integer_sequence<int, sizeof ... (TY_ARGS)>());
		}

		template<Vector_t ... TY_ARGS>
		constexpr explicit Matrix(TY_ARGS&& ... args) noexcept requires (sizeof ... (TY_ARGS) == Height)
		{
			auto forwarded_args = std::forward_as_tuple(args...);
			helper_V(forwarded_args, std::make_integer_sequence<int, sizeof ... (TY_ARGS)>());
		}

		constexpr Matrix operator * (const float rhs)
		{
			Matrix out = *this;

			for (auto& c : out.matrix)
				for (auto& e : c)
					e = e * rhs;

			return out;
		}

		template< const int RHS_Height>
		constexpr Matrix<Width, RHS_Height> operator * (const Matrix<Width, RHS_Height>& rhs) const noexcept
		{
			static_assert(Width == RHS_Height, "ROW AND RHS COLS DO NOT MATCH");
			Matrix<Width, RHS_Height> out;
			auto transposed = rhs.Transpose();

			for (size_t i = 0; i < Width; ++i)
			{
				const auto& v = Row(i);
				for (size_t i2 = 0; i2 < Height; ++i2)
				{
					const auto v2 = transposed[i2];
					out(i2, i) = v.Dot(v2);
				}
			}

			return out;
		}


		constexpr Matrix<4, 4> operator * (const Matrix<4, 4>& rhs) const noexcept
		{
			if (std::is_constant_evaluated())
			{
				Matrix out;
				const auto transposed = rhs.Transpose();

				for (size_t x = 0; x < Width; ++x)
				{
					const auto&& ri = GetRow(x);

					for (size_t y = 0; y < Height; ++y)
					{
						const auto cj = transposed.GetRow(y);
						out.SetAt(y, x, Vect4FDot(ri, cj));
					}
				}

				return out;
			}
			else
			{
				Matrix<4, 4, Ty> out;
				out[0] =
					Vect4(Row(0)[0]) * rhs.Row(0u) +
					Vect4(Row(0)[1]) * rhs.Row(1u) +
					Vect4(Row(0)[2]) * rhs.Row(2u) +
					Vect4(Row(0)[3]) * rhs.Row(3u);

				out[1] =
					Vect4(Row(1)[0]) * rhs.Row(0u) +
					Vect4(Row(1)[1]) * rhs.Row(1u) +
					Vect4(Row(1)[2]) * rhs.Row(2u) +
					Vect4(Row(1)[3]) * rhs.Row(3u);

				out[2] =
					Vect4(Row(2)[0]) * rhs.Row(0u) +
					Vect4(Row(2)[1]) * rhs.Row(1u) +
					Vect4(Row(2)[2]) * rhs.Row(2u) +
					Vect4(Row(2)[3]) * rhs.Row(3u);

				out[3] =
					(Row(3u)[0u]) * rhs.Row(0u) +
					(Row(3u)[1u]) * rhs.Row(1u) +
					(Row(3u)[2u]) * rhs.Row(2u) +
					(Row(3u)[3u]) * rhs.Row(3u);

				return out;
			}
		}


		constexpr Matrix<4, 4> operator + (const Matrix<4, 4>& rhs) const noexcept
		{
			if (std::is_constant_evaluated())
			{
				Matrix out;

				for (size_t x = 0; x < Width; ++x)
					for (size_t y = 0; y < Height; ++y)
						out.SetAt(x, y, At(x, y) + rhs(x, y));

				return out;
			}
			else
			{
				auto addRow = [&](size_t y) { return Row(y) + rhs.Row(y); };

				return Matrix{
					addRow(0),
					addRow(1),
					addRow(2),
					addRow(3),
				};
			}
		}

		constexpr Matrix<4, 4>& operator = (const Matrix<4, 4>& rhs) noexcept
		{
			memcpy(&matrix, &rhs.matrix, sizeof(Matrix<4, 4>));
			return *this;
		}

		constexpr 		Ty& At(const size_t x, const size_t y)			noexcept { return matrix[y][x]; }
		constexpr const	Ty& At(const size_t x, const size_t y) const	noexcept { return matrix[y][x]; }


		constexpr void		SetAt(const size_t x, const size_t y, const Ty val)	noexcept { matrix[y][x] = val; }

#ifdef __cpp_multidimensional_subscript
		constexpr		Ty& operator[] (const size_t x, const size_t y)			noexcept { return At(x, y); }
		constexpr const	Ty& operator[] (const size_t x, const size_t y) const	noexcept { return At(x, y); }
#endif

		constexpr 		Ty& operator() (const size_t x, const size_t y)			noexcept { return At(x, y); }
		constexpr const	Ty& operator() (const size_t x, const size_t y) const	noexcept { return At(x, y); }


		constexpr Vect<Width>& operator[] (const int y) { return rows[y]; }
		constexpr const Vect<Width>& operator[] (const int y) const { return rows[y]; }

		constexpr Vect<Width>& operator[] (const size_t i) { return rows[i]; }
		constexpr const Vect<Width>& operator[] (const size_t i) const { return rows[i]; }

		operator Ty* ()			noexcept { return (Ty*)matrix; }
		operator const  Ty* () const	noexcept { return (Ty*)matrix; }

		Ty* Data()			noexcept { return (Ty*)matrix; }
		const	Ty* Data() const	noexcept { return (Ty*)matrix; }

		void Serialize(auto& ar)
		{
			ar.SerializeBuffer((char*)this, sizeof(THIS_TYPE));
		}

		constexpr static Matrix Identity() noexcept requires (Width == Height)
		{
			Matrix m = Zero();

			for (size_t i = 0u; i < Width; i++)
				m(i, i) = static_cast<Ty>(1);

			return m;
		}


		consteval static Matrix Zero() noexcept
		{
			if (std::is_constant_evaluated())
			{
				Matrix m;

				for (size_t y = 0; y < Height; y++)
					for (size_t x = 0; x < Width; x++)
						m.SetAt(y, x, static_cast<Ty>(0));

				return m;
			}
			else
			{
				int zero_val = std::bit_cast<int, Ty>(static_cast<Ty>(0));
				Matrix m;
				memset(&m, zero_val, sizeof(m));

				return m;
			}
		}


		Vect<Width, Ty>& Row(const size_t columnIdx)	noexcept
		{
			return rows[columnIdx];
		}

		const Vect<Width, Ty>& Row(const size_t rowIdx) const noexcept
		{
			return rows[rowIdx];
		}

		constexpr const Vect<Width, Ty>		GetRow(const size_t rowIdx)	const noexcept
		{
			Vect<Width, Ty> out;
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


		constexpr Vect<Height, Ty> Column(const size_t columnIdx) noexcept
		{
			auto gatherVector = [&]<size_t ... Sequence>(std::integer_sequence<size_t, Sequence...> sequence) -> Vect<Height>
			{
				return { Sequence ... };
			};

			return gatherVector(std::make_index_sequence<Height>{});
		}


		constexpr Matrix<Height, Width, Ty> Transpose() const noexcept
		{
			Matrix<Height, Width, Ty> m_transposed;

			for (size_t y = 0; y < Height; ++y)
				for (size_t x = 0; x < Width; ++x)
					m_transposed(x, y) = At(y, x);

			return m_transposed;
		}


		constexpr static size_t ColumnCount() noexcept
		{
			return Width;
		}

		constexpr static size_t RowCount() noexcept
		{
			return Height;
		}

		Ty				matrix[Height][Width];	// Row Major
		Vect<Width, Ty>	rows[Height];
		NO_UNIQUE_ADDRESS	VectorView		vectorView;	// Optionally Exists, SIMD View
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

		operator float* ()			noexcept { return m; }
		operator const float* ()	const	noexcept { return m; }

		operator Internal_TY () noexcept { return ToCPU(); }
		operator Internal_TY () const noexcept { return ToCPU(); }

		Internal_TY m;
	};


	/************************************************************************************************/


	FLEXKITAPI inline float2 Mul(Matrix<2, 2, float>& LHS, float2& RHS)
	{
		float2 Out;

		for (size_t I = 0; I < 2; ++I)
			Out[I] = DotProduct2(LHS.matrix[I], RHS);

		return Out;
	}


	FLEXKITAPI inline float3 Mul(Matrix<3, 3, float>& LHS, float3& RHS)
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


	using float3x3 = Matrix<3, 3>;
	using float4x4 = Matrix<4, 4>;

	using float3x3_GPU = Matrix_GPU<float3x3>;
	using float4x4_GPU = Matrix_GPU<float4x4>;


	FLEXKITAPI inline float4x4 TranslationMatrix(float3 POS)
	{
		float4x4 Out = float4x4::Identity();

		for (const auto [i, v] : enumerate(POS))
			Out(3, i) = v;

		return Out;
	}


	FLEXKITAPI inline float4x4 ScaleMatrix(float3 POS)
	{
		float4x4 Out = float4x4::Identity();
		Out(0, 0) = POS.x;
		Out(1, 1) = POS.y;
		Out(2, 2) = POS.z;

		return Out;
	}


	FLEXKITAPI inline float3 ExtractTranslationVector(const float4x4& m)
	{
		float3 out;
		out.x = m[0][3];
		out.y = m[1][3];
		out.z = m[2][3];

		return out;
	}


	/************************************************************************************************/


	FLEXKITAPI inline float CalcMatrixTrace(float in[Matrix_Size])
	{
		float sum = 0.0f;
		for (size_t itr = 0; itr < Matrix_Size; itr++)
			for (size_t itr_2 = 0; itr_2 < 3; itr_2 += 4, itr += itr_2)
				sum += in[itr];
		return sum;
	}


	FLEXKITAPI inline float4x4 FastInverseNoScale(const float4x4 m)
	{
		float4x4 inverseRotation = m;
		inverseRotation[0][3] = 0.0f;
		inverseRotation[1][3] = 0.0f;
		inverseRotation[2][3] = 0.0f;
		inverseRotation[3] = Vect4{ -m[0][3], -m[1][3], -m[2][3], 1 };
		inverseRotation = inverseRotation.Transpose();

		return inverseRotation;
	}


	float4x4 Inverse(const float4x4& m);


	/************************************************************************************************/


	template<typename TY>
	inline TY Lerp(const TY A, const TY B, float t) { return A * (1.0f - t) + t * B; }

	template<typename TY>
	inline TY lerp(const TY A, const TY B, float t) { return A * (1.0f - t) + t * B; }

	namespace Conversion
	{
		template< typename Ty >
		static Vect3 toVector3(Ty& Convert) { return Vect3(Convert.x, Convert.y, Convert.z); }

		template< typename Ty > Ty Vect3To(const Vect3& Convert) { return Ty{ Convert[0], Convert[1], Convert[2] }; }
		template< typename Ty >	Ty Vect4To(const Vect4& Convert) { return Ty{ Convert[0], Convert[1], Convert[2], Convert[3] }; }

		template< typename Ty_Out, typename Ty_2>	Ty_Out Vect2To(const Ty_2& Convert) { return Ty_Out{ Convert[0], Convert[1] }; }

		template<typename TY>
		float2 Vect2TOfloat2(Vect<2, TY> Convert) { return float2{ (float)Convert[0], (float)Convert[1] }; }
	}


	/************************************************************************************************/


	inline float4 operator * (const float4x4& lhs, const float4 rhs)
	{// TODO: FAST PATH
		const Vect4 temp = rhs;
		const auto lhs_t = lhs.Transpose();

		return Conversion::Vect4To<float4>({
				lhs[0].Dot(temp),
				lhs[1].Dot(temp),
				lhs[2].Dot(temp),
				lhs[3].Dot(temp),
			});
	}


	inline float3 operator * (const float3x3& lhs, const float3 rhs)
	{// TODO: FAST PATH
		const Vect3   temp = rhs;

		return Conversion::Vect3To<float3>({
				lhs[0].Dot(temp),
				lhs[1].Dot(temp),
				lhs[2].Dot(temp),
			});
	}


	/************************************************************************************************/


	int			Exp(int32_t Number, uint32_t exp);
	Quaternion	MatrixToQuat(const Matrix<4, 4>&);
	void		NumberToString(int32_t n, std::string& _Dest);
	int			Testing();


	inline float3 GetTranslation(const float4x4&)
	{
		return float3{ 0, 0, 0 };
	}

	void printfloat2(const float2& in);
	void printfloat3(const float3& in);
	void printfloat4(const float4& in);
	void printQuaternion(const Quaternion in);


	inline float dot(const float3 lhs, const float3 rhs)
	{
		return DotProduct3(lhs, rhs);
	}


	inline float dot(const float4 lhs, const float4 rhs)
	{
		return DotProduct4(lhs, rhs);
	}


	/************************************************************************************************/


	FLEXKITAPI float4x4		CreatePerspectiveRH(const float FOV, const float minZ, const float maxZ, const float aspectRatio);


	/************************************************************************************************/


	float4x4	Quaternion2Matrix(const Quaternion q);
	Quaternion	Matrix2Quat(const float4x4& M);
	float4x4	Vector2RotationMatrix(const float3& Forward, const float3& Up, const float3& Right);
	Quaternion	Vector2Quaternion(const float3& Forward, const float3& Up, const float3& Right);


	/************************************************************************************************/


	inline Quaternion PointAt(float3 A, float3 B, const float3 UpV = { 0.0f, 1.0f, 0.0f })
	{
		float3 Dir = (B - A).normal();
		Dir = { Dir.z, Dir.y, -Dir.x };
		const float3 DirXUpV = Dir.cross(UpV);

		return Vector2Quaternion(Dir, DirXUpV.cross(Dir), DirXUpV);
	}


	/************************************************************************************************/


	inline std::ostream& operator << (std::ostream& stream, float2 xyz)
	{
		stream << "{ " << xyz.x << ", " << xyz.y << " }";
		return stream;
	}

	inline std::ostream& operator << (std::ostream& stream, float3 xyz)
	{
		stream << "{ " << xyz.x << ", " << xyz.y << ", " << xyz.z << " }";
		return stream;
	}

	inline std::ostream& operator << (std::ostream& stream, float4 xyz)
	{
		stream << "{ " << xyz.x << ", " << xyz.y << ", " << xyz.z << ", " << xyz.w << " }";
		return stream;
	}

	inline std::ostream& operator << (std::ostream& stream, Quaternion q)
	{
		stream << "{ i * " << q.x << ", j * " << q.y << ", k * " << q.z << ", " << q.w << " }";
		return stream;
	}


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
