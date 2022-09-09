#include "MathUtils.h"
#include "MathUtils.cpp"
#include "intersection.h"
#include "intersection.cpp"
#include "containers.h"
#include <iostream>
#include <immintrin.h>
#include <chrono>
#include <windows.h>

using namespace FlexKit;

struct Triangle
{
	FlexKit::float3 position[3];

	void Serialize(auto& ar)
	{
		ar& position[0];
		ar& position[1];
		ar& position[2];
	}

	FlexKit::float3& operator [] (size_t idx) noexcept;
	const FlexKit::float3&  operator [] (size_t idx) const noexcept;
	Triangle                Offset(FlexKit::float3) const noexcept;
	FlexKit::AABB           GetAABB() const noexcept;
	const FlexKit::float3   Normal() const noexcept;
	const FlexKit::float3   TriPoint() const noexcept;
};


Triangle Triangle::Offset(FlexKit::float3 offset) const noexcept
{
	Triangle tri_out;

	for (size_t I = 0; I < 3; I++)
		tri_out[I] = position[I] + offset;

	return tri_out;
}


FlexKit::float3& Triangle::operator [] (size_t idx) noexcept
{
	return position[idx];
}


const FlexKit::float3& Triangle::operator [] (size_t idx) const noexcept
{
	return position[idx];
}


FlexKit::AABB Triangle::GetAABB() const noexcept
{
	FlexKit::AABB aabb;
	aabb += position[0];
	aabb += position[1];
	aabb += position[2];

	return aabb;
}


const float3 Triangle::Normal() const noexcept
{
	return FlexKit::TripleProduct(position[0], position[1], position[2]);
}


const float3 Triangle::TriPoint() const noexcept
{
	__m128 w = _mm_set1_ps(1.0f / 3.0f);
	__m128 sum = _mm_set1_ps(0.0f);

	sum = _mm_fmadd_ps(position[0], w, sum);
	sum = _mm_fmadd_ps(position[1], w, sum);
	sum = _mm_fmadd_ps(position[2], w, sum);

	return sum;
}


float ProjectPointOntoRay(const FlexKit::float3& vector, const FlexKit::Ray& r) noexcept
{
	return (vector - r.O).dot(r.D);
}


bool Intersects(const float2 A_0, const float2 A_1, const float2 B_0, const float2 B_1, float2& intersection) noexcept
{
	const auto S0 = (A_1 - A_0);
	const auto S1 = (B_1 - B_0);

	if ((S1.y / S1.x - S0.y / S0.x) < 0.000001f)
		return false; // Parallel

	const float C = S0.x * S1.y - S1.y + S1.x;
	const float A = A_0.x * A_1.y - A_0.y * A_1.x;
	const float B = B_0.x * B_1.y - B_0.y * B_1.x;

	const float x = A * S1.x - B * S0.x;
	const float y = A * S1.y - B * S0.y;

	intersection = { x, y };

	return( x >= Min(A_0.x, A_1.x) &&
			x <= Max(A_0.x, A_1.x) &&
			y >= Min(A_0.y, A_1.y) &&
			y <= Max(A_0.y, A_1.y));
}


bool Intersects(const float2 A_0, const float2 A_1, const float2 B_0, const float2 B_1) noexcept
{
	float2 _;
	return Intersects(A_0, A_1, B_0, B_1, _);
}


