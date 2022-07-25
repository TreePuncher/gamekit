#pragma once

#ifndef MATHUTTILS
#define MATHUTTILS
#pragma warning(disable : 4201)

// Includes
#include "buildsettings.h"

#include <concepts>
#include <initializer_list>
#include <limits>
#include <math.h>
#include <ostream>
#include <emmintrin.h>
#include <smmintrin.h>
#include <xmmintrin.h>
#include <stdint.h>
#include <string>
#include <type_traits>
#include <concepts>

namespace FlexKit
{   /************************************************************************************************/

    template<class T>
    concept isScaler = std::is_integral_v<T> || std::is_floating_point_v<T>;

    template<class T>
	concept isVector = !isScaler<T> && requires(T t)
	{
		t[0];
		t.Size();
	};

	template<isScaler TY_1, isScaler TY_2> [[nodiscard]] constexpr auto Floor	(const TY_1 x, const TY_2 y) noexcept { return (((TY_1)x > (TY_1)y) ? y : x);   }
    template<isScaler TY_1, isScaler TY_2> [[nodiscard]] constexpr auto Min		(const TY_1 x, const TY_2 y) noexcept { return (((TY_1)x > (TY_1)y) ? y : x);   }
    template<isScaler TY_1, isScaler TY_2> [[nodiscard]] constexpr auto Max		(const TY_1 x, const TY_2 y) noexcept { return (((TY_1)x > (TY_1)y) ? x : y);   }
	template<isScaler TY_1, isScaler TY_2> [[nodiscard]] constexpr auto Fastmod (const TY_1 x, const TY_2 y) noexcept { return (((TY_1)x < (TY_1)y) ? x : x%y); }

    template<isVector TY_V> [[nodiscard]] constexpr auto Floor(const TY_V x, const TY_V y) noexcept
    {
        TY_V out;
        for (size_t I = 0; I < TY_V::Size(); I++)
           out[I] = Floor(x[I], y[I]);

        return out;
    }

    template<isVector TY_V> [[nodiscard]] constexpr auto Min(const TY_V x, const TY_V y) noexcept
    {
        TY_V out;
        for (size_t I = 0; I < TY_V::Size(); I++)
            out[I] = Min(x[I], y[I]);

        return out;
    }

    template<isVector TY_V> [[nodiscard]] constexpr auto Max(const TY_V x, const TY_V y) noexcept
    {
        TY_V out;
        for (size_t I = 0; I < TY_V::Size(); I++)
            out[I] = Max(x[I], y[I]);

        return out;
    }

    template<isVector TY_V> [[nodiscard]] constexpr auto Fastmod(const TY_V x, const TY_V y) noexcept
    {
        TY_V out;
        for (size_t I = 0; I < TY_V::Size(); I++)
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


	FLEXKITAPI inline static const double	pi = 3.141592653589793;
	FLEXKITAPI inline static const unsigned int Matrix_Size = 16;

	FLEXKITAPI template<size_t N>	inline int Factorial	() { return N * Factorial<N-1>(); }
	FLEXKITAPI template<>			inline int Factorial<1>	() { return 1; }

	FLEXKITAPI template<typename Ty> constexpr Ty DegreetoRad( Ty deg ) noexcept { return (Ty)(deg * pi ) /180; }
	FLEXKITAPI template<typename Ty> constexpr Ty RadToDegree( Ty deg ) noexcept { return (Ty)(deg * 180) / pi; }


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


    FLEXKITAPI inline __m128 SSE_CopySign(__m128 sign, __m128 abs) noexcept
	{
		const uint32_t M1 = (1u << 31);
		const uint32_t M2 =~(1u << 31);

		const __m128 Sgn = _mm_and_ps(sign, _mm_castsi128_ps(_mm_set1_epi32(M1)));
		const __m128 Abs = _mm_and_ps(abs, _mm_castsi128_ps(_mm_set1_epi32(M2)));
		const __m128 res = _mm_or_ps(Sgn, Abs);

        return res;
	}


    /************************************************************************************************/


    FLEXKITAPI inline __m128 SSE_ABS(__m128 abs) noexcept
    {
        return SSE_CopySign(_mm_set_ps1(1), abs);
    }


    /************************************************************************************************/


    FLEXKITAPI template<typename TY>
	inline TY Saturate(TY A) { return Max( 0.0f, Min(1.0f, A)); }


    FLEXKITAPI template<typename TY>
	inline bool CompareFloats(TY A, TY B, TY E) { return fabs(A - B) <= E;	}


    FLEXKITAPI template<typename TY>
    bool VectorCompare(TY A, TY B, float E) noexcept { return (A - B).magnitudeSq() < E * E; }


    FLEXKITAPI inline const float* GetArray_ptr_const(const __m128& V)  noexcept { return reinterpret_cast<const float*>(&V); }
    FLEXKITAPI inline       float* GetArray_ptr(__m128& V)              noexcept { return reinterpret_cast<float*>(&V); }

    FLEXKITAPI inline const float& GetElement(const __m128& V, const size_t idx) noexcept { return GetArray_ptr_const(V)[idx]; }

    FLEXKITAPI inline float* GetElement_ptr(__m128& V, const size_t idx ) noexcept { return GetArray_ptr(V) + idx; }
    FLEXKITAPI inline float& GetElement_ref(__m128& V, const size_t idx)  noexcept { return GetArray_ptr(V)[idx]; }

    FLEXKITAPI inline void SetElement   (__m128& V, float X, const size_t idx) noexcept { GetArray_ptr(V)[idx] = X;	}

    FLEXKITAPI inline float GetFirst	(const __m128& V) noexcept { return GetElement(V, 0); } // Should Return the X Component
    FLEXKITAPI inline float GetLast	    (const __m128& V) noexcept { return GetElement(V, 2); } // SHould Return the W Component

    FLEXKITAPI inline void SetFirst	    (__m128& V, const float X) noexcept { return SetElement(V, X, 0); }
    FLEXKITAPI inline void SetLast		(__m128& V, const float W) noexcept { return SetElement(V, W, 3); }


	/************************************************************************************************/


    FLEXKITAPI union float2
	{
	public:
		float2() noexcept : x(0), y(0) {}


		float2( const float X, const float Y ) noexcept
		{
			x = X;
			y = Y;
		}

		explicit float2(const float in_f) noexcept { x = in_f; y = in_f; }

        template<typename TY>
        float2(const TY& vect) noexcept
        {
            x = vect[0];
            y = vect[1];
        }


		inline bool		operator == ( const float2& rhs ) const { return ( rhs.x == x && rhs.y == y ) ? true : false; }

		inline float&   operator[] (const size_t i) noexcept        { FK_ASSERT(i < 2); return i ? y : x; }
		inline float    operator[] (const size_t i) const noexcept  { FK_ASSERT(i < 2); return i ? y : x; }

		inline float2 operator + ( const float2& a ) const noexcept { return float2( this->x + a.x,	this->y + a.y );		}
		inline float2 operator + ( const float   a ) const noexcept { return float2( x + a, y + a );						}
		inline float2 operator - ( const float2& a ) const noexcept { return float2( x - a.x, y - a.y );					}
		inline float2 operator - ( const float   a ) const noexcept { return float2( this->x - a,	this->y - a );			}
		inline float2 operator * ( const float2& a ) const noexcept { return float2( this->x * a.x,	this->y * a.y );		}
		inline float2 operator * ( const float   a ) const noexcept { return float2( this->x * a,	this->y * a );			}
		inline float2 operator / ( const float2& a ) const noexcept { return float2( this->x / a.x,	this->y / a.y );		}
		inline float2 operator / ( const float   a ) const noexcept { return float2( this->x / a,	this->y / a );			}
		inline float2 operator % ( const float2& a ) const noexcept { return float2( std::fmod(x, a.x), std::fmod(y, a.y));	}

        inline float2& operator = (const float2& a) noexcept = default;// { x = a.x; y = a.y; return *this; }

		inline float2 operator *= (const float2& a) noexcept
		{ 
			*this = *this * a;
			return *this; 
		}


		inline float2&	operator -= (const float2& rhs) noexcept
		{
			*this = *this - rhs;
			return *this;
		}


		inline float2&	operator += (const float2& rhs) noexcept
		{
			*this = *this + rhs;
			return *this;
		}


		inline bool operator > (const float2& rhs) const noexcept
		{
			return (x > rhs.x) && (y > rhs.y);
		}


		inline bool operator < (const float2& rhs) const noexcept
		{
			auto temp = !(*this > rhs);
			return temp;
		}


		inline void Add( const float2& lhs, const float2& rhs ) noexcept
		{
			x = lhs.x + rhs.x;
			y = lhs.y + rhs.y;
		}

        inline float2 floor() const noexcept
        {
            return { ::floorf(x), ::floorf(y) };
        }

        inline float2 ceil() const noexcept
        {
            return { ::ceilf(x), ::ceilf(y) };
        }

		operator float* () noexcept { return XY; }


		float Product() const noexcept { return x * y; }
		float Sum()	    const noexcept { return x + y; }

		float Magnitude() const noexcept
		{ 
			const auto V_2 = (*this * *this);
			return  sqrt(V_2.Sum());
		}

        float magnitudeSq() const noexcept
        {
            return (*this * *this).Sum();
        }

        struct
        {
            float x, y;
        };

		constexpr static size_t Size() { return 2; }

		float XY[2];

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
    
    FLEXKITAPI template<unsigned int SIZE, typename TY = float>
	class Vect
	{
		typedef Vect<SIZE, TY> THISTYPE;

        template<typename TY_tuple, int ... ints>
        void helper(const TY_tuple& tuple, std::integer_sequence<int, ints...>) noexcept
        {
            ((Vector[ints] = static_cast<TY>(std::get<ints>(tuple))), ...);
        }


        template<typename TY_vect, int ... ints>
        static auto extractVect(const TY_vect& vect, const std::integer_sequence<int, ints...> x) noexcept
        {
            return std::make_tuple(vect[ints]...);
        }


        template<typename TY_V>
        static auto BuildTuple(const TY_V value) noexcept
        {
            return std::tuple{ value };
        }


        static auto BuildTuple(const float2 f2) noexcept
        {
            return std::tuple{ f2.x, f2.y };
        }


        template<size_t vectorSize>
        static auto BuildTuple(const Vect<vectorSize, TY> vect)
        {
            return extractVect(vect, std::make_integer_sequence<int, vectorSize>());
        }


        template<typename TY_Value, typename ... TY_args>
        static auto BuildTuple(const TY_Value value, TY_args ... args) noexcept
        {
            return std::tuple_cat(BuildTuple(value), BuildTuple(args...));
        }


	public:
        Vect() noexcept = default;


		Vect(TY n) noexcept
		{
			for( auto& e : Vector )
				e = n;
		}


		template<typename TY_2>
		Vect(Vect<SIZE, TY_2> in) noexcept
		{
			for (size_t I=0; I<SIZE; ++I)
				Vector[I] = static_cast<TY>(in[I]);
		}


        template<typename ... TY_ARGS>
        Vect(TY_ARGS ... args) noexcept
        {
            static_assert(sizeof ... (args) <= SIZE, "Input value count must be less than container size!");

            const auto          tuple      = BuildTuple(args...);
            constexpr size_t    valueCount = std::tuple_size_v<decltype(tuple)>;
            const auto          indexes    = std::make_integer_sequence<int, valueCount>{};

            helper(tuple, indexes);
        }

		TY* begin() noexcept
		{
			return Vector;
		}


		TY* end() noexcept
		{
			return Vector + SIZE;
		}


		template<typename TY_i>
        [[nodiscard]] Vect<3, TY>	Cross( Vect<3, TY_i> rhs ) noexcept
		{
			Vect<SIZE, TY> out;
			for( size_t i = 0; i < SIZE; ++i )
				out[i] = ( Vector[(1+i)%SIZE] * rhs[(2+i)%SIZE] ) - ( rhs[(1+i)%SIZE] * Vector[(2+i)%SIZE] );

			return out;
		}


		template<typename TY_i>
        [[nodiscard]] TY Dot( const Vect<SIZE, TY_i>& rhs ) const noexcept
		{
			TY dotproduct = 0;
			for( size_t i = 0; i < SIZE; ++i )
				dotproduct += rhs[i] * Vector[i];

			return dotproduct;
		}


		template<typename TY_i>
        [[nodiscard]] TY Dot(const Vect<SIZE, TY_i>* rhs_ptr) noexcept
		{
			auto& rhs = *rhs_ptr;
			Vect<SIZE> products;
			for( size_t i = 0; i < SIZE; ++i )
				products[i] += rhs[i] * Vector[i];

			return products.Sum();
		}


        [[nodiscard]] TY Norm(unsigned int exp = 2) noexcept
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


        [[nodiscard]] TY Magnitude() noexcept
		{
			TY sum = 0;
			for( auto element : Vector )
			{
				sum += element * element;	
			}

			return sqrt( sum );
		}


		TY& operator [](const int index) noexcept
		{
			return Vector[index];
		}


        const TY& operator [](const int index) const noexcept
        {
            return Vector[index];
        }


        TY& operator [](const size_t index) noexcept
        {
            return Vector[index];
        }


        const TY& operator [](const size_t index) const noexcept
        {
            return Vector[index];
        }


        Vect operator & (this auto& self, const TY index) noexcept
        {
            Vect out;

            for (size_t I = 0; I < 3; I++)
                out[I] = self[I] & index;

            return out;
        }


        [[nodiscard]] constexpr TY Sum() const noexcept
		{
			TY sum = 0;
			for( auto element : Vector )
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
			for( auto element : Vector )
				Product *= element;
			return Product;
		}


		template<typename TY_2>
		THISTYPE operator + (const Vect<SIZE, TY_2>& in) const noexcept
		{
			THISTYPE temp = *this;
			for (auto I = 0; I < SIZE; ++I)
				temp[I] += in[I];
			
			return temp;
		}


		template<typename TY_2>
		THISTYPE& operator += (const Vect<SIZE, TY_2>& in) noexcept
		{
			for (auto I = 0; I < SIZE; ++I)
				Vector[I] += in[I];
			
			return *this;
		}


		template<typename TY_2>
		THISTYPE operator - (const Vect<SIZE, TY_2>& in) const noexcept
		{
			THISTYPE temp = *this;
			for (auto I = 0; I < SIZE; ++I)
				temp[I] -= in[I];
			
			return temp;
		}


		template<typename TY_2>
		THISTYPE& operator -= ( const Vect<SIZE, TY_2>& in ) noexcept
		{
			for (auto I = 0; I < SIZE; ++I)
				Vector[I] -= in[I];
			
			return *this;
		}


		template<typename TY_2>
		THISTYPE& operator = (const Vect<SIZE, TY_2>& in) noexcept
		{
			for (auto I = 0; I < SIZE; ++I)
				Vector[I] = in[I];
			
			return *this;
		}


		template<typename TY_2>
		THISTYPE operator / (TY_2 in) const noexcept
		{
			THISTYPE temp = *this;
			for (auto I = 0; I < SIZE; ++I)
				temp[I] /= in;

			return temp;
		}


		template<typename TY_2>
		THISTYPE operator % (TY_2 in) const noexcept
		{
			THISTYPE temp = *this;
			for (auto I = 0; I < SIZE; ++I)
				temp[I] %= in;

			return temp;
		}


		template<typename TY_2>
		THISTYPE& operator /= (TY_2 in) noexcept
		{
			for (auto I = 0; I < SIZE; ++I)
				Vector[I] /= in;

			return *this;
		}


		template<typename TY_2>
		THISTYPE operator /= (const Vect<SIZE, TY_2> in) noexcept
		{
			THISTYPE Out;
			for (auto I = 0; I < SIZE; ++I)
				Out[I] = Vector[I] / in[I];

			return Out;
		}


		template<typename TY_2>
		bool operator == (const Vect<SIZE, TY_2>& in) const noexcept
		{
			bool res = true;
			for (auto I = 0; I < SIZE; ++I)
				res = res & (Vector[I] == in[I]);

			return res;
		}


		template<typename TY_2>
		THISTYPE& operator = ( const std::initializer_list<TY_2>& il ) noexcept
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


        template<size_t ... TY_ARGS>
        auto Swizzle(this auto& self)
        {
			static_assert(sizeof ... (TY_ARGS) == Size(), "Incorrect number of arguments");
			return THISTYPE(self[TY_ARGS] ...);
        }


		static THISTYPE Zero() noexcept
		{
			THISTYPE zero;
			for( auto element : zero.Vector )
				element = 0;

			return zero;
		}


        constexpr static const size_t Size() noexcept
        {
            return SIZE;
        }


		operator TY* () noexcept { return Vector; }


        operator __m128 ()  noexcept
        {
            auto temp = _mm_loadu_ps(Vector);
            return _mm_loadr_ps((float*)&temp);
        }

		TY Vector[SIZE];
	};

	/************************************************************************************************/


    FLEXKITAPI template<typename TY_S, size_t ELEMENT_COUNT = 1>
    [[nodiscard]] constexpr Vect<ELEMENT_COUNT, TY_S> operator* (const Vect<ELEMENT_COUNT, TY_S> lhs, const Vect<ELEMENT_COUNT, TY_S> rhs)// vector multiply
	{
        Vect<ELEMENT_COUNT, TY_S> V_out;

		for (size_t i = 0; i < rhs.size(); ++i)
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
	
    typedef Vect<2, uint32_t>       uint2;
    typedef Vect<2, uint64_t>       uint2_64;

	typedef Vect<3, uint32_t>       uint3;
	typedef Vect<4, uint32_t>       uint4;
	typedef Vect<4, uint16_t>       uint4_16;
	typedef Vect<4, uint64_t>       uint4_32;

	typedef Vect<2, int> int2;
	typedef Vect<3, int> int3;
	typedef Vect<4, int> int4;


    typedef std::uint32_t uint;


	/************************************************************************************************/



	FLEXKITAPI template<typename TY_> float2 operator + (const float2& LHS, const Vect<2, TY_>& RHS) noexcept { return{ LHS.x + RHS[0], LHS.y + RHS[1] };}
	FLEXKITAPI template<typename TY_> float2 operator - (const float2& LHS, const Vect<2, TY_>& RHS) noexcept { return{ LHS.x - RHS[0], LHS.y - RHS[1] };}
	FLEXKITAPI template<typename TY_> float2 operator * (const float2& LHS, const Vect<2, TY_>& RHS) noexcept { return{ LHS.x * RHS[0], LHS.y * RHS[1] };}
	FLEXKITAPI template<typename TY_> float2 operator / (const float2& LHS, const Vect<2, TY_>& RHS) noexcept { return{ LHS.x / RHS[0], LHS.y / RHS[1] };}

    FLEXKITAPI template<typename TY_> Vect<2, TY_> operator * (const Vect<2, TY_> LHS, const float2 RHS) noexcept { return{ (uint32_t)(LHS[0] * RHS[0]), (uint32_t)(LHS[1] * RHS[1]) }; }


    FLEXKITAPI inline float2 operator * (const float   lhs, const float2 rhs) noexcept  { return float2(lhs) * rhs; }


	/************************************************************************************************/

    FLEXKITAPI inline float DotProduct2(const float* lhs, float* rhs) noexcept
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

    FLEXKITAPI inline float DotProduct3(const __m128& lhs, const __m128& rhs) noexcept
	{
#if USING(FASTMATH)
		__m128 res = _mm_dp_ps(lhs, rhs, 0x77);
		return _mm_cvtss_f32(res);

#else
		return ( lhs.m128_f32[0] * rhs.m128_f32[0] ) + ( lhs.m128_f32[1] * rhs.m128_f32[1] ) + ( lhs.m128_f32[2] * rhs.m128_f32[2] );
#endif
	}

    FLEXKITAPI inline float DotProduct4(const __m128& lhs, const __m128& rhs) noexcept
	{
#if USING(FASTMATH)
		__m128 res = _mm_dp_ps(lhs, rhs, 0xFF);
		return GetFirst(res);
#else
		return ( lhs.m128_f32[0] * rhs.m128_f32[0] ) + ( lhs.m128_f32[1] * rhs.m128_f32[1] ) + ( lhs.m128_f32[2] * rhs.m128_f32[2] );
#endif
	}

    FLEXKITAPI inline __m128 CrossProductSlow(const __m128 lhs, const __m128 rhs) noexcept
	{
		__m128 out = _mm_set1_ps(0);
		SetElement( out, (GetElement(lhs, 1) * GetElement(rhs, 2)) - (GetElement(lhs, 2) * GetElement(rhs, 1)), 0 );
		SetElement( out, (GetElement(lhs, 2) * GetElement(rhs, 0)) - (GetElement(lhs, 0) * GetElement(rhs, 2)), 1 );
		SetElement( out, (GetElement(lhs, 0) * GetElement(rhs, 1)) - (GetElement(lhs, 1) * GetElement(rhs, 0)), 2 );
		return out;
	}
	

    FLEXKITAPI inline __m128 CrossProduct(const __m128 a, const __m128 b ) noexcept
	{
#if USING(FASTMATH)
		__m128 temp1 = _mm_mul_ps(_mm_shuffle_ps(a, a, 0x01 | 0x02 << 2 | 0x00 << 4 | 0x00 << 6), _mm_shuffle_ps(b, b, 0x02 | 0x00 << 2 | 0x01 << 4 | 0x00 << 6));
		__m128 temp2 = _mm_mul_ps(_mm_shuffle_ps(a, a, 0x02 | 0x00 << 2 | 0x01 << 4 | 0x00 << 6), _mm_shuffle_ps(b, b, 0x01 | 0x02 << 2 | 0x00 << 4 | 0x00 << 6));
		__m128 res	 = _mm_sub_ps(temp1, temp2);

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
        float3() noexcept {}

        float3(float val)	noexcept { pfloats = _mm_set_ps1(val); }
        float3(float X, float Y, float Z)	    noexcept { pfloats = _mm_set_ps(0.0f, Z, Y, X); }
        float3(const float2 in, float Z = 0)	noexcept { pfloats = _mm_setr_ps(in.x, in.y, Z, 0.0f); }
        float3(const float3& a)     noexcept { pfloats = a.pfloats; }
        float3(const __m128& in)    noexcept { pfloats = in; }

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
            pfloats = _mm_load_ps(f);
            return *this;
        }


        float3& operator = (float F) noexcept
        {
            pfloats = _mm_set_ps1(F);
            return *this;
        }


        float3& operator = (const float3& F) noexcept
        {
            pfloats = F.pfloats;
            return *this;
        }


        float& operator[] (const size_t index)	        noexcept { return *GetElement_ptr(pfloats, index); }
        float operator[]  (const size_t index)	const   noexcept { return GetElement(pfloats, index); }

        // Operator Overloads
        float3 operator - ()	        noexcept                { return _mm_mul_ps(pfloats, _mm_set_ps1(-1)); }
        float3 operator - ()	const   noexcept                { return _mm_mul_ps(pfloats, _mm_set_ps1(-1)); }
        float3 operator + (const float& rhs)	const noexcept  { return _mm_add_ps(pfloats, _mm_set_ps1(rhs)); }
        float3 operator + (const float3& rhs)	const noexcept  { return _mm_add_ps(pfloats, rhs); }


        float3& operator += (const float3& rhs) noexcept
        {
            pfloats = _mm_add_ps(pfloats, rhs);

            return *this;
        }

        float3& operator += (const float rhs) noexcept
        {
            pfloats = _mm_add_ps(pfloats, _mm_set1_ps(rhs));

            return *this;
        }

        float3 operator - (const float rhs) const noexcept
        {
            return _mm_sub_ps(pfloats, _mm_set1_ps(rhs));
        }


        float3& operator -= (const float3& rhs) noexcept
        {
            pfloats = _mm_sub_ps(pfloats, rhs.pfloats);

            return *this;
        }

        float3& operator -= (const float rhs) noexcept
        {
            pfloats = _mm_sub_ps(pfloats, _mm_set1_ps(rhs));

            return *this;
        }


        bool operator == (const float3& rhs) const noexcept
        {
            if (rhs.x == x)
                if (rhs.y == y)
                    if (rhs.z == z)
                        return true;
            return false;
        }


        float3 operator - (const float3& a) const noexcept
        {
            return _mm_sub_ps(pfloats, a);
        }

        static bool Compare(const float3& lhs, const float3& rhs, float ep = 0.001f) noexcept
        {
            float3 temp = lhs - rhs;
            return (temp.x < ep) && (temp.y < ep) && (temp.z < ep);
        }

        float3 operator *	(const float3& a) const noexcept
        {
#if USING(FASTMATH)
            return _mm_mul_ps(a.pfloats, pfloats);
#else
            return float3(x * a.x, y * a.y, z * a.z);
#endif
        }


        float3 operator *	(const float a) const noexcept
        {
#if USING(FASTMATH)
            return _mm_mul_ps(_mm_set1_ps(a), pfloats);
#else
            return float3(x * a, y * a, z * a);
#endif

        }


        float3& operator *=	(const float3& a) noexcept
        {
            pfloats = _mm_mul_ps(pfloats, a);

            return *this;
        }


        float3& operator *=	(float a) noexcept
        {
            pfloats = _mm_mul_ps(pfloats, _mm_set1_ps(a));

            return *this;
        }


        float3 operator / (const float a) const noexcept
        {
            return _mm_div_ps(pfloats, _mm_set1_ps(a));
        }


        float3 operator / (const float3& a) const noexcept
        {
            return _mm_div_ps(pfloats, a);
        }


        float3& operator /= (const float a) noexcept
        {
            pfloats = _mm_div_ps(pfloats, _mm_set1_ps(a));
            return *this;
        }


        float3& operator /= (const float3& a) noexcept
        {
            pfloats = _mm_div_ps(pfloats, a);
            return *this;
        }


        float3& Scale(float S) noexcept
        {
            pfloats = _mm_mul_ps(pfloats, _mm_set1_ps(S));
            return *this;
        }


        const float3 inverse() const noexcept
        {
            return _mm_mul_ps(pfloats, _mm_set1_ps(-1));
        }

        // Identities
        const float3 cross(const float3& rhs) const noexcept
        {
            return CrossProduct(pfloats, rhs.pfloats);
        }


        const float3 distance(const float3& b) const noexcept
        {
            return (*this - b).magnitude();
        }


        float dot(const float3& b) const noexcept
        {
            __m128 res = _mm_dp_ps(pfloats, b.pfloats, 0b01110001);

            return _mm_cvtss_f32(res);
        }


        float3 abs() const noexcept
        {
            return { fabs(x), fabs(y), fabs(z) };
        }


        float magnitude() const noexcept
        {
            auto sq    = _mm_mul_ps(pfloats, pfloats);
            auto temp1 = _mm_movehdup_ps(sq);
            auto temp2 = _mm_add_ps(sq, temp1);
            auto temp3 = _mm_movehl_ps(sq, sq);
            auto temp4 = _mm_add_ps(temp2, temp3);

            return _mm_cvtss_f32(_mm_sqrt_ps(temp4));
        }


        inline float magnitudeSq() const noexcept
        {
            auto sq    = _mm_mul_ps(pfloats, pfloats);
            auto temp1 = _mm_movehdup_ps(sq);
            auto temp2 = _mm_add_ps(sq, temp1);
            auto temp3 = _mm_movehl_ps(sq, sq);
            auto temp4 = _mm_add_ps(temp2, temp3);

            return _mm_cvtss_f32(temp4);
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
            auto temp1 = _mm_movehdup_ps(pfloats);
            auto temp2 = _mm_max_ps(pfloats, temp1);
            auto temp3 = _mm_movehl_ps(pfloats, pfloats);
            auto temp4 = _mm_max_ps(temp2, temp3);

            return _mm_cvtss_f32(temp4);
        }

        float Min() const noexcept
        {
            auto temp1 = _mm_movehdup_ps(pfloats);
            auto temp2 = _mm_min_ps(pfloats, temp1);
            auto temp3 = _mm_movehl_ps(pfloats, pfloats);
            auto temp4 = _mm_min_ps(temp2, temp3);

            return _mm_cvtss_f32(temp4);
        }

		float3 normal() const noexcept
		{
            auto sq     = _mm_mul_ps(pfloats, pfloats);
            auto temp1  = _mm_movehdup_ps(sq);
            auto temp2  = _mm_add_ps(sq, temp1);
            auto temp3  = _mm_movehl_ps(sq, sq);
            auto temp4  = _mm_add_ps(temp2, temp3);
            auto m      = _mm_shuffle_ps(temp4, temp4, _MM_SHUFFLE(0, 0, 0, 0));
            
            return _mm_div_ps(pfloats, _mm_sqrt_ps(m));
		}

        static float3 Zero() { return float3{ 0 }; }

		operator __m128 () const	 { return pfloats; }

		inline float* toFloat3_ptr() noexcept   { return reinterpret_cast<float*>( &pfloats ); }
        operator float* () noexcept             { return toFloat3_ptr(); }

		operator Vect3 () const		 { return { x, y, z }; }
		operator float2() const		 { return { x, y }; }

		struct
		{
			float x, y, z, PAD;
		};

		__m128	pfloats;

		static float3 Load(float* a) noexcept
		{
			auto temp = _mm_loadu_ps(a);
			return float3(_mm_loadr_ps((float*)&temp));
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
			return _mm_set1_ps(in);
		}

	};


	FLEXKITAPI const float3 BLACK	= float3(0.0f, 0.0f, 0.0f);
	FLEXKITAPI const float3 WHITE	= float3(1.0f, 1.0f, 1.0f);
	FLEXKITAPI const float3 RED	    = float3(1.0f, 0.0f, 0.0f);
	FLEXKITAPI const float3 BLUE	= float3(0.0f, 0.0f, 1.0f);
	FLEXKITAPI const float3 GREEN	= float3(0.0f, 1.0f, 0.0f);
	FLEXKITAPI const float3 PURPLE	= float3(1.0f, 0.0f, 1.0f);


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


	inline float3 operator* ( float s, float3 V )
	{
#if USING(FASTMATH)
		return _mm_mul_ps(V, _mm_set1_ps(s));
#else
		return V*s;
#endif
	}


	inline float3 RotateVectorAxisAngle( float3 N, float a, float3 V ) { return V*cos(a) + (V.dot(N) * N * (1-cos(a)) + (N.cross(V)*sin(a)));	}
	
    FLEXKITAPI union alignas(16) float4
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

		inline float4( const float3& V,  const float W = 0) 
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


		inline operator float*				()					{ return (float*)&pFloats;} 
		inline operator const float* const	()	const			{ return (float*)&pFloats;}
		inline float& operator[] ( const size_t index )			{ return GetElement_ref( pFloats, index); }
		inline float operator[]  ( const size_t index )	const	{ return GetElement( pFloats, index); }
		inline operator __m128	 ()						const	{ return pFloats;} 

		inline float4 operator+ ( const float4& rhs ) const 
		{
#if USING(FASTMATH)
			return _mm_add_ps(pFloats, rhs.pFloats);
#else
			return float4(	x + rhs.x,
							y + rhs.y,
							z + rhs.z,
							w + rhs.w );
#endif
		}

		inline float4 operator+ (const float rhs) const
		{
#if USING(FASTMATH)
			return _mm_add_ps(pFloats, _mm_set1_ps(rhs));
#else
			return float4(	x + rhs, 
							y + rhs, 
							z + rhs, 
							w + rhs );
#endif
		}

		inline float4 operator- ( const float4& rhs) const
		{
#if USING(FASTMATH)
			return _mm_sub_ps(pFloats, rhs);
#else
			return float4(	x - rhs.x, 
							y - rhs.y, 
							z - rhs.z, 
							w - rhs.w );
#endif
		}

		inline float4 operator- ( const float rhs) const
		{
#if USING(FASTMATH)
			return _mm_sub_ps(pFloats, _mm_set1_ps(rhs));
#else
			return float4(	x - rhs, 
							y - rhs, 
							z - rhs, 
							w - rhs );
#endif
		}

		//inline float4 operator* ( const float4 a ) const
		//{
#if USING(FASTMATH)
		//	return _mm_mul_ps(pFloats, a);
#else
			return float4(	x * rhs.x, 
							y * rhs.y, 
							z * rhs.z, 
							w * rhs.w );
#endif
		//}

		inline float4 operator* (const float rhs) const
		{
#if USING(FASTMATH)
			return _mm_mul_ps(pFloats, _mm_set1_ps(rhs));
#else
			return float4(	x * rhs, 
							y * rhs, 
							z * rhs, 
							w * rhs );
#endif
		}

		inline float4 operator += (const float4& rhs)
		{
#if USING(FASTMATH)
			pFloats = _mm_add_ps(pFloats, rhs);
#else
			return float4(	x + rhs.x, 
							y + rhs.y, 
							z + rhs.z, 
							w + rhs.w );
#endif
			return pFloats;
		}

		inline float4 operator / (const float4& rhs) const
		{
#if USING(FASTMATH)
			return _mm_div_ps(pFloats, rhs);
#else
			return float4(	x / rhs.x, 
							y / rhs.y, 
							z / rhs.z, 
							w / rhs.w );
#endif
		}

		inline float4& operator /= (const float rhs)
		{
#if USING(FASTMATH)
			pFloats = _mm_div_ps(pFloats, _mm_set1_ps(rhs));

			return *this;
#else
			return float4(	x / rhs.x, 
							y / rhs.y, 
							z / rhs.z, 
							w / rhs.w );
#endif
		}

		inline float4 operator / (const float rhs) const
		{
#if USING(FASTMATH)
			return _mm_div_ps(pFloats, _mm_set1_ps(rhs));
#else
			return float4(	x / rhs, 
							y / rhs, 
							z / rhs, 
							w / rhs );
#endif
		}

		inline float4 operator % (const float4& rhs) const
		{
			return float4(	std::fmod( x, rhs.x ), 
							std::fmod( y, rhs.y ),
							std::fmod( z, rhs.z ),
							std::fmod( w, rhs.w ) );
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

		float3 xyz() const 
		{
			return { x, y, z };
		}

		operator Vect4 ()		{ return{ x, y, z, w }; };
		operator Vect4 () const { return{ x, y, z, w }; };

		constexpr static size_t Size() { return 4; }

        /*
        void Serialize(auto& ar)
        {
            ar& x;
            ar& y;
            ar& z;
            ar& w;
        }
        */

		__m128 pFloats;
	};

	inline float  F4Dot		(float4 rhs, float4 lhs)				{ return DotProduct4(lhs, rhs); }
	inline float4 operator* (float4 lhs, float4 rhs)				{ return _mm_mul_ps(lhs, rhs); }
	inline float4 F4MUL		(const float4 lhs, const float4 rhs)	{ return _mm_mul_ps(lhs, rhs); }


	/************************************************************************************************/

    FLEXKITAPI union alignas(16) Quaternion
	{
	public:
		inline Quaternion() {}
		inline Quaternion(__m128 in) { floats = in; }


		inline explicit Quaternion(const float3& vector, float scaler)
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


		inline Quaternion(float X, float Y, float Z, float W)
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


		inline explicit Quaternion( float* in )
		{
			_mm_store_ps( in, floats );
		}


		inline explicit Quaternion( const Quaternion& in ) :
			floats( in.floats )	{}


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


		inline Quaternion& operator *= (const Quaternion& rhs )
		{
			(*this) = GrassManProduct(*this, rhs);
			return (*this);
		}


		inline Quaternion& operator = (const Quaternion& rhs )
		{
			floats = rhs.floats;
			return (*this);
		}


		inline Quaternion& operator = (const  __m128 rhs )
		{
			floats = rhs;
			return (*this);
		}


		inline Quaternion operator * (const Quaternion q) const
		{
			return GrassManProduct(q, *this);
		}


		inline float& operator [] (const size_t index)		    { return GetElement_ref	(floats, index); }
		inline float operator  [] (const size_t index) const	{ return GetElement		(floats, index); }

        inline float& operator [] (const int index)		    { return GetElement_ref	(floats, index); }
		inline float operator  [] (const int index) const	{ return GetElement		(floats, index); }

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

		inline Quaternion Conjugate() const noexcept
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
		inline Quaternion Inverse() const { return  Conjugate(); }
		
		inline float dot(Quaternion rhs) const noexcept
		{ 
			return V().dot(rhs.V()) + w * rhs.w; 
		}

		inline Quaternion operator / (float denom)		{return { x / denom , y / denom , z / denom , w / denom };}
		inline Quaternion operator + (Quaternion RHS)	{return { x + RHS.x, y + RHS.y, z + RHS.z, w + RHS.w }; }

		inline float3 XYZ() const { return float3(x, y, z); }
		inline float3 V()   const { return float3(x, y, z); }

		inline float Magnitude() const noexcept
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

		inline Quaternion& normalize() noexcept
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


		inline Quaternion normal() const noexcept
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
            else
                Res = floats;

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

        void Serialize(auto& ar)
        {
            ar& x;
            ar& y;
            ar& z;
            ar& w;
        }

		constexpr static size_t Size() { return 4; }

		struct
		{
			float x, y, z, w;
		};

		 __m128	floats;
	};


	/************************************************************************************************/


    FLEXKITAPI inline float3		operator * (const Quaternion P, const float3 V)
	{
		auto v		= -1 * P.XYZ();
		auto vXV	= v.cross(V);
		auto ret	= float3(V + (vXV * (2 * P.w)) + (v.cross(vXV) * 2));

		return ret;
	}


    FLEXKITAPI inline Quaternion operator * (const Quaternion Q, const float scaler)
	{
		__m128 r = Q;
		__m128 s = _mm_set1_ps(scaler);
		return _mm_mul_ps(r, s);
	}


	/************************************************************************************************/


    FLEXKITAPI inline Quaternion Qlerp(Quaternion P, Quaternion Q, float W)
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
	

    FLEXKITAPI inline float3	Vect3ToFloat3(const Vect3 R3)  noexcept { return {R3[0], R3[1], R3[2]}; }
    FLEXKITAPI inline Vect3	    Float3ToVect3(const float3 R3) noexcept { return {R3[0], R3[1], R3[2]}; }
	
    FLEXKITAPI inline float4	Vect4ToFloat4(const Vect4  R4) noexcept { return {R4[0], R4[1], R4[2], R4[3]}; }
	FLEXKITAPI inline Vect4	    Float4ToVect4(const float4 R4) noexcept { return {R4[0], R4[1], R4[2], R4[3]}; }


	/************************************************************************************************/


    FLEXKITAPI inline float Vect3FDot(const Vect3& lhs, const Vect3& rhs) noexcept
	{
		auto temp1 = _mm_set_ps(0, lhs.Vector[2], lhs.Vector[1], lhs.Vector[0]);
		auto temp2 = _mm_set_ps(0, rhs.Vector[2], rhs.Vector[1], rhs.Vector[0]);

		__m128 res = _mm_dp_ps(temp1, temp2, 0xFF);

		return GetFirst(res);
	}


	/************************************************************************************************/


    FLEXKITAPI inline float Vect4FDot( const Vect4& lhs, const Vect4& rhs )
	{
		auto temp1 = _mm_loadr_ps(lhs.Vector); //_mm_set_ps(lhs.Vector[3], lhs.Vector[2], lhs.Vector[1], lhs.Vector[0]);
		auto temp2 = _mm_loadr_ps(rhs.Vector); //_mm_set_ps(rhs.Vector[3], rhs.Vector[2], rhs.Vector[1], rhs.Vector[0]);

		__m128 res = _mm_dp_ps(temp1, temp2, 0xFF);

		return GetFirst(res);
	}


	/************************************************************************************************/


#pragma warning(push)
#pragma warning(disable : 4324)
	// Row Major
	template< const int ROW, const int COL, typename Ty = float >
	union alignas(16) Matrix
	{
    private:
        template<typename TY_tuple, int ... ints>
        void helper(TY_tuple&& tuple, const std::integer_sequence<int, ints...> x) noexcept
        {
            Ty* flatArray = &matrix[0][0];
            ((flatArray[ints] = static_cast<Ty>(std::get<ints>(tuple))), ...);
        }


        template<typename TY_vect, int ... ints>
        static auto extractVect(const TY_vect& vect, const std::integer_sequence<int, ints...> x) noexcept
        {
            return std::make_tuple(vect[ints]...);
        }


        template<typename TY>
        static auto BuildTuple(const TY value) noexcept
        {
            return std::tuple{ value };
        }


        template<size_t vectorSize>
        static auto BuildTuple(const Vect<vectorSize, Ty> vect)
        {
            return extractVect(vect, std::make_integer_sequence<int, vectorSize>());
        }


        static auto BuildTuple(const float2 vect) noexcept
        {
            return std::make_tuple(vect[0], vect[1]);
        }


        static auto BuildTuple(const float3 vect) noexcept
        {
            return std::make_tuple(vect[0], vect[1], vect[2]);
        }


        static auto BuildTuple(const float4 vect) noexcept
        {
            return std::make_tuple(vect[0], vect[1], vect[2], vect[3]);
        }


        template<typename TY_Value, typename ... TY_args>
        static auto BuildTuple(const TY_Value value, TY_args ... args) noexcept
        {
            return std::tuple_cat(BuildTuple(value), BuildTuple(args...));
        }


	public:
        using THIS_TYPE = const Matrix<ROW, COL, Ty>;

        Matrix()                                            = default;
        Matrix(const THIS_TYPE& initial)                    = default;

        template<typename ... TY_ARGS>
        explicit Matrix(TY_ARGS ... args) noexcept
        {
            const std::tuple tuple      = BuildTuple(args...);

            constexpr size_t valueCount = std::tuple_size_v<decltype(tuple)>;
            const auto indexes          = std::make_integer_sequence<int, valueCount>{};

            static_assert(valueCount <= ROW * COL, "Input value count must be less than container size!");

            helper(tuple, indexes);

            Ty* flatArray = &matrix[0][0];

            for (size_t I = valueCount; I < ROW * COL; I++)
                flatArray[I] = 0.0f;
        }

        Matrix<ROW, COL> operator * (const float rhs)
		{
			Matrix<ROW, COL> out = *this;

			for (auto& c : out.matrix)
				for(auto& e : c)
					e = e * rhs;

			return out;
		}



		template< const int RHS_COL >
		Matrix<ROW, RHS_COL> operator * ( const Matrix<ROW, RHS_COL>& rhs ) const noexcept
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


		Matrix<3, 3> operator * (const Matrix<3, 3>& rhs) const noexcept
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


		Matrix<4, 4> operator*( const Matrix<4, 4>& rhs ) const noexcept
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


		Vect<ROW>& operator[] (const int i)         { return *((Vect<ROW>*)matrix[i]); }
        Vect<ROW>& operator[] (const int i) const   { return *((Vect<ROW>*)matrix[i]); }

        Vect<ROW>& operator[] (const size_t i)          { return *((Vect<ROW>*)matrix[i]); }
        Vect<ROW>& operator[] (const size_t i) const    { return *((Vect<ROW>*)matrix[i]); }

        operator        Ty* () noexcept { return (Ty*)matrix; }
        operator const  Ty* () const noexcept { return (Ty*)matrix; }

		static inline Matrix<ROW, COL> Identity() noexcept
		{
			Matrix<ROW, COL> m = Zero();

			for( size_t i = 0; i < ROW; i++ )
				m[i][i] = 1;

			return m;
		}


		static inline Matrix<ROW, COL> Zero() noexcept
		{
			Matrix<ROW, COL> m;
			for (size_t I = 0; I < ROW; ++I)
				for (size_t II = 0; II < COL; II++)
					m[I][II] = 0.0;

			return m;
		}


		Matrix<ROW, COL> Transpose() const noexcept
		{
			Matrix<ROW, COL> m_transposed;

			for( size_t col = 0; col < COL; ++col )
				for( size_t row = 0; row < ROW; ++row )
					m_transposed[col][row] = matrix[row][col];

			return m_transposed;
		}


		Ty matrix[COL][ROW];
	};

#pragma warning(pop)


	/************************************************************************************************/


    FLEXKITAPI inline float2 Mulfloat2(Matrix<2, 2, float>& LHS, float2& RHS)
	{
		float2 Out;
		auto transposed = LHS.Transpose();

		for (size_t I = 0; I < 2; ++I)
			Out[I] = DotProduct2(transposed.matrix[I], RHS);

		return Out;
	}


    FLEXKITAPI inline float3 Mulfloat3(Matrix<3, 3, float>& LHS, float3& RHS)
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


    FLEXKITAPI inline float4x4 TranslationMatrix(float3 POS)
	{
		float4x4 Out = float4x4::Identity();
		
        Out[3] = Float4ToVect4(float4(POS, 1));

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


    FLEXKITAPI inline float4x4 FastInverse(const float4x4 m)
    {
        float4x4 inverseRotation = m;
        inverseRotation[0][3]   = 0.0f;
        inverseRotation[1][3]   = 0.0f;
        inverseRotation[2][3]   = 0.0f;
        inverseRotation[3]      = Vect4{ -m[0][3], -m[1][3], -m[2][3], 1};
        inverseRotation         = inverseRotation.Transpose();

        return inverseRotation;
    }


	/************************************************************************************************/


	template<typename TY>
	inline TY Lerp(const TY A, const TY B, float t){ return A * (1.0f - t) + t*B; }

	template<typename TY>
	inline TY lerp(const TY A, const TY B, float t) { return A * (1.0f - t) + t * B; }

	namespace Conversion
	{
		template< typename Ty >
		static Vect3 toVector3( Ty& Convert ) { return Vect3( Convert.x, Convert.y, Convert.z ); }

		template< typename Ty > Ty Vect3To( const Vect3& Convert )	{ return Ty{Convert[0], Convert[1], Convert[2]}; }
		template< typename Ty >	Ty Vect4To( const Vect4& Convert )	{ return Ty{Convert[0], Convert[1], Convert[2], Convert[3]}; }

		template< typename Ty_Out, typename Ty_2>	Ty_Out Vect2To( const Ty_2& Convert) { return Ty_Out{ Convert[0], Convert[1] }; }

		template<typename TY>
		float2 Vect2TOfloat2(Vect<2, TY> Convert) { return float2{ (float)Convert[0], (float)Convert[1] }; }
	}


	/************************************************************************************************/


	inline float4 operator * (const float4x4& LHS, const float4 rhs)
	{// TODO: FAST PATH
		const Vect4 temp    = rhs;
		const auto T        = LHS.Transpose();

        return Conversion::Vect4To<float4>(
            {   T[0].Dot(temp),
                T[1].Dot(temp),
                T[2].Dot(temp),
                T[3].Dot(temp),
            });
	}


    inline float3 operator * (const float3x3& LHS, const float3 rhs)
    {// TODO: FAST PATH
        const Vect3   temp    = rhs;
        const auto    T       = LHS.Transpose();

        return Conversion::Vect3To<float3>(
            {   T[0].Dot(temp),
                T[1].Dot(temp),
                T[2].Dot(temp),
            });
    }


	/************************************************************************************************/


	FLEXKITAPI int			Exp( int32_t Number, uint32_t exp );
	FLEXKITAPI Quaternion	MatrixToQuat( Matrix<4,4>& );
	FLEXKITAPI void			NumberToString( int32_t n, std::string& _Dest );
	FLEXKITAPI int			Testing();

	FLEXKITAPI void printfloat2(const float2& in);
	FLEXKITAPI void printfloat3(const float3& in);
	FLEXKITAPI void printfloat4(const float4& in);
	FLEXKITAPI void printQuaternion(const Quaternion in);


	inline float dot(const float3 lhs, const float3 rhs)
	{
		return DotProduct3(lhs, rhs);
	}


	inline float dot(const float4 lhs, const float4 rhs)
	{
		return DotProduct4(lhs, rhs);
	}


	/************************************************************************************************/


    FLEXKITAPI inline FlexKit::float4x4 Quaternion2Matrix(Quaternion q)
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

        return Quaternion{ res }.normalize();
#else 

		Quaternion Q
		{
			sqrtf( Max(1.0f + M[0][0] - M[1][1] - M[2][2], 0.0f))/2, 
			sqrtf( Max(1.0f - M[0][0] + M[1][1] - M[2][2], 0.0f))/2, 
			sqrtf( Max(1.0f - M[0][0] - M[1][1] + M[2][2], 0.0f))/2, 
			sqrtf( Max(1.0f + M[0][0] + M[1][1] + M[2][2], 0.0f))/2
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
		out[0] = Float4ToVect4(float4{ 1 - Q.y*Q.y - 2 * Q.z*Q.z, 2 * Q.x*Q.y + 2 * Q.z*Q.w, 2 * Q.x * Q.z - 2 * Q.y * Q.w, 0  });
		out[1] = Float4ToVect4(float4{ 2 * Q.x*Q.y - 2 * Q.z*Q.w, 1 - Q.x*Q.x - 2 * Q.z*Q.z, 2 * Q.y * Q.z - 2 * Q.x * Q.w, 0  });
		out[2] = Float4ToVect4(float4{ 2*Q.x*Q.z + 2*Q.y*Q.w, 2*Q.y*Q.z - 2*Q.x*Q.w, 1 - 2 * Q.x * Q.x - 2 * Q.y*Q.y,		 0 });
		out[3] = Float4ToVect4(float4{ 0,						0,						0,								1	   });

		return out;
	}


	/************************************************************************************************/


	inline Quaternion PointAt(float3 A, float3 B, const float3 UpV = { 0.0f, 1.0f, 0.0f })
	{
		float3 Dir		        = (B - A).normal();
		Dir                     = {Dir.z, Dir.y, -Dir.x};
		const float3 DirXUpV	= Dir.cross(UpV);

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

#endif


/**********************************************************************

Copyright (c) 2015 - 2020 Robert May

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
