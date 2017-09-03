#include "..\coreutilities\intersection.h"
#include "..\graphicsutilities\graphics.h"

namespace FlexKit
{
	bool CompareBSAgainstFrustum(Frustum* F, BoundingSphere BS)
	{
		bool Bottom = false;
		bool Near	= false;
		bool Far	= false;
		bool Left	= false;
		bool Right	= false;
		bool Top	= false;
		float3 V = BS.xyz();
		float  r = BS.w;

		{
			auto P = V - F->Planes[EPlane_FAR].Orgin;
			auto N = F->Planes[EPlane_FAR].Normal;

			float NdP	= N.dot(P);
			float D		= NdP - r;
			Far			= D <= 0;
		}
		{
			auto P = V - F->Planes[EPlane_NEAR].Orgin;
			auto N = F->Planes[EPlane_NEAR].Normal;

			float NdP	= N.dot(P);
			float D		= NdP - r;
			Near		= D <= 0;
		}
		{
			auto P = V - F->Planes[EPlane_TOP].Orgin;
			auto N = F->Planes[EPlane_TOP].Normal;

			float NdP	= N.dot(P);
			float D		= NdP - r;
			Top			= D <= 0;
		}
		{
			auto P = V - F->Planes[EPlane_BOTTOM].Orgin;
			auto N = F->Planes[EPlane_BOTTOM].Normal;

			float NdP	= N.dot(P);
			float D		= NdP - r;
			Bottom		= D <= 0;
		}
		{
			auto P = V - F->Planes[EPlane_LEFT].Orgin;
			auto N = F->Planes[EPlane_LEFT].Normal;

			float NdP	= N.dot(P);
			float D		= NdP - r;
			Left		= D <= 0;
			int x = 0; 
		}
		{
			auto P = V - F->Planes[EPlane_RIGHT].Orgin;
			auto N = F->Planes[EPlane_RIGHT].Normal;

			float NdP	= N.dot(P);
			float D		= NdP - r;
			Right		= D <= 0;
			int x = 0;
		}

		return (Bottom & Near & Far & Left & Right & Top);
	}

	Frustum GetFrustum(Camera* C, float3 Position, Quaternion Q)
	{
		float3 FTL(0);
		float3 FTR(0);
		float3 FBL(0);
		float3 FBR(0);

		FTL.z = -C->Far;
		FTL.y = tan(C->FOV) * C->Far;
		FTL.x = -FTL.y * C->AspectRatio;

		FTR ={ -FTL.x,  FTL.y, FTL.z };
		FBL ={ FTL.x, -FTL.y, FTL.z };
		FBR ={ -FTL.x, -FTL.y, FTL.z };

		float3 NTL(0);
		float3 NTR(0);
		float3 NBL(0);
		float3 NBR(0);

		NTL.z = -C->Near;
		NTL.y = tan(C->FOV / 2) * C->Near;
		NTL.x = NTL.y * C->AspectRatio;

		NTR ={ -NTL.x,  NTL.y, NTL.z };
		NBL ={ -NTL.x, -NTL.y, NTL.z };
		NBR ={ NTL.x, -NTL.y, NTL.z };

		FTL = Position + Q * FTL;
		FTR = Position + Q * FTR;
		FBL = Position + Q * FBL;
		FBR = Position + Q * FBR;

		NTL = Position + Q * NTL;
		NTR = Position + Q * NTR;
		NBL = Position + Q * NBL;
		NBR = Position + Q * NBR;

		Frustum Out;

		{
			float3 N1 = DirectionVector(FTL, FTR);
			float3 N2 = DirectionVector(FTL, FBL);

			Out.Planes[EPlane_FAR].Orgin	= (FBL + FTR) / 2;
			Out.Planes[EPlane_FAR].Normal	= N1.cross(N2).normal();
		}
		{
			Out.Planes[EPlane_NEAR].Orgin	= (NBL + NTR) / 2;
			Out.Planes[EPlane_NEAR].Normal	= -Out.Planes[EPlane_FAR].Normal;
		}
		{
			float3 N1 = DirectionVector(FTL, FTR);
			float3 N2 = DirectionVector(FTL, NTL);

			Out.Planes[EPlane_TOP].Orgin	= ((FTL + FTR) / 2 + (NTL + NTR) / 2) / 2;
			Out.Planes[EPlane_TOP].Normal	= -N1.cross(N2).normal();
		}
		{
			float3 N1 = DirectionVector(FBL, FBR);
			float3 N2 = DirectionVector(FBL, NBL);

			Out.Planes[EPlane_BOTTOM].Orgin	    = (FBL + NBR) / 2;
			Out.Planes[EPlane_BOTTOM].Normal	= N1.cross(N2).normal();
		}
		{
			float3 N1 = DirectionVector(FTL, FBL);
			float3 N2 = DirectionVector(FTL, NTL);

			Out.Planes[EPlane_LEFT].Orgin	= ((FTL + FBL) / 2 + (NTL + NBL) / 2) / 2;
			Out.Planes[EPlane_LEFT].Normal	= N1.cross(N2).normal();
		}
		{
			float3 N1 = DirectionVector(FBR, FTR);
			float3 N2 = DirectionVector(FTR, NTR);

			Out.Planes[EPlane_RIGHT].Orgin	= ((FTR + FBR) / 2 + (NTR + NBR) / 2) / 2;
			Out.Planes[EPlane_RIGHT].Normal	= N1.cross(N2).normal();
		}
		return Out;
	}

}