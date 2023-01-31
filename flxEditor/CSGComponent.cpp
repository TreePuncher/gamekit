#include "PCH.h"
#include "CSGComponent.h"
#include "ViewportScene.h"
#include "EditorInspectorView.h"
#include "EditorViewport.h"


/************************************************************************************************/


struct IntersectionPoints
{
	float3 A;
	float3 B;
};


float ProjectPointOntoRay(const FlexKit::float3& vector, const FlexKit::Ray& r) noexcept
{
	return (vector - r.O).dot(r.D);
}


/************************************************************************************************/


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

	return(x >= FlexKit::Min(A_0.x, A_1.x) &&
		x <= FlexKit::Max(A_0.x, A_1.x) &&
		y >= FlexKit::Min(A_0.y, A_1.y) &&
		y <= FlexKit::Max(A_0.y, A_1.y));
}


/************************************************************************************************/


bool Intersects(const float2 A_0, const float2 A_1, const float2 B_0, const float2 B_1) noexcept
{
	float2 _;
	return Intersects(A_0, A_1, B_0, B_1, _);
}


/************************************************************************************************/


bool Intersects(const Triangle& A, const Triangle& B) noexcept
{
	const auto A_n = A.Normal();
	const auto A_p = A.TriPoint();
	const auto B_n = B.Normal();
	const auto B_p = B.TriPoint();

	bool infront = false;
	bool behind = false;

	float3 distances_A;
	float3 distances_B;

	for (size_t idx = 0; idx < 3; ++idx)
	{
		const auto t1 = B[idx] - A_p;
		const auto dp1 = t1.dot(A_n);
		const auto t2 = A[idx] - B_p;
		const auto dp2 = t2.dot(B_n);

		distances_B[idx] = dp1;
		distances_A[idx] = dp2;
	}

	infront = (distances_B[0] > 0.0f) | (distances_B[1] > 0.0f) | (distances_B[2] > 0.0);
	behind = (distances_B[0] < 0.0f) | (distances_B[1] < 0.0f) | (distances_B[2] < 0.0);

	if (!(infront && behind))
		return false;

	infront = false;
	behind = false;

	infront = distances_A[0] > 0.0f | distances_A[1] > 0.0f | distances_A[2] > 0.0;
	behind = distances_A[0] < 0.0f | distances_A[1] < 0.0f | distances_A[2] < 0.0;

	if (!(infront && behind))
		return false;

	auto M_a = distances_A.Max();
	auto M_b = distances_B.Max();

	if (M_a < 0.00001f || M_b < 0.00001f)
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

		/*
		const float i[] =
		{
			FlexKit::Intersects(r[0], p),
			FlexKit::Intersects(r[1], p),
			FlexKit::Intersects(r[2], p)
		};
		*/
		size_t idx = 0;
		float d = INFINITY;

		for (size_t I = 0; I < 3; I++)
		{
			//if (!std::isnan(i[I]) && !std::isinf(i[I]))
			{
				if (i[I] < d)
				{
					idx = I;
					d = i[I];
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

		return (a_min < b_max) & (b_min < a_max);
	}
}


/************************************************************************************************/


uint32_t ExtrudeFace(uint32_t faceIdx, float z, ModifiableShape& shape)
{
	auto extrudedFace   = shape.DuplicateFace(faceIdx);
	auto normal         = shape.GetFaceNormal(extrudedFace);

	
	for (auto itr = shape.wFaces[extrudedFace].EdgeView(shape).begin(); !itr.end(); itr++)
		shape.wVertices[itr->vertices[0]].point += normal * z;


	auto itr1 = shape.wFaces[faceIdx].EdgeView(shape).begin();
	auto itr2 = shape.wFaces[extrudedFace].EdgeView(shape).begin();

	while (!itr1.end())
	{
		uint32_t points[] = {
			itr1->vertices[0],
			itr1->vertices[1],
			itr2->vertices[1],
			itr2->vertices[0],
		};

		shape.AddPolygon(points, points + 4);

		itr1++;
		itr2++;
	}

	shape.RemoveFace(faceIdx);
	shape.Build();

	return extrudedFace;
}


/************************************************************************************************/


static_vector<Vertex, 24> CreateWireframeCube(const float halfW)
{
	using FlexKit::float2;
	using FlexKit::float3;
	using FlexKit::float4;


	const static_vector<Vertex, 24> vertices = {
		// Top
		{ float4{ -halfW,  halfW,  halfW, 1 }, float4{ 1, 1, 1, 1 }, float2{ 0, 0 } },
		{ float4{  halfW,  halfW,  halfW, 1 }, float4{ 1, 1, 1, 1 }, float2{ 0, 0 } },

		{ float4{ -halfW,  halfW, -halfW, 1 }, float4{ 1, 1, 1, 1 }, float2{ 0, 0 } },
		{ float4{  halfW,  halfW, -halfW, 1 }, float4{ 1, 1, 1, 1 }, float2{ 0, 0 } },

		{ float4{ -halfW,  halfW,  halfW, 1 }, float4{ 1, 1, 1, 1 }, float2{ 0, 0 } },
		{ float4{ -halfW,  halfW, -halfW, 1 }, float4{ 1, 1, 1, 1 }, float2{ 0, 0 } },


		{ float4{  halfW,  halfW,  halfW, 1 }, float4{ 1, 1, 1, 1 }, float2{ 0, 0 } },
		{ float4{  halfW,  halfW, -halfW, 1 }, float4{ 1, 1, 1, 1 }, float2{ 0, 0 } },

		// Bottom
		{ float4{ -halfW, -halfW,  halfW, 1 }, float4{ 1, 1, 1, 1 }, float2{ 0, 0 } },
		{ float4{  halfW, -halfW,  halfW, 1 }, float4{ 1, 1, 1, 1 }, float2{ 0, 0 } },

		{ float4{ -halfW, -halfW, -halfW, 1 }, float4{ 1, 1, 1, 1 }, float2{ 0, 0 } },
		{ float4{  halfW, -halfW, -halfW, 1 }, float4{ 1, 1, 1, 1 }, float2{ 0, 0 } },

		{ float4{ -halfW, -halfW,  halfW, 1 }, float4{ 1, 1, 1, 1 }, float2{ 0, 0 } },
		{ float4{ -halfW, -halfW, -halfW, 1 }, float4{ 1, 1, 1, 1 }, float2{ 0, 0 } },

		{ float4{  halfW, -halfW,  halfW, 1 }, float4{ 1, 1, 1, 1 }, float2{ 0, 0 } },
		{ float4{  halfW, -halfW, -halfW, 1 }, float4{ 1, 1, 1, 1 }, float2{ 0, 0 } },

		// Sides
		{ float4{  halfW,  halfW,  halfW, 1 }, float4{ 1, 1, 1, 1 }, float2{ 0, 0 } },
		{ float4{  halfW, -halfW,  halfW, 1 }, float4{ 1, 1, 1, 1 }, float2{ 0, 0 } },

		{ float4{  halfW,  halfW, -halfW, 1 }, float4{ 1, 1, 1, 1 }, float2{ 0, 0 } },
		{ float4{  halfW, -halfW, -halfW, 1 }, float4{ 1, 1, 1, 1 }, float2{ 0, 0 } },


		{ float4{ -halfW,  halfW,  halfW, 1 }, float4{ 1, 1, 1, 1 }, float2{ 0, 0 } },
		{ float4{ -halfW, -halfW,  halfW, 1 }, float4{ 1, 1, 1, 1 }, float2{ 0, 0 } },


		{ float4{ -halfW,  halfW, -halfW, 1 }, float4{ 1, 1, 1, 1 }, float2{ 0, 0 } },
		{ float4{ -halfW, -halfW, -halfW, 1 }, float4{ 1, 1, 1, 1 }, float2{ 0, 0 } },
	};

	return vertices;
}


std::vector<Vertex> CreateWireframeCube2(const float halfW)
{
	using FlexKit::float2;
	using FlexKit::float3;
	using FlexKit::float4;

	const std::vector<Vertex> vertices = {
		// Top
		{ float4{ -halfW,  halfW,  halfW, 1 }, float4{ 1, 1, 1, 1 }, float2{ 0, 0 } },
		{ float4{  halfW,  halfW,  halfW, 1 }, float4{ 1, 1, 1, 1 }, float2{ 0, 0 } },

		{ float4{ -halfW,  halfW, -halfW, 1 }, float4{ 1, 1, 1, 1 }, float2{ 0, 0 } },
		{ float4{  halfW,  halfW, -halfW, 1 }, float4{ 1, 1, 1, 1 }, float2{ 0, 0 } },

		{ float4{ -halfW,  halfW,  halfW, 1 }, float4{ 1, 1, 1, 1 }, float2{ 0, 0 } },
		{ float4{ -halfW,  halfW, -halfW, 1 }, float4{ 1, 1, 1, 1 }, float2{ 0, 0 } },


		{ float4{  halfW,  halfW,  halfW, 1 }, float4{ 1, 1, 1, 1 }, float2{ 0, 0 } },
		{ float4{  halfW,  halfW, -halfW, 1 }, float4{ 1, 1, 1, 1 }, float2{ 0, 0 } },

		// Bottom
		{ float4{ -halfW, -halfW,  halfW, 1 }, float4{ 1, 1, 1, 1 }, float2{ 0, 0 } },
		{ float4{  halfW, -halfW,  halfW, 1 }, float4{ 1, 1, 1, 1 }, float2{ 0, 0 } },

		{ float4{ -halfW, -halfW, -halfW, 1 }, float4{ 1, 1, 1, 1 }, float2{ 0, 0 } },
		{ float4{  halfW, -halfW, -halfW, 1 }, float4{ 1, 1, 1, 1 }, float2{ 0, 0 } },

		{ float4{ -halfW, -halfW,  halfW, 1 }, float4{ 1, 1, 1, 1 }, float2{ 0, 0 } },
		{ float4{ -halfW, -halfW, -halfW, 1 }, float4{ 1, 1, 1, 1 }, float2{ 0, 0 } },

		{ float4{  halfW, -halfW,  halfW, 1 }, float4{ 1, 1, 1, 1 }, float2{ 0, 0 } },
		{ float4{  halfW, -halfW, -halfW, 1 }, float4{ 1, 1, 1, 1 }, float2{ 0, 0 } },

		// Sides
		{ float4{  halfW,  halfW,  halfW, 1 }, float4{ 1, 1, 1, 1 }, float2{ 0, 0 } },
		{ float4{  halfW, -halfW,  halfW, 1 }, float4{ 1, 1, 1, 1 }, float2{ 0, 0 } },

		{ float4{  halfW,  halfW, -halfW, 1 }, float4{ 1, 1, 1, 1 }, float2{ 0, 0 } },
		{ float4{  halfW, -halfW, -halfW, 1 }, float4{ 1, 1, 1, 1 }, float2{ 0, 0 } },


		{ float4{ -halfW,  halfW,  halfW, 1 }, float4{ 1, 1, 1, 1 }, float2{ 0, 0 } },
		{ float4{ -halfW, -halfW,  halfW, 1 }, float4{ 1, 1, 1, 1 }, float2{ 0, 0 } },


		{ float4{ -halfW,  halfW, -halfW, 1 }, float4{ 1, 1, 1, 1 }, float2{ 0, 0 } },
		{ float4{ -halfW, -halfW, -halfW, 1 }, float4{ 1, 1, 1, 1 }, float2{ 0, 0 } },
	};

	return vertices;
}

/************************************************************************************************/


ModifiableShape CreateCubeCSGShape() noexcept
{
	ModifiableShape cubeShape;

	const uint32_t V1 = cubeShape.AddVertex({ -1,  1,  1 }); 
	const uint32_t V2 = cubeShape.AddVertex({  1,  1,  1 }); 
	const uint32_t V3 = cubeShape.AddVertex({  1,  1, -1 }); 
	const uint32_t V4 = cubeShape.AddVertex({ -1,  1, -1 }); 

	const uint32_t V5 = cubeShape.AddVertex({ -1, -1,  1 });
	const uint32_t V6 = cubeShape.AddVertex({  1, -1,  1 });
	const uint32_t V7 = cubeShape.AddVertex({  1, -1, -1 });
	const uint32_t V8 = cubeShape.AddVertex({ -1, -1, -1 });

	uint32_t top[] = {
		V1, V2, V3, V4, V5
	};

	cubeShape.AddPolygon(top, top + 4);


	uint32_t bottom[] = {
		V8, V7, V6, V5
	};

	/*
	// Top
	cubeShape.AddTri(V1, V2, V3);
	cubeShape.AddTri(V1, V3, V4);

	// Bottom
	cubeShape.AddTri(V8, V6, V5);
	cubeShape.AddTri(V6, V8, V7);

	// Right
	cubeShape.AddTri(V3, V2, V6);
	cubeShape.AddTri(V3, V6, V7);

	// Left 
	cubeShape.AddTri(V1, V4, V5);
	cubeShape.AddTri(V5, V4, V8);

	// Rear
	cubeShape.AddTri(V2, V1, V6); // top triangle
	cubeShape.AddTri(V6, V1, V5); // bottom triangle

	// Front
	cubeShape.AddTri(V4, V7, V8);
	cubeShape.AddTri(V4, V3, V7);
	*/

	cubeShape.Build();

	return cubeShape;
}


/************************************************************************************************/


bool CSGBrush::IsLeaf() const noexcept
{
	return (left == nullptr && right == nullptr);
}


/************************************************************************************************/


FlexKit::AABB CSGBrush::GetAABB() const noexcept
{
	return shape.GetAABB(position);
}


/************************************************************************************************/


void CSGBrush::Rebuild() noexcept
{
	if (left == nullptr && right == nullptr)
		return;

	intersections.clear();
	shape.tris.clear();

	auto LHS_tris = left->shape.GetTris();
	auto RHS_tris = right->shape.GetTris();

	struct IntersectionPair
	{
		Triangle* A;
		Triangle* B;
	};

	std::vector<IntersectionPair>   intersectionPairs;
	std::vector<Triangle*>          intersectSetA;
	std::vector<Triangle*>          intersectSetB;
	std::vector<Triangle*>          finalSet;

	const auto AABB_A = left->GetAABB();

	for (auto& B : RHS_tris)
	{
		auto AABB_B = B.GetAABB().Offset(right->position);

		if (!FlexKit::Intersects(AABB_A, AABB_B))
		{
			shape.tris.push_back(B.Offset(right->position - position));
		}
		else
			intersectSetB.push_back(&B);
	}

	const auto AABB_B = right->GetAABB();

	for (auto& A : LHS_tris)
	{
		auto AABB_A = A.GetAABB().Offset(left->position);

		if (!FlexKit::Intersects(AABB_A, AABB_B))
		{
			shape.tris.push_back(A.Offset(left->position - position));
		}
		else
			intersectSetA.push_back(&A);
	}


	for (auto& A : intersectSetA)
	{
		for (auto& B : intersectSetB)
		{
			if (Intersects(A->Offset(left->position), B->Offset(right->position)))
			{
				intersectionPairs.emplace_back(A, B);

				auto A_temp = A->Offset(left->position - position);
				auto B_temp = B->Offset(right->position - position);
				intersections.emplace_back(A_temp, B_temp);
			}
			else
			{
				//shape.tris.push_back(A->Offset(left->position - position));
				//shape.tris.push_back(B->Offset(right->position - position));
			}
		}
	}


	std::sort(finalSet.begin(), finalSet.end());
	finalSet.erase(std::unique(finalSet.begin(), finalSet.end()), finalSet.end());

	//for (auto triangle : finalSet)
	//    shape.tris.push_back(triangle->Offset(-position));
}


/************************************************************************************************/


std::optional<ModifiableShape::RayCast_result> ModifiableShape::RayCast(const FlexKit::Ray& r) const noexcept
{
	if (!Intersects(r, GetAABB()))
		return {};

	float           d           = INFINITY;
	uint32_t        faceIdx     = 0;
	uint32_t        faceSubIdx  = 0;
	FlexKit::float3 hitLocation;

	for (uint32_t triIdx = 0; triIdx < wFaces.size(); triIdx++)
	{
		const auto geometry = GetFaceGeometry(triIdx);

		for(uint32_t subIdx = 0; subIdx < geometry.size(); subIdx++)
		{
			const auto tri = geometry[subIdx];

			auto [hit, distance, hit_cord] = Intersects(r, tri);
			if (hit && distance < d)
			{
				d           = distance;
				faceIdx     = triIdx;
				faceSubIdx  = subIdx;
				hitLocation = hit_cord;
			}
		}
	}

	return RayCast_result{
		.hitLocation    = hitLocation,
		.distance       = d,
		.faceIdx        = faceIdx,
		.faceSubIdx     = faceSubIdx
	};
}


std::optional<CSGBrush::RayCast_result> CSGBrush::RayCast(const FlexKit::Ray& r) const noexcept
{
	float                       d     = INFINITY;
	std::shared_ptr<CSGBrush>   brush;

	if (left)
	{
		if (auto res = FlexKit::Intersects(r, left->GetAABB()); res)
		{
			d       = res.value();
			brush   = left;
		}
	}

	if (right)
	{
		if (auto res = FlexKit::Intersects(r, right->GetAABB()); res)
		{
			if (res.value() < d)
				brush = right;
		}
	}

	if (brush)
		return brush->RayCast(r);
	else
	{
		auto localR = r;
		localR.O -= position;

		auto res = shape.RayCast(localR);

		if (res)
		{
			return RayCast_result{
				.shape              = const_cast<ModifiableShape*>(&shape),
				.faceIdx            = res->faceIdx,
				.faceSubIdx         = res->faceSubIdx,
				.distance           = res->distance,
				.BaryCentricResult  = res->hitLocation,
			};
		}
		else
			return {};
	}
}



/************************************************************************************************/


constexpr ViewportModeID CSGEditModeID = GetTypeGUID(CSGEditMode);


class CSGEditMode : public IEditorViewportMode
{
public:
	CSGEditMode(FlexKit::ImGUIIntegrator& IN_hud, CSGView& IN_selection, EditorViewport& IN_viewport) :
		hud         { IN_hud        },
		selection   { IN_selection  },
		viewport    { IN_viewport   } {}

	ViewportModeID GetModeID() const override { return CSGEditModeID; };

	enum class Mode : int
	{
		Selection,
		Manipulator
	}mode = Mode::Selection;


	void DrawImguI() final
	{
		DrawCSGMenu();

		switch (mode)
		{
		case CSGEditMode::Mode::Selection:
			DrawEditUI();
			break;
		case CSGEditMode::Mode::Manipulator:
			DrawObjectManipulatorWidget();
			break;
		default:
			break;
		}
	}

	void DrawCSGMenu()
	{
		if (ImGui::Begin("Edit CSG Menu", nullptr, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize))
		{
			ImGui::SetWindowPos({ 0, 0 });
			ImGui::SetWindowSize({ 400, 400 });

			CSG_OP op = CSG_OP::CSG_ADD;

			static const char* modeStr[] =
			{
				"Add",
				"Sub",
				"Intersection"
			};

			if (ImGui::BeginCombo("OP", modeStr[(int)op]))
			{
				if (ImGui::Selectable(modeStr[0]))
					op = CSG_OP::CSG_ADD;
				if (ImGui::Selectable(modeStr[1]))
					op = CSG_OP::CSG_SUB;
				if (ImGui::Selectable(modeStr[2]))
					op = CSG_OP::CSG_INTERSECTION;

				ImGui::EndCombo();
			}

			static const char* selectionStr[] =
			{
				"Vertex",
				"Edge",
				"Triangle",
				"Polygon",
				"Object",
				"Disabled",
			};

			if (ImGui::BeginCombo("Selection", selectionStr[(int)selectionContext.mode]))
			{
				if (ImGui::Selectable(selectionStr[0]))
					selectionContext.mode = SelectionPrimitive::Vertex;
				if (ImGui::Selectable(selectionStr[1]))
					selectionContext.mode = SelectionPrimitive::Edge;
				if (ImGui::Selectable(selectionStr[2]))
					selectionContext.mode = SelectionPrimitive::Triangle;
				if (ImGui::Selectable(selectionStr[3]))
					selectionContext.mode = SelectionPrimitive::Polygon;
				if (ImGui::Selectable(selectionStr[4]))
					selectionContext.mode = SelectionPrimitive::Object;
				if (ImGui::Selectable(selectionStr[5]))
					selectionContext.mode = SelectionPrimitive::Disabled;

				ImGui::EndCombo();
			}

			const auto selectedIdx = selection.GetData().selectedBrush;

			if (selectedIdx != -1)
			{
				auto& brushes   = selection.GetData().brushes;
				auto& brush     = brushes[selectedIdx];

				ImGui::Text(fmt::format("Tri count: {}", brush.shape.tris.size()).c_str());
			}

			if (ImGui::Button("draw normals"))
				drawNormals = !drawNormals;
		}
		ImGui::End();
	}

	void DrawEditUI()
	{
		auto& csgComponent = CSGComponent::GetComponent();

		if (selectionContext.selectedPrimitives.size())
		{
			ImGui::SetNextWindowPos({ 0, 400 });

			if (ImGui::Begin("Edit mode", nullptr, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize))
			{
				ImGui::SetWindowSize({ 400, 400 });

				ImGui::Text(fmt::format("Selection Count: {}", selectionContext.selectedPrimitives.size()).c_str());

				for (size_t I = 0; I < selectionContext.selectedPrimitives.size(); I++)
				{
					auto& primitive = selectionContext.selectedPrimitives[I];
					ImGui::Text(fmt::format("{}: [{}, {}, {}]",
						I,
						primitive.BaryCentricResult[0],
						primitive.BaryCentricResult[1],
						primitive.BaryCentricResult[2]).c_str());
				}

				ImGui::Text("debugVal1: %i", selection->debugVal1);

				if (ImGui::Button("Split"))
				{
					switch(selectionContext.mode)
					{
					case SelectionPrimitive::Edge:
					{
						auto edgeIdx = selectionContext.selectedElement;
						if (edgeIdx != -1u)
						{
							selectionContext.brush->shape.SplitEdge(edgeIdx);
							selectionContext.brush->shape.Build();
						}
					}   break;
					case SelectionPrimitive::Triangle:
					{
						auto faceIdx = selectionContext.selectedPrimitives[0].faceIdx;
						selectionContext.brush->shape.SplitTri(faceIdx, selectionContext.selectedPrimitives[0].BaryCentricResult);
					}   break;
					}
				}

				if (ImGui::Button("Rotate Edge") &&
					(selectionContext.mode == SelectionPrimitive::Edge) &&
					(selectionContext.selectedElement != -1))
				{
					selectionContext.brush->shape.RotateEdgeCCW(selectionContext.selectedElement);
					selectionContext.brush->shape.Build();
				}

				if ((selectionContext.mode == SelectionPrimitive::Polygon) && ImGui::Button("Duplicate Face"))
				{
					auto selected = selectionContext.selectedPrimitives;
					selectionContext.selectedPrimitives.clear();

					for (auto& primitive : selected)
					{
						auto face = primitive.shape->DuplicateFace(primitive.faceIdx);

						CSGBrush::RayCast_result selection;
						selection.BaryCentricResult = float3{ 1 / 3.0f };
						selection.distance          = 0.0f;
						selection.faceIdx           = face;
						selection.faceSubIdx        = 0;
						selection.shape             = primitive.shape;
						selectionContext.SetSelectedFace(selection, *selectionContext.brush);
					}
				}

				if ((selectionContext.mode == SelectionPrimitive::Polygon) && ImGui::Button("Extrude Face"))
				{
					auto selected = selectionContext.selectedPrimitives;
					selectionContext.selectedPrimitives.clear();

					for (auto& primitive : selected)
					{
						auto face = ExtrudeFace(primitive.faceIdx, 1.0f, *primitive.shape);

						CSGBrush::RayCast_result selection;
						selection.BaryCentricResult = float3{ 1 / 3.0f };
						selection.distance          = 0.0f;
						selection.faceIdx           = face;
						selection.faceSubIdx        = 0;
						selection.shape             = primitive.shape;
						selectionContext.SetSelectedFace(selection, *selectionContext.brush);
					}
				}
			}
			ImGui::End();
		}
	}

	void ObjectManipulatorMode() const
	{
		const auto selectedIdx  = selection.GetData().selectedBrush;
		auto& brushes           = selection.GetData().brushes;
		auto& brush             = brushes[selectedIdx];

		const auto camera = viewport.GetViewportCamera();
		FlexKit::CameraComponent::GetComponent().GetCamera(camera).UpdateMatrices();
		auto& cameraData = FlexKit::CameraComponent::GetComponent().GetCamera(camera);

				FlexKit::float4x4 m             = FlexKit::TranslationMatrix(brushes[selectedIdx].position);
				FlexKit::float4x4 delta         = FlexKit::float4x4::Identity();

		const   FlexKit::float4x4 view          = cameraData.View.Transpose();
		const   FlexKit::float4x4 projection    = cameraData.Proj;

		if (ImGuizmo::Manipulate(view, projection, operation, space, m, delta))
		{
			FlexKit::float3 pos;
			FlexKit::float3 rotation;
			FlexKit::float3 scale;
			ImGuizmo::DecomposeMatrixToComponents(m, pos, rotation, scale);

			brushes[selectedIdx].position   = pos;

			auto itr = brushes.begin();
			for (;itr != brushes.end();)
			{
				auto& brush_b = *itr;
				if (&brush_b != &brush && FlexKit::Intersects(brush.GetAABB(), brush_b.GetAABB()))
				{
					auto lhsBrush = std::make_shared<CSGBrush>();
					auto rhsBrush = std::make_shared<CSGBrush>();

					*lhsBrush = brush;
					*rhsBrush = brush_b;

					brush.left  = lhsBrush;
					brush.right = rhsBrush;

					brush.Rebuild();

					itr = brushes.erase(itr);
				}
				else
					itr++;
			}
		}
	}

	void EdgeManipulatorMode() const
	{
		const auto camera = viewport.GetViewportCamera();
		FlexKit::CameraComponent::GetComponent().GetCamera(camera).UpdateMatrices();
		auto& cameraData = FlexKit::CameraComponent::GetComponent().GetCamera(camera);

		auto& shape             = *selectionContext.shape;
		const auto edgeIdx      = selectionContext.selectedElement;
		auto& vertices          = shape.wEdges[edgeIdx].vertices;
		FlexKit::float3 point   = (shape.wVertices[vertices[0]].point + shape.wVertices[vertices[1]].point) / 2.0f;

				FlexKit::float4x4 m             = FlexKit::TranslationMatrix(point);
				FlexKit::float4x4 delta         = FlexKit::float4x4::Identity();

		const   FlexKit::float4x4 view          = cameraData.View.Transpose();
		const   FlexKit::float4x4 projection    = cameraData.Proj;


		switch (operation)
		{
		case ImGuizmo::OPERATION::SCALE:
		{
			static FlexKit::float3 P1{ 0 };
			static FlexKit::float3 P2{ 0 };
			static FlexKit::float3 scale{ 1, 1, 1 };

			m[0][0] = scale[0];
			m[1][1] = scale[1];
			m[2][2] = scale[2];

			if (!ImGuizmo::Manipulate(view, projection, operation, ImGuizmo::WORLD, m, delta))
			{
				P1      = shape.wVertices[vertices[0]].point;
				P2      = shape.wVertices[vertices[1]].point;
				scale   = FlexKit::float3{ 1, 1, 1 };
			}
			else
			{
				// TODO(R.M): I need a different way of handling the scaling.
				// Storage of the original edge seems to be required to do this correctly.
				FlexKit::float3 deltapos;
				FlexKit::float3 rotation;
				ImGuizmo::DecomposeMatrixToComponents(m, deltapos, rotation, scale);

				auto newPos1 = (P1 - point) * scale;
				auto newPos2 = (P2 - point) * scale;

				shape.wVertices[vertices[0]].point = newPos1 + point;
				shape.wVertices[vertices[1]].point = newPos2 + point;
				selectionContext.shape->Build();
			}
			break;
		}
		default:
			if (ImGuizmo::Manipulate(view, projection, operation, space, m, delta))
			{
				FlexKit::float4 p1{ shape.wVertices[vertices[0]].point, 1 };
				FlexKit::float4 p2{ shape.wVertices[vertices[1]].point, 1 };

				shape.wVertices[vertices[0]].point = (delta * p1).xyz();
				shape.wVertices[vertices[1]].point = (delta * p2).xyz();
				selectionContext.shape->Build();
			}   break;
		}



		ImGui::SetNextWindowPos({ 0, 400 });
		if (ImGui::Begin("Edge Manipulator", nullptr, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize))
		{
			ImGui::SetWindowSize({ 400, 400 });

			FlexKit::float3 newPos = point;
			if (ImGui::InputFloat3("XYZ", newPos))
			{
				auto deltaP = newPos - point;
				shape.wVertices[vertices[0]].point += deltaP;
				shape.wVertices[vertices[1]].point += deltaP;
				selectionContext.shape->Build();
			}
		}
		ImGui::End();
	}

	void TriangleManipulatorMode() const
	{
		const auto camera = viewport.GetViewportCamera();
		FlexKit::CameraComponent::GetComponent().GetCamera(camera).UpdateMatrices();
		auto& cameraData = FlexKit::CameraComponent::GetComponent().GetCamera(camera);

		auto& shape             = *selectionContext.shape;
		const auto& selection   = selectionContext.selectedPrimitives.front();

		auto subFace            = shape.GetSubFace(selection.faceIdx, selection.faceSubIdx);

		FlexKit::float3 point   = (
			shape.wVertices[subFace.vertices[0]].point +
			shape.wVertices[subFace.vertices[1]].point +
			shape.wVertices[subFace.vertices[2]].point) / 3.0f;

				FlexKit::float4x4 m             = FlexKit::TranslationMatrix(point);
				FlexKit::float4x4 delta         = FlexKit::float4x4::Identity();

		const   FlexKit::float4x4 view          = cameraData.View.Transpose();
		const   FlexKit::float4x4 projection    = cameraData.Proj;

		FlexKit::float3 deltapos;
		if (ImGuizmo::Manipulate(view, projection, operation, space, m, delta))
		{
			FlexKit::float3 rotation;
			FlexKit::float3 scale;
			ImGuizmo::DecomposeMatrixToComponents(delta, deltapos, rotation, scale);

			FlexKit::float4 p1{ shape.wVertices[subFace.vertices[0]].point, 1 };
			FlexKit::float4 p2{ shape.wVertices[subFace.vertices[1]].point, 1 };
			FlexKit::float4 p3{ shape.wVertices[subFace.vertices[2]].point, 1 };

			shape.wVertices[subFace.vertices[0]].point = (delta * p1).xyz();
			shape.wVertices[subFace.vertices[1]].point = (delta * p2).xyz();
			shape.wVertices[subFace.vertices[2]].point = (delta * p3).xyz();
			selectionContext.shape->Build();
		}

		ImGui::SetNextWindowPos({ 0, 400 });
		if (ImGui::Begin("Vertex Manipulator", nullptr, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize))
		{
			ImGui::SetWindowSize({ 400, 400 });

			FlexKit::float3 newPos = point;
			if (ImGui::InputFloat3("XYZ", newPos))
			{
				auto deltaP = newPos - point;
				shape.wVertices[subFace.vertices[0]].point += deltaP;
				shape.wVertices[subFace.vertices[1]].point += deltaP;
				shape.wVertices[subFace.vertices[2]].point += deltaP;
				selectionContext.shape->Build();
			}
		}
		ImGui::End();
	}

	void FaceManipulatorMode() const
	{
		const auto camera = viewport.GetViewportCamera();
		FlexKit::CameraComponent::GetComponent().GetCamera(camera).UpdateMatrices();
		auto& cameraData = FlexKit::CameraComponent::GetComponent().GetCamera(camera);

		auto& shape             = *selectionContext.shape;
		const auto& selection   = selectionContext.selectedPrimitives.front();

		FlexKit::float3 point   = shape.GetFaceCenterPoint(selection.faceIdx);

				FlexKit::float4x4 m             = FlexKit::TranslationMatrix(point);
				FlexKit::float4x4 delta         = FlexKit::float4x4::Identity();

		const   FlexKit::float4x4 view          = cameraData.View.Transpose();
		const   FlexKit::float4x4 projection    = cameraData.Proj;

		FlexKit::float3 deltapos;
		if (ImGuizmo::Manipulate(view, projection, operation, space, m, delta))
		{
			FlexKit::float3 rotation;
			FlexKit::float3 scale;
			ImGuizmo::DecomposeMatrixToComponents(delta, deltapos, rotation, scale);

			auto itr = shape.wFaces[selection.faceIdx].EdgeView(shape).begin();

			for (;!itr.end(); itr++)
			{
				FlexKit::float4 p{ shape.wVertices[itr->vertices[0]].point, 1 };
				shape.wVertices[itr->vertices[0]].point = (delta * p).xyz();
			}

			selectionContext.shape->Build();
		}

		ImGui::SetNextWindowPos({ 0, 400 });
		if (ImGui::Begin("Vertex Manipulator", nullptr, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize))
		{
			ImGui::SetWindowSize({ 400, 400 });

			FlexKit::float3 newPos = point;
			if (ImGui::InputFloat3("XYZ", newPos))
			{
				auto deltaP = newPos - point;
				auto itr    = shape.wFaces[selection.faceIdx].EdgeView(shape).begin();
				auto end    = shape.wFaces[selection.faceIdx].EdgeView(shape).end();

				for (; itr != end; itr++)
				{
					FlexKit::float4 p{ shape.wVertices[itr->vertices[0]].point, 1 };
					shape.wVertices[itr->vertices[0]].point += deltaP;
				}

				selectionContext.shape->Build();
			}
		}
		ImGui::End();
	}

	void VertexManipulatorMode() const
	{
		const auto camera = viewport.GetViewportCamera();
		FlexKit::CameraComponent::GetComponent().GetCamera(camera).UpdateMatrices();
		auto& cameraData = FlexKit::CameraComponent::GetComponent().GetCamera(camera);

		const auto vertexIdx    = selectionContext.selectedElement;
		float3 point            = selectionContext.shape->wVertices[vertexIdx].point;

				FlexKit::float4x4 m             = FlexKit::TranslationMatrix(point);
				FlexKit::float4x4 delta         = FlexKit::float4x4::Identity();

		const   FlexKit::float4x4 view          = cameraData.View.Transpose();
		const   FlexKit::float4x4 projection    = cameraData.Proj;

		if (ImGuizmo::Manipulate(view, projection, operation, space, m, delta))
		{
			FlexKit::float3 rotation;
			FlexKit::float3 scale;
			ImGuizmo::DecomposeMatrixToComponents(m, point, rotation, scale);

			selectionContext.shape->wVertices[vertexIdx].point = point;
			selectionContext.shape->Build();
		}

		ImGui::SetNextWindowPos({ 0, 400 });
		if (ImGui::Begin("Vertex Manipulator", nullptr, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize))
		{
			ImGui::SetWindowSize({ 400, 400 });

			if (ImGui::InputFloat3("XYZ", point))
			{
				selectionContext.shape->wVertices[vertexIdx].point = point;
				selectionContext.shape->Build();
			}
		}
		ImGui::End();
	}

	void DrawObjectManipulatorWidget() const
	{
		auto drawNoSelectionWindow =
			[]
			{
				ImGui::SetNextWindowPos({ 0, 400 });
				if (ImGui::Begin("Manipulator", nullptr, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize))
				{
					ImGui::SetWindowSize({ 400, 400 });
					ImGui::Text("Nothing Selected");
				}

				ImGui::End();
			};


		if (selectionContext.selectedElement == -1)
			return drawNoSelectionWindow();

		const auto selectedIdx = selection.GetData().selectedBrush;
		if (selectedIdx == -1)
			return drawNoSelectionWindow();

		auto& brushes = selection.GetData().brushes;
		auto& brush = brushes[selectedIdx];

		if (brushes.size() < selectedIdx)
			return;

		ImGuiIO& io = ImGui::GetIO();
		ImGuizmo::SetRect(0, 0, io.DisplaySize.x, io.DisplaySize.y);

		switch (selectionContext.mode)
		{
		case SelectionPrimitive::Edge:
			return EdgeManipulatorMode();
		case SelectionPrimitive::Vertex:
			return VertexManipulatorMode();
		case SelectionPrimitive::Triangle:
			return TriangleManipulatorMode();
		case SelectionPrimitive::Polygon:
			return FaceManipulatorMode();
		case SelectionPrimitive::Object:
			ObjectManipulatorMode();
			return;
		default:
			break;
		}
	}

	void Draw(FlexKit::UpdateDispatcher& dispatcher, FlexKit::FrameGraph& frameGraph, TemporaryBuffers& temps, FlexKit::ResourceHandle renderTarget, FlexKit::ResourceHandle depthBuffer) final
	{
		struct DrawHUD
		{
			FlexKit::ReserveVertexBufferFunction    ReserveVertexBuffer;
			FlexKit::ReserveConstantBufferFunction  ReserveConstantBuffer;
			FlexKit::FrameResourceHandle            renderTarget;
			FlexKit::FrameResourceHandle            depthBuffer;
		};


		if(selection.GetData().brushes.size())
		{
			frameGraph.AddNode<DrawHUD>(
				DrawHUD{
					temps.ReserveVertexBuffer,
					temps.ReserveConstantBuffer
				},
				[&](FlexKit::FrameGraphNodeBuilder& builder, DrawHUD& data)
				{
					data.renderTarget   = builder.RenderTarget(renderTarget);
					data.depthBuffer    = builder.DepthTarget(depthBuffer);
				},
				[&](DrawHUD& data, FlexKit::ResourceHandler& resources, FlexKit::Context& ctx, auto& allocator)
				{
					ctx.BeginEvent_DEBUG("Draw CSG HUD");

					const auto& brushes = selection.GetData().brushes;

					// Setup state
					FlexKit::DescriptorHeap descHeap;
					descHeap.Init(
						ctx,
						resources.renderSystem().Library.RS6CBVs4SRVs.GetDescHeap(0),
						&allocator);
					descHeap.NullFill(ctx);

					ctx.SetRootSignature(resources.renderSystem().Library.RS6CBVs4SRVs);
					ctx.SetPipelineState(resources.GetPipelineState(FlexKit::DRAW_TRI3D_PSO));

					ctx.SetScissorAndViewports({ resources.GetResource(data.renderTarget) });
					ctx.SetRenderTargets({ resources.GetResource(data.renderTarget) }, true, resources.GetResource(data.depthBuffer));

					ctx.SetPrimitiveTopology(FlexKit::EInputTopology::EIT_TRIANGLE);

					ctx.SetGraphicsDescriptorTable(0, descHeap);

					ctx.NullGraphicsConstantBufferView(3);
					ctx.NullGraphicsConstantBufferView(4);
					ctx.NullGraphicsConstantBufferView(5);
					ctx.NullGraphicsConstantBufferView(6);


					struct DrawConstants
					{
						FlexKit::float4     unused1;
						FlexKit::float4     unused2;
						FlexKit::float4x4   transform;
					};

					auto constantBuffer = data.ReserveConstantBuffer(
						FlexKit::AlignedSize<FlexKit::Camera::ConstantBuffer>() +
						FlexKit::AlignedSize<DrawConstants>() * brushes.size());

					FlexKit::ConstantBufferDataSet cameraConstants{ FlexKit::GetCameraConstants(viewport.GetViewportCamera()), constantBuffer };
					ctx.SetGraphicsConstantBufferView(1, cameraConstants);

					const size_t selectedNodeIdx    = selection.GetData().selectedBrush;
					FlexKit::VBPushBuffer TBBuffer  = data.ReserveVertexBuffer(sizeof(Vertex) * 1024 * brushes.size());
					FlexKit::VBPushBuffer LBBuffer  = data.ReserveVertexBuffer(sizeof(Vertex) * 1024 * brushes.size());

					size_t idx = 0;

					static const FlexKit::float4 colors[] =
					{
						{ 185 / 255.0f, 131 / 255.0f, 255 / 255.0f, 1.0f },
						{ 148 / 255.0f, 179 / 255.0f, 253 / 255.0f, 1.0f },
						{ 148 / 255.0f, 218 / 255.0f, 255 / 255.0f, 1.0f },
						{ 153 / 255.0f, 254 / 255.0f, 255 / 255.0f, 1.0f },

						{ 137 / 255.0f, 181 / 255.0f, 175 / 255.0f, 1.0f },
						{ 150 / 255.0f, 199 / 255.0f, 193 / 255.0f, 1.0f },
						{ 222 / 255.0f, 217 / 255.0f, 196 / 255.0f, 1.0f },
						{ 208 / 255.0f, 202 / 255.0f, 178 / 255.0f, 1.0f },
					};


					for (const auto& brush : brushes)
					{
						const auto aabb     = brush.GetAABB();
						const float r       = aabb.Dim()[aabb.LongestAxis()];
						const auto offset   = aabb.MidPoint();

						std::vector<Vertex> verts;

						for (size_t faceIdx = 0; faceIdx < brush.shape.wFaces.size(); faceIdx++)
						{
							auto& face = brush.shape.wFaces[faceIdx];
							FlexKit::float4 Color = colors[faceIdx % 8];


							Vertex v;
							v.UV = FlexKit::float2(1, 1);
							auto temp = brush.shape.GetFaceGeometry(faceIdx);

							for (size_t triIdx = 0; triIdx < temp.size(); triIdx++)
							{
								auto& vertex = temp[triIdx];

								v.Color = Color;

								if (selectionContext.selectedPrimitives.size())
								{
									switch (selectionContext.mode)
									{
									case  SelectionPrimitive::Polygon:
										if (selectionContext.selectedPrimitives[0].faceIdx == faceIdx)
											v.Color = FlexKit::float4{ 0.8, 0, 0, 1 };
										else
										break;
									case  SelectionPrimitive::Triangle:
										if (selectionContext.selectedPrimitives[0].faceIdx == faceIdx &&
											selectionContext.selectedPrimitives[0].faceSubIdx == triIdx)
											v.Color = FlexKit::float4{ 0.8, 0, 0, 1 };
										break;
									}
								}

								for (size_t idx = 0; idx < 3; idx++)
								{
									v.Position = FlexKit::float4(vertex[idx], 1);
									verts.emplace_back(v);
								}
							}

						}

						for (auto& intersection : brush.intersections)
						{
							Vertex v;
							v.Color     = FlexKit::float4(1, 0, 0, 1);
							v.UV        = FlexKit::float2(1, 1);

							for (size_t idx = 0; idx < 3; idx++)
							{
								v.Position = FlexKit::float4(intersection.A[idx], 1);
								verts.emplace_back(v);
							}

							for (size_t idx = 0; idx < 3; idx++)
							{
								v.Position = FlexKit::float4(intersection.B[idx], 1);
								verts.emplace_back(v);
							}
						}

						const FlexKit::VertexBufferDataSet vbDataSet{
							verts,
							TBBuffer };

						ctx.SetVertexBuffers({ vbDataSet });

						const DrawConstants CB_Data = {
							.unused1    = FlexKit::float4{ 1, 1, 1, 1 },
							.unused2    = FlexKit::float4{ 1, 1, 1, 1 },
							.transform  = FlexKit::TranslationMatrix(brush.position)
						};

						const FlexKit::ConstantBufferDataSet constants{ CB_Data, constantBuffer };

						ctx.SetGraphicsConstantBufferView(2, constants);

						ctx.Draw(verts.size());

						++idx;

					}


					for (const auto& brush : brushes)
					{
						ctx.SetPipelineState(resources.GetPipelineState(FlexKit::DRAW_LINE3D_PSO));
						ctx.SetPrimitiveTopology(FlexKit::EInputTopology::EIT_LINE);

						Vertex v;
						v.Color = FlexKit::float4{ 0, 0, 0, 0 };
						v.UV    = FlexKit::float2{ 1, 1 };

						auto& shape = brush.shape;

						std::vector<Vertex> verts;

						for (uint32_t faceIdx = 0; faceIdx < shape.wFaces.size(); faceIdx++)
						{
							const auto& face        = shape.wFaces[faceIdx];

							auto edgeIdx            = face.edgeStart;

							if (edgeIdx == -1)
								continue;

							const auto faceNormal = shape.GetFaceNormal(faceIdx);

							for(size_t i = 0; i < 3; i++)
							{
								const ModifiableShape::wEdge& edge = shape.wEdges[edgeIdx];

								v.Position = FlexKit::float4{ shape.wVertices[edge.vertices[0]].point + faceNormal * 0.001f, 1 };
								verts.emplace_back(v);

								v.Position = FlexKit::float4{ shape.wVertices[edge.vertices[1]].point + faceNormal * 0.001f, 1 };
								verts.emplace_back(v);

								edgeIdx = edge.next;
							}

							if (drawNormals)
							{
								const auto geometry = shape.GetFaceGeometry(faceIdx);
								for(auto triangle : geometry)
								{
									const auto tri      = triangle.Offset(brush.position);
									const auto A        = triangle.TriPoint();
									const auto B        = A + triangle.Normal();

									verts.push_back(
										Vertex{
											.Position   = A,
											.Color      = FlexKit::float4{ 1, 1, 1, 1 },
											.UV         = FlexKit::float2{ 0, 0 }
										});

									verts.push_back(
										Vertex{
											.Position   = B,
											.Color      = FlexKit::float4{ 1, 1, 1, 1 },
											.UV         = FlexKit::float2{ 1, 1 }
										});
								}
							}
						}

						const FlexKit::VertexBufferDataSet vbDataSet{ verts, TBBuffer };

						ctx.SetVertexBuffers({ vbDataSet });
						ctx.Draw(verts.size());
					}

					ctx.EndEvent_DEBUG();
				}
			);
		}
	}

	void RayCast(const FlexKit::Ray& r, auto OnHit)
	{
		size_t selectedIdx  = -1;
		float  minDistance  = 10000.0f;

		const CSGBrush*             hitBrush;
		CSGBrush::RayCast_result    hitResult;

		for (const auto& brush : selection.GetData().brushes)
		{
			const auto AABB = brush.GetAABB();
			const auto res = FlexKit::Intersects(r, AABB);
			if (res && res.value() < minDistance && res.value() > 0.0f)
			{
				if (auto res = brush.RayCast(r); res)
				{
					if (res->distance < minDistance)
					{
						minDistance = res->distance;
						hitResult   = res.value();
						hitBrush    = &brush;
					}
				}
			}
		}

		if (minDistance < 10'000.0f)
			OnHit(hitResult, *hitBrush);
	}

	void EdgeSelectionMode(QMouseEvent* evt)
	{
		const auto r = viewport.GetMouseRay();

		RayCast(
			r,
			[&](CSGBrush::RayCast_result& hit, const CSGBrush& brush)
			{
				const auto point_BC     = hit.BaryCentricResult;
				uint32_t selectedEdge   = -1;
				
				auto subFace = hit.shape->GetSubFace(hit.faceIdx, hit.faceSubIdx);

				if (FlexKit::Min(point_BC.x, 0.5f) + FlexKit::Min(point_BC.y, 0.5f) > 0.75f)
					selectedEdge = subFace.edges[0];
				else if (point_BC.y + point_BC.z > 0.75f)
					selectedEdge = subFace.edges[1];
				else if (point_BC.x + point_BC.z > 0.75f)
					selectedEdge = subFace.edges[2];

				selectionContext.SetSelectedEdge(
					hit,
					const_cast<CSGBrush&>(brush),
					const_cast<ModifiableShape&>(brush.shape),
					selectedEdge);
			});
	}

	void TriangleSelectionMode(QMouseEvent* evt)
	{
		const auto r = viewport.GetMouseRay();

		RayCast(
			r,
			[&](CSGBrush::RayCast_result& hit, const CSGBrush& brush)
			{
				selectionContext.SetSelectedTriangle(
					hit,
					const_cast<CSGBrush&>(brush),
					const_cast<ModifiableShape&>(brush.shape));
			});
	}

	void VertexSelectionMode(QMouseEvent* evt)
	{
		const auto r = viewport.GetMouseRay();

		RayCast(
			r,
			[&](CSGBrush::RayCast_result& hit, const CSGBrush& brush)
			{
				const auto point_BC = hit.BaryCentricResult;
				uint32_t faceVertex = -1;

				if (point_BC.x > 0.65)
					faceVertex = 0;
				else if (point_BC.y > 0.65)
					faceVertex = 1;
				else if (point_BC.z > 0.65)
					faceVertex = 2;

				if (faceVertex == -1u)
					return;

				auto selectedVertex = hit.shape->GetVertexFromFaceLocalIdx(hit.faceIdx, hit.faceSubIdx, faceVertex);

				selectionContext.SetSelectedVertex(
					hit,
					const_cast<CSGBrush&>(brush),
					const_cast<ModifiableShape&>(brush.shape),
					selectedVertex);
			});
	}

	void FaceSelectionMode(QMouseEvent* evt)
	{
		const auto r = viewport.GetMouseRay();

		RayCast(
			r,
			[&](CSGBrush::RayCast_result& hit, const CSGBrush& brush)
			{
				selectionContext.SetSelectedFace(
					hit,
					const_cast<CSGBrush&>(brush));
			});
	}

	bool ImGUI_MousePress(QMouseEvent* evt)
	{
		if(ImGui::IsAnyItemHovered() || mode == CSGEditMode::Mode::Manipulator)
		{
			if (evt->button() == Qt::MouseButton::LeftButton)
			{
				previousMousePosition = FlexKit::int2{ -160000, -160000 };

				FlexKit::Event mouseEvent;
				mouseEvent.InputSource  = FlexKit::Event::Mouse;
				mouseEvent.Action       = FlexKit::Event::Pressed;
				mouseEvent.mType        = FlexKit::Event::Input;

				mouseEvent.mData1.mKC[0] = FlexKit::KC_MOUSELEFT;
				hud.HandleInput(mouseEvent);
			}
			else if (evt->button() == Qt::MouseButton::RightButton)
			{
				previousMousePosition = FlexKit::int2{ -160000, -160000 };

				FlexKit::Event mouseEvent;
				mouseEvent.InputSource  = FlexKit::Event::Mouse;
				mouseEvent.Action       = FlexKit::Event::Pressed;
				mouseEvent.mType        = FlexKit::Event::Input;

				mouseEvent.mData1.mKC[0] = FlexKit::KC_MOUSERIGHT;
				hud.HandleInput(mouseEvent);
			}
			return true;
		}
		else return false;
	}

	bool ImGUI_MouseRelease(QMouseEvent* evt)
	{
		if (ImGui::IsAnyItemHovered() || mode == CSGEditMode::Mode::Manipulator)
		{
			if (evt->button() == Qt::MouseButton::LeftButton)
			{
				previousMousePosition = FlexKit::int2{ -160000, -160000 };

				FlexKit::Event mouseEvent;
				mouseEvent.InputSource  = FlexKit::Event::Mouse;
				mouseEvent.Action       = FlexKit::Event::Release;
				mouseEvent.mType        = FlexKit::Event::Input;

				mouseEvent.mData1.mKC[0] = FlexKit::KC_MOUSELEFT;
				hud.HandleInput(mouseEvent);
			}
			else if (evt->button() == Qt::MouseButton::RightButton)
			{
				previousMousePosition = FlexKit::int2{ -160000, -160000 };

				FlexKit::Event mouseEvent;
				mouseEvent.InputSource  = FlexKit::Event::Mouse;
				mouseEvent.Action       = FlexKit::Event::Release;
				mouseEvent.mType        = FlexKit::Event::Input;

				mouseEvent.mData1.mKC[0] = FlexKit::KC_MOUSERIGHT;
				hud.HandleInput(mouseEvent);
			}
			return true;
		} else return false;
	}

	void mousePressEvent(QMouseEvent* evt) final 
	{
		if(!ImGUI_MousePress(evt))
		{
			switch (mode)
			{
			case CSGEditMode::Mode::Selection:
			{
				switch (selectionContext.mode)
				{
				case SelectionPrimitive::Vertex:
				{
					VertexSelectionMode(evt);
				}   break;
				case SelectionPrimitive::Edge:
				{
					EdgeSelectionMode(evt);
				}   break;
				case SelectionPrimitive::Triangle:
				{
					TriangleSelectionMode(evt);
				}   break;
				case SelectionPrimitive::Polygon:
				{
					FaceSelectionMode(evt);
				}   break;
				default:
					break;
				}

			}   break;
			case CSGEditMode::Mode::Manipulator:
				break;
			default:
				break;
			}
		}
	}

	void mouseReleaseEvent(QMouseEvent* evt) final 
	{

		if(!ImGUI_MouseRelease(evt))
		{
			switch (mode)
			{
			case CSGEditMode::Mode::Selection:
			{
			}   break;
			case CSGEditMode::Mode::Manipulator:
				break;
			default:
				break;
			}
		}
	}

	void keyPressEvent(QKeyEvent* evt) final
	{
		switch (evt->key())
		{
		case Qt::Key_1:
			selectionContext.mode = SelectionPrimitive::Vertex;
			break;
		case Qt::Key_2:
			selectionContext.mode = SelectionPrimitive::Edge;
			break;
		case Qt::Key_3:
			selectionContext.mode = SelectionPrimitive::Triangle;
			break;
		case Qt::Key_4:
			selectionContext.mode = SelectionPrimitive::Polygon;
			break;
		case Qt::Key_5:
			selectionContext.mode = SelectionPrimitive::Object;
			break;
		case Qt::Key_W:
			if (operation == ImGuizmo::OPERATION::TRANSLATE)
				mode        = (mode != Mode::Manipulator) ? mode = Mode::Manipulator : mode = Mode::Selection;

			operation   = ImGuizmo::OPERATION::TRANSLATE;
			break;
		case Qt::Key_E:
			if(operation == ImGuizmo::OPERATION::SCALE)
				mode        = (mode != Mode::Manipulator) ? mode = Mode::Manipulator : mode = Mode::Selection;

			operation   = ImGuizmo::OPERATION::SCALE;
			break;
		case Qt::Key_R:
			if (operation == ImGuizmo::OPERATION::ROTATE)
				mode        = (mode != Mode::Manipulator) ? mode = Mode::Manipulator : mode = Mode::Selection;

			operation   = ImGuizmo::OPERATION::ROTATE;
			break;
		case Qt::Key_T:
		{
			if(mode == Mode::Manipulator)
				space = space == ImGuizmo::MODE::WORLD ? ImGuizmo::MODE::LOCAL: ImGuizmo::MODE::WORLD;
		}   break;
		default:
		{
			ImGuiIO& io = ImGui::GetIO();
			auto str = evt->text().toStdString();

			for (auto c : str) {
				io.AddInputCharacter(c);
				io.KeysDown[c] = true;
			}
		}   break;
		}
	}

	void keyReleaseEvent(QKeyEvent* evt) final
	{
		ImGuiIO& io = ImGui::GetIO();
		auto str = evt->text().toStdString();

		for (auto c : str)
			io.KeysDown[c] = false;
	}


	enum class SelectionPrimitive
	{
		Vertex,
		Edge,
		Triangle,
		Polygon,
		Object,
		Disabled
	};

	struct CSGSelectionContext
	{
		SelectionPrimitive mode = SelectionPrimitive::Disabled;

		CSGBrush*   brush           = nullptr;
		ModifiableShape*   shape           = nullptr;
		uint32_t selectedElement    = -1;

		std::vector<CSGBrush::RayCast_result> selectedPrimitives;

		void SetSelectedEdge(CSGBrush::RayCast_result& hit, CSGBrush& IN_brush, ModifiableShape& IN_shape, const uint32_t edgeIdx)
		{
			selectedPrimitives.clear();

			selectedPrimitives.push_back(hit);
			selectedElement     = edgeIdx;
			mode                = SelectionPrimitive::Edge;
			brush               = const_cast<CSGBrush*>(&IN_brush);
			shape               = const_cast<ModifiableShape*>(&IN_shape);
		}

		void SetSelectedTriangle(CSGBrush::RayCast_result& hit, CSGBrush& IN_brush, ModifiableShape& IN_shape)
		{
			selectedPrimitives.clear();

			selectedPrimitives.push_back(hit);
			selectedElement = hit.faceIdx;
			mode            = SelectionPrimitive::Triangle;
			brush           = const_cast<CSGBrush*>(&IN_brush);
			shape           = const_cast<ModifiableShape*>(&IN_shape);
		}

		void SetSelectedVertex(CSGBrush::RayCast_result& hit, CSGBrush& IN_brush, ModifiableShape& IN_shape, const uint32_t vertexIdx)
		{
			selectedPrimitives.clear();

			selectedPrimitives.push_back(hit);
			selectedElement = vertexIdx;
			mode            = SelectionPrimitive::Vertex;
			brush           = const_cast<CSGBrush*>(&IN_brush);
			shape           = const_cast<ModifiableShape*>(&IN_shape);
		}

		void SetSelectedFace(CSGBrush::RayCast_result& hit, CSGBrush& IN_brush, bool add = false)
		{
			if(!add)
				selectedPrimitives.clear();
			
			selectedPrimitives.push_back(hit);
			selectedElement = hit.faceIdx;
			mode            = SelectionPrimitive::Polygon;
			brush           = const_cast<CSGBrush*>(&IN_brush);
			shape           = const_cast<ModifiableShape*>(hit.shape);
		}

		void GetSelectionUIGeometry(std::vector<Vertex>& verts) const
		{
			if (!shape)
				return;

			switch (mode)
			{
			case SelectionPrimitive::Vertex:
			{
			}   break;
			case SelectionPrimitive::Edge:
			{
			}   break;
			case SelectionPrimitive::Triangle:
			{
				for (auto& primitive : selectedPrimitives)
				{
					const auto geometry = primitive.shape->GetFaceGeometry(primitive.faceIdx);
					auto tri = geometry[selectedPrimitives.front().faceSubIdx].Offset(brush->position);

					tri = tri.Offset(tri.Normal() * 0.001f + brush->position);

					Vertex v;
					v.Color = FlexKit::float4(1, 0, 0, 1);
					v.UV    = FlexKit::float2(1, 1);

					for (size_t idx = 0; idx < 3; idx++)
					{
						v.Position = FlexKit::float4(tri[idx], 1);
						verts.emplace_back(v);
					}
				}
			}   break;
			case SelectionPrimitive::Polygon:
			{
				for (auto& primitive : selectedPrimitives)
				{
					const auto geometry = primitive.shape->GetFaceGeometry(primitive.faceIdx);
					auto n              = primitive.shape->GetFaceNormal(primitive.faceIdx);

					for (auto tri : geometry)
					{
						tri = tri.Offset(n * 0.001f + brush->position);

						Vertex v;
						v.Color = FlexKit::float4(1, 0, 0, 1);
						v.UV = FlexKit::float2(1, 1);

						for (size_t idx = 0; idx < 3; idx++)
						{
							v.Position = FlexKit::float4(tri[idx], 1);
							verts.emplace_back(v);
						}
					}
				}
			}   break;
			}
		}
	}selectionContext;

	ImGuizmo::MODE      space       = ImGuizmo::MODE::WORLD;
	ImGuizmo::OPERATION operation   = ImGuizmo::OPERATION::TRANSLATE;

	bool drawNormals = false;

	EditorViewport&             viewport;
	CSGView&                    selection;
	FlexKit::ImGUIIntegrator&   hud;
	FlexKit::int2               previousMousePosition = FlexKit::int2{ -160000, -160000 };
};


/************************************************************************************************/


constexpr ViewportModeID CSGSliceModeID = GetTypeGUID(CSGEditMode);


class CSGSliceMode : public IEditorViewportMode
{
public:
	CSGSliceMode(CSGEditMode& IN_CSGMode, FlexKit::ImGUIIntegrator& IN_hud, CSGView& IN_selection, EditorViewport& IN_viewport) :
		CSGMode     { IN_CSGMode    },
		hud         { IN_hud        },
		selection   { IN_selection  },
		viewport    { IN_viewport   } {}

	ViewportModeID GetModeID() const override { return CSGSliceModeID; };

	void Draw(FlexKit::UpdateDispatcher& dispatcher, FlexKit::FrameGraph& frameGraph, TemporaryBuffers& temps, FlexKit::ResourceHandle renderTarget, FlexKit::ResourceHandle depthBuffer) final
	{

	}

	void mousePressEvent(QMouseEvent* evt) final
	{
	}

	void keyPressEvent(QKeyEvent* evt) final
	{
		CSGMode.keyPressEvent(evt);
	}

	void keyReleaseEvent(QKeyEvent* evt) final
	{
		CSGMode.keyReleaseEvent(evt);
	}

	CSGEditMode&                CSGMode;
	EditorViewport&             viewport;
	CSGView&                    selection;
	FlexKit::ImGUIIntegrator&   hud;
	FlexKit::int2               previousMousePosition = FlexKit::int2{ -160000, -160000 };
};

/************************************************************************************************/


void UpdateCSGComponent(CSGComponentData& brushComponent)
{
	auto itr1 = brushComponent.brushes.begin();
	auto itr2 = brushComponent.brushes.begin();

	for (; itr1 != brushComponent.brushes.end();)
	{
		for (; itr2 != brushComponent.brushes.end();)
		{
			auto& brush_b = *itr2;
			if (itr2 != itr1 && FlexKit::Intersects(itr1->GetAABB(), itr2->GetAABB()))
			{
				auto lhsBrush = std::make_shared<CSGBrush>();
				auto rhsBrush = std::make_shared<CSGBrush>();

				*lhsBrush = *itr1;
				*rhsBrush = *itr2;

				itr1->left = lhsBrush;
				itr1->right = rhsBrush;

				itr1->Rebuild();

				itr2 = brushComponent.brushes.erase(itr2);
			}
			else
				itr2++;
		}
	}
}


/************************************************************************************************/


class CSGInspector : public IComponentInspector
{
public:
	CSGInspector(EditorViewport& IN_viewport) :
		viewport{ IN_viewport } {}

	FlexKit::ComponentID ComponentID() override
	{
		return CSGComponentID;
	}

	std::string ComponentName() const noexcept final
	{
		return "Constructive Solid Geometry";
	}

	void Inspect(ComponentViewPanelContext& panelCtx, FlexKit::GameObject& gameObject, FlexKit::ComponentViewBase& view) override
	{
		CSGView& csgView = static_cast<CSGView&>(view);

		panelCtx.AddButton(
			"Edit",
			[&]
			{
				viewport.PushMode(std::make_shared<CSGEditMode>(viewport.GetHUD(), csgView, viewport));
			});

		panelCtx.AddButton(
			"Add Brush",
			[&]
			{
				CSGBrush newBrush;
				newBrush.op         = CSG_OP::CSG_ADD;
				newBrush.shape      = CreateCubeCSGShape();
				newBrush.position   = FlexKit::float3{ 0, 0, 0 };
				newBrush.dirty      = true;

				csgView->brushes.push_back(newBrush);
				csgView->selectedBrush = csgView->brushes.size() - 1;
			});

		panelCtx.AddButton(
			"Remove",
			[&]
			{
				if (csgView->selectedBrush != -1)
				{
					csgView->brushes.erase(csgView->brushes.begin() + csgView->selectedBrush);
					csgView->selectedBrush = -1;
				}
			});

		panelCtx.AddButton(
			"Test",
			[&]
			{
				auto& csg = csgView.GetData();

				if(csg.brushes.size() == 0)
				{
					CSGBrush brush1;
					brush1.op       = CSG_OP::CSG_ADD;
					brush1.shape    = CreateCubeCSGShape();
					brush1.position = FlexKit::float3{ 0 };
					brush1.shape.Build();

					csg.brushes.emplace_back(std::move(brush1));
					csg.selectedBrush = -1;
				}

				ModifiableShape& shape = csg.brushes.back().shape;
				shape = CreateCubeCSGShape();

				const auto end = shape.wFaces.size();
				for (uint32_t faceIdx = 0; faceIdx < end; faceIdx++)
					shape.SplitTri(faceIdx);

				shape.Build();

				csg.debugVal1++;
			});

		panelCtx.AddList(
			[&]() -> size_t
			{    // Update Size
				return csgView.GetData().brushes.size();
			}, 
			[&](auto idx, QListWidgetItem* item)
			{   // Update Contents
				auto op = csgView.GetData().brushes[idx].op;

				std::string label = op == CSG_OP::CSG_ADD ? "Add" :
									op == CSG_OP::CSG_SUB ? "Sub" : "";

				item->setData(Qt::DisplayRole, label.c_str());
			},             
			[&](QListWidget* listWidget)
			{   // On Event
				listWidget->setSelectionMode(QAbstractItemView::SelectionMode::SingleSelection);
				auto idx = listWidget->indexFromItem(listWidget->currentItem()).row();
				csgView.GetData().selectedBrush = idx;
			}              
		);

		panelCtx.Pop();
	}

	EditorViewport& viewport;
};


/************************************************************************************************/


FlexKit::Blob EditorComponentCSG::GetBlob()
{
	FlexKit::SaveArchiveContext archive;

	Serialize(archive);

	return archive.GetBlob();
}


/************************************************************************************************/


void CSGComponentEventHandler::OnCreateView(FlexKit::GameObject& gameObject, FlexKit::ValueMap values, const std::byte* buffer, const size_t bufferSize, iAllocator* allocator)
{
	std::vector<CSGBrush>   brushes;

	FlexKit::Blob blob{ (const char*)buffer, bufferSize };
	FlexKit::LoadBlobArchiveContext archive(blob);

	auto& csgView = !gameObject.hasView(CSGComponentID) ? gameObject.AddView<CSGView>() : static_cast<CSGView&>(*gameObject.GetView(CSGComponentID));

	EditorComponentCSG csgComponent;
	archive& csgComponent;

	csgView->brushes = std::move(csgComponent.brushes);
}

struct CSGComponentFactory : public IComponentFactory
{
	FlexKit::ComponentViewBase& Construct(FlexKit::GameObject& gameObject, ComponentConstructionContext& ctx)
	{
		return gameObject.AddView<CSGView>();
	}

	inline static const std::string name = "CSG";

	const std::string&      ComponentName() const noexcept { return name; }
	FlexKit::ComponentID    ComponentID() const noexcept { return CSGComponentID; }

	static bool Register()
	{
		EditorInspectorView::AddComponentFactory(std::make_unique<CSGComponentFactory>());

		return true;
	}

	static void Update(FlexKit::EntityComponent& component, FlexKit::ComponentViewBase& base, ViewportSceneContext& scene)
	{
		auto& editorCSGComponent    = static_cast<EditorComponentCSG&>(component);
		auto& csgView               = static_cast<CSGView&>(base);

		editorCSGComponent.brushes  = csgView->brushes;
	}

	inline static IEntityComponentRuntimeUpdater::RegisterConstructorHelper<CSGComponentFactory, CSGComponentID> _register;

	inline static bool _registered = Register();
};


/************************************************************************************************/


void RegisterCSGInspector(EditorViewport& viewport)
{
	EditorInspectorView::AddComponentInspector<CSGInspector>(viewport);
}


/**********************************************************************

Copyright (c) 2022 Robert May

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
