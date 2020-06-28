#include "intersection.h"
#include "graphics.h"

namespace FlexKit
{	/************************************************************************************************/


	bool CompareBSAgainstFrustum(const Frustum* F, const BoundingSphere BS)
	{
		bool Bottom = false;
		bool Near	= false;
		bool Far	= false;
		bool Left	= false;
		bool Right	= false;
		bool Top	= false;
		const float3 V = BS.xyz();
		const float  r = BS.w;

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


	/************************************************************************************************/


	Frustum GetFrustum(
		const float AspectRatio, 
		const float FOV, 
		const float Near, 
		const float Far, 
		float3 Position, 
		Quaternion Q)
	{
		float3 FTL(0);
		float3 FTR(0);
		float3 FBL(0);
		float3 FBR(0);

		FTL.z = -Far;
		FTL.y = tan(FOV) * Far;
		FTL.x = -FTL.y * AspectRatio;

		FTR ={ -FTL.x,  FTL.y, FTL.z };
		FBL ={  FTL.x, -FTL.y, FTL.z };
		FBR ={ -FTL.x, -FTL.y, FTL.z };

		float3 NTL(0);
		float3 NTR(0);
		float3 NBL(0);
		float3 NBR(0);

		NTL.z = -Near;
		NTL.y = tan(FOV / 2) * Near;
		NTL.x = NTL.y * AspectRatio;

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


	/************************************************************************************************/


	Frustum GetSubFrustum(
		const float AspectRatio,
		const float FOV,
		const float Near,
		const float Far,
		float3		Position,
		Quaternion	Q,
		float2		TopLeft,
		float2		BottomRight)
	{
		float3 FTL(0);
		float3 FTR(0);
		float3 FBL(0);
		float3 FBR(0);

		float3 NTL(0);
		float3 NTR(0);
		float3 NBL(0);
		float3 NBR(0);

		{
			float3 FTL_FULL(0);
			float3 FTR_FULL(0);
			float3 FBL_FULL(0);
			float3 FBR_FULL(0);

			FTL_FULL.z = -Far;
			FTL_FULL.y = tan(FOV) * Far;
			FTL_FULL.x = -FTL_FULL.y * AspectRatio;

			FTR_FULL = { -FTL_FULL.x,  FTL_FULL.y, FTL_FULL.z };
			FBL_FULL = {  FTL_FULL.x, -FTL_FULL.y, FTL_FULL.z };
			FBR_FULL = { -FTL_FULL.x, -FTL_FULL.y, FTL_FULL.z };

			float3 NTL_FULL(0);
			float3 NTR_FULL(0);
			float3 NBL_FULL(0);
			float3 NBR_FULL(0);

			NTL_FULL.z = -Near;
			NTL_FULL.y = tan(FOV / 2) * Near;
			NTL_FULL.x = NTL_FULL.y * AspectRatio;

			NTR_FULL = { -NTL_FULL.x,  NTL_FULL.y, NTL_FULL.z };
			NBL_FULL = { -NTL_FULL.x, -NTL_FULL.y, NTL_FULL.z };
			NBR_FULL = {  NTL_FULL.x, -NTL_FULL.y, NTL_FULL.z };

			FTL.x = lerp(FTL_FULL.x, FTR_FULL.x, TopLeft.x);
			FTL.y = lerp(FTL_FULL.y, FBR_FULL.y, TopLeft.y);
			FTL.z = -Far;

			FTR.x = lerp(FTL_FULL.x, FTR_FULL.x, BottomRight.x);
			FTR.y = lerp(FTL_FULL.y, FBR_FULL.y, TopLeft.y);
			FTR.z = -Far;

			NTL.x = lerp(NTL_FULL.x, NTR_FULL.x, TopLeft.x);
			NTL.y = lerp(NTL_FULL.y, NBR_FULL.y, TopLeft.y);
			NTL.z = -Near;

			NTR.x = lerp(NTL_FULL.x, NTR_FULL.x, BottomRight.x);
			NTR.y = lerp(NTL_FULL.y, NBR_FULL.y, TopLeft.y);
			NTR.z = -Near;

			FBL.x = lerp(FBL_FULL.x, FBR_FULL.x, TopLeft.x);
			FBL.y = lerp(FTL_FULL.y, FBR_FULL.y, BottomRight.y);
			FBL.z = -Far;

			FBR.x = lerp(FBL_FULL.x, FBR_FULL.x, BottomRight.x);
			FBR.y = lerp(FTL_FULL.y, FBR_FULL.y, BottomRight.y);
			FBR.z = -Far;

			NBL.x = lerp(NTL_FULL.x, NTR_FULL.x, TopLeft.x);
			NBL.y = lerp(NTL_FULL.y, NBL_FULL.y, BottomRight.y);
			NBL.z = -Near;

			NBR.x = lerp(NTL_FULL.x, NTR_FULL.x, BottomRight.x);
			NBR.y = lerp(NTR_FULL.y, NBR_FULL.y, BottomRight.y);
			NBR.z = -Near;
		}

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

			Out.Planes[EPlane_BOTTOM].Orgin	 = (FBL + NBR) / 2;
			Out.Planes[EPlane_BOTTOM].Normal = N1.cross(N2).normal();
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


/**********************************************************************

Copyright (c) 2014-2018 Robert May

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