bool Intersects(const Triangle& A, const Triangle& B) noexcept
{
	const auto A_n = A.Normal();
	const auto A_p = A.TriPoint();
	const auto B_n = B.Normal();
	const auto B_p = B.TriPoint();


	float3 distances_A;
	float3 distances_B;
	for (size_t idx = 0; idx < 3; ++idx)
	{
		const auto t2   = A[idx] - B_p;
		const auto t1   = B[idx] - A_p;
		const auto dp1  = t1.dot(A_n);
		const auto dp2  = t2.dot(B_n);

		distances_A[idx] = dp2;
		distances_B[idx] = dp1;
	}

	const bool infront1   = ((distances_B[0] >= 0.0f) | (distances_B[1] >= 0.0f) | (distances_B[2] >= 0.0));
	const bool behind1    = ((distances_B[0] <= 0.0f) | (distances_B[1] <= 0.0f) | (distances_B[2] <= 0.0));
	const bool infront2   = ((distances_A[0] >= 0.0f) | (distances_A[1] >= 0.0f) | (distances_A[2] >= 0.0));
	const bool behind2    = ((distances_A[0] <= 0.0f) | (distances_A[1] <= 0.0f) | (distances_A[2] <= 0.0));

	if(!(infront1 && behind1) || !(infront2 && behind2))
		return false;

	const auto M_a = distances_A.Max();
	const auto M_b = distances_B.Max();

	if ((fabs(M_a) < 0.00001f) || (fabs(M_b) < 0.00001f))
	{   // Coplanar
		const float3 n = FlexKit::SSE_ABS(A_n);
		int i0, i1;

		if (n[0] > n[1])
		{
			if (n[0] > n[2])
			{
				i0 = 1;
				i1 = 2;
			}
			else
			{
				i0 = 0;
				i1 = 1;
			}
		}
		else
		{
			if (n[2] > n[1])
			{
				i0 = 0;
				i1 = 1;
			}
			else
			{
				i0 = 0;
				i1 = 2;
			}
		}

		const float2 XY_A[3] =
		{
			{ A[0][i0], A[0][i1] },
			{ A[1][i0], A[1][i1] },
			{ A[2][i0], A[2][i1] }
		};

		const float2 XY_B[3] =
		{
			{ B[0][i0], B[0][i1] },
			{ B[1][i0], B[1][i1] },
			{ B[2][i0], B[2][i1] }
		};

		// Edge to edge intersection testing

		bool res = false;
		for (size_t idx = 0; idx < 3; idx++)
		{
			for (size_t idx2 = 0; idx2 < 3; idx2++)
			{
				res |= Intersects(
					XY_A[(idx + idx2 + 0) % 3],
					XY_A[(idx + idx2 + 1) % 3],
					XY_B[(idx + idx2 + 0) % 3],
					XY_B[(idx + idx2 + 1) % 3]);
			}
		}

		if (!res)
		{
			const auto P1 = XY_B[0];

			const auto sign = [](auto& p1, auto& p2, auto& p3)
			{
				return (p1.x - p3.x) * (p2.y - p3.y) - (p2.x - p3.x) * (p1.y - p3.y);
			};

			const float S0 = sign(P1, XY_A[0], XY_A[1]);
			const float S1 = sign(P1, XY_A[1], XY_A[2]);
			const float S2 = sign(P1, XY_A[2], XY_A[0]);

			const bool neg = S0 < 0.0f || S1 < 0.0f || S2 < 0.0f;
			const bool pos = S0 > 0.0f || S1 > 0.0f || S2 > 0.0f;

			return !(neg && pos);
		}
		else
			return res;
	}
	else
	{   // Non-Planar
		const auto L_n = A_n.cross(B_n);
		const FlexKit::Plane p{ B_n, B_p };

		const FlexKit::Ray r[] = {
			{ (A[1] - A[0]).normal(), A[0] },
			{ (A[2] - A[1]).normal(), A[1] },
			{ (A[0] - A[2]).normal(), A[2] } };

		const auto w0 = p.o - r[0].O;
		const auto w1 = p.o - r[1].O;
		const auto w2 = p.o - r[2].O;

		const float3 W_dp = { w0.dot(p.n),      w1.dot(p.n),    w2.dot(p.n) };
		const float3 V_dp = { r[0].D.dot(p.n),  r[1].D.dot(p.n), r[2].D.dot(p.n) };

		const float3 i = W_dp / V_dp;

		size_t idx = 0;
		float d = INFINITY;

		for (size_t I = 0; I < 3; I++)
		{
			//if (!std::isnan(i[I]) && !std::isinf(i[I]))
			{
				if (i[I] < d)
				{
					idx = I;
					d   = i[I];
				}
			}
		}

		const FlexKit::Ray L{ L_n, r[idx].R(i[idx]) };

		const float3 A_L = {
			ProjectPointOntoRay(A[0], L),
			ProjectPointOntoRay(A[1], L),
			ProjectPointOntoRay(A[2], L),
		};

		const float3 B_L = {
			ProjectPointOntoRay(B[0], L),
			ProjectPointOntoRay(B[1], L),
			ProjectPointOntoRay(B[2], L),
		};

		const float a_min = A_L.Min();
		const float a_max = A_L.Max();

		const float b_min = B_L.Min();
		const float b_max = B_L.Max();

		return (a_min < b_max) && (b_min < a_max);
	}
}


int main()
{
	Triangle A;
	Triangle B; // Perpendicular to A, intersects
	Triangle C; // In plane to A, contained
	Triangle D; // In plane to A, doesn't intersect
	Triangle E; // Not perpendular to A, doesn't intersect
	Triangle F; // Not perpendular to A, intersects
	Triangle G; // Shared Edge

	A[0] = float3(0.0f, 0.2f,  0.0f);
	A[1] = float3(0.0f, 0.2f, -1.0f);
	A[2] = float3(1.0f, 0.2f,  0.0f);

	B[0] = float3(0.0f, 0.0f, -0.5f);
	B[1] = float3(0.0f, 1.0f, -0.5f);
	B[2] = float3(1.0f, 0.0f, -0.5f);

	C[0] = float3(0.1f, 0.2f, -0.2f);
	C[1] = float3(0.2f, 0.2f, -0.2f);
	C[2] = float3(0.2f, 0.2f, -0.1f);
	
	D[0] = float3(0.0f, 0.2f,  0.0f) + float3(5, 0, 0);
	D[1] = float3(0.0f, 0.2f, -1.0f) + float3(5, 0, 0);
	D[2] = float3(1.0f, 0.2f,  0.0f) + float3(5, 0, 0);

	E[0] = float3(0.0f, 0.0f, -0.1f);
	E[1] = float3(0.0f, 1.0f,  1.0f);
	E[2] = float3(1.0f, 0.0f, -0.1f);

	F[0] = float3(0.0f, 0.0f, -0.8f);
	F[1] = float3(0.0f, 1.0f,  0.3333f);
	F[2] = float3(1.0f, 0.0f, -0.866667f);

	G[0] = float3(0.0f, 0.2f, 0.0f);
	G[2] = float3(-1.0f, 0.2f, 0.0f);
	G[1] = float3(0.0f, 0.2f, -1.0f);

	auto res1 = Intersects(A, B); // true
	auto res2 = Intersects(A, C); // true
	auto res3 = Intersects(A, D); // false
	auto res4 = Intersects(A, E); // false
	auto res5 = Intersects(A, F); // true
	//auto res6 = Intersects(A, G); // false


	assert(res1 == true);
	assert(res2 == true);
	assert(res3 == false);
	assert(res4 == false);
	assert(res5 == true);

	const size_t batchIncrement = 4096;
	const size_t runCount = 1;

	std::vector<Triangle> triangles;
	triangles.reserve(batchIncrement << runCount);

	static const std::chrono::high_resolution_clock Clock;


	std::cout << "Benchmark setting up\n";


	for (size_t i = 0; i < batchIncrement << runCount; i++)
		triangles.emplace_back(
			Triangle{
				float3{ (float)rand(), (float)rand(), (float)rand() },
				float3{ (float)rand(), (float)rand(), (float)rand() },
				float3{ (float)rand(), (float)rand(), (float)rand() } });



	double average = 0.0f;
	size_t totalIterations = 0;

	bool res = false;

	std::cout << "Benchmark starting\n";

	for(size_t II = 0; II < 100000; II++)
	{
		for (size_t I = 0; I < runCount; I++)
		{
			float3 Sum{ 0, 0, 0 };

			const auto batchSize = batchIncrement << I;
			const auto Before = Clock.now();

			for (size_t i = 0; i < batchSize - 1; i++)
				res |= Intersects(
					triangles[(i + 0)],
					triangles[(i + 1)]);

			res = Sum.Max() > 1024;

			const auto After = Clock.now();

			typedef std::chrono::high_resolution_clock Time;
			typedef std::chrono::milliseconds ms;
			typedef std::chrono::duration<double> fsec;

			const auto Duration = std::chrono::duration_cast<std::chrono::nanoseconds>(After - Before);
			const fsec d = After - Before;

			//std::cout << "Run completed in " << Duration.count() << "ns.\n";
			//std::cout << "Iterations " << batchSize << "\n";
			//std::cout << "nanoseconds per iteration is " << Duration.count() / float(batchSize) << "\n";

			totalIterations += batchSize;
			average += Duration.count();
		}
	}

	std::cout << "iteration average timing " << average / totalIterations << "\n";

	return res;
}
