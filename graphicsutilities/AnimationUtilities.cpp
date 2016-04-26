/**********************************************************************

Copyright (c) 2015 - 2016 Robert May

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

#include "..\PCH.h"
#include "..\buildsettings.h"
#include "AnimationUtilities.h"

namespace FlexKit
{
	/************************************************************************************************/

	Joint&	Skeleton::operator [] (JointHandle hndl){ return Joints[hndl]; }

	/************************************************************************************************/


	JointHandle Skeleton::AddJoint(Joint J, XMMATRIX& I)
	{
		Joints		[JointCount] = J;
		IPose		[JointCount] = I;

		auto Temp = GetInversePose(J.mParent) * DirectX::XMMatrixInverse(nullptr, I);
		auto Pose = GetPose(Temp);
		JointPoses[JointCount] = Pose;

		return JointCount++;
	}


	/************************************************************************************************/


	XMMATRIX Skeleton::GetInversePose(JointHandle H)
	{
		return (H != 0XFFFF)? IPose[H] : DirectX::XMMatrixIdentity();
	}

	EntityPoseState* CreatePoseState(Entity* E, FlexKit::BlockAllocator* MEM)
	{
		using DirectX::XMMATRIX;
		using DirectX::XMMatrixIdentity;

		if (!E->Mesh && !E->Mesh->Skeleton)
			return nullptr;

		size_t JointCount = E->Mesh->Skeleton->JointCount;

		auto New_EAS = (EntityPoseState*)MEM->_aligned_malloc(sizeof(EntityPoseState));
		New_EAS->ShaderResource = nullptr;
		New_EAS->StateBuffer	= nullptr;
		New_EAS->Joints			= (XMMATRIX*)MEM->_aligned_malloc(sizeof(XMMATRIX) * JointCount, 0x40);
		New_EAS->CurrentPose	= (XMMATRIX*)MEM->_aligned_malloc(sizeof(XMMATRIX) * JointCount, 0x40);
		New_EAS->JointCount		= JointCount;
		New_EAS->Sk				= E->Mesh->Skeleton;

		for (size_t I = 0; I < JointCount; ++I)
			New_EAS->Joints[I] = XMMatrixIdentity();

		return New_EAS;
	}


	/************************************************************************************************/


	bool InitiatePoseState(RenderSystem* RS, EntityPoseState* EAS, PoseState_DESC& Desc, VShaderJoint* M)
	{
		D3D11_SUBRESOURCE_DATA	SR;
		SR.pSysMem = M;

		ID3D11Buffer* Buffer = nullptr;
		D3D11_BUFFER_DESC B_DESC;
		B_DESC.BindFlags			= D3D11_BIND_SHADER_RESOURCE;
		B_DESC.ByteWidth			= Desc.JointCount * sizeof(VShaderJoint);
		B_DESC.CPUAccessFlags		= D3D11_CPU_ACCESS_WRITE;
		B_DESC.MiscFlags			= D3D11_RESOURCE_MISC_FLAG::D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
		B_DESC.StructureByteStride	= sizeof(VShaderJoint);
		B_DESC.Usage				= D3D11_USAGE_DYNAMIC;

		FK_ASSERT(0);
		//auto RES = RS->pDevice->CreateBuffer(&B_DESC, &SR, &Buffer);
		//FK_ASSERT(SUCCEEDED(RES), "BUFFER FAILED TO CREATE!");

		ID3D11ShaderResourceView* SRV = nullptr;
		D3D11_SHADER_RESOURCE_VIEW_DESC	SRV_DESC = {};
		SRV_DESC.Format					= DXGI_FORMAT::DXGI_FORMAT_UNKNOWN;
		SRV_DESC.ViewDimension			= D3D11_SRV_DIMENSION::D3D11_SRV_DIMENSION_BUFFER;
		SRV_DESC.Buffer.ElementOffset	= 0;
		SRV_DESC.Buffer.ElementWidth	= Desc.JointCount;
		//RES = RS->pDevice->CreateShaderResourceView(Buffer, &SRV_DESC, &SRV);
		//FK_ASSERT(SUCCEEDED(RES), "SRV FAILED TO CREATE!");
	
		EAS->ShaderResource = SRV;
		EAS->StateBuffer	= Buffer;

		return true;
	}


	/************************************************************************************************/


	void Destroy(EntityPoseState* EPS)
	{
		if(EPS->ShaderResource) EPS->ShaderResource->Release();
		if(EPS->StateBuffer)	EPS->StateBuffer->Release();

		EPS->ShaderResource = nullptr;
		EPS->StateBuffer	= nullptr;
	}


	/************************************************************************************************/


	void Skeleton::InitiateSkeleton(BlockAllocator* Allocator, size_t JC)
	{
		auto test = sizeof(Skeleton);
		Animations	= nullptr;
		JointCount	= 0;
		Joints		= (Joint*)		Allocator->_aligned_malloc(sizeof(Joint)	 * JC, 0x40);
		IPose		= (XMMATRIX*)	Allocator->_aligned_malloc(sizeof(XMMATRIX)  * JC, 0x40); // Inverse Global Space Pose
		JointPoses	= (JointPose*)	Allocator->_aligned_malloc(sizeof(JointPose) * JC, 0x40); // Local Space Pose
		FK_ASSERT(Joints);

		for (auto I = 0; I < JointCount; I++)
		{
			IPose[I]			= DirectX::XMMatrixIdentity();
			Joints[I].mID		= nullptr;
			Joints[I].mParent	= JointHandle();
		}
	}

	void Skeleton::InitiateSkeleton(StackAllocator* Allocator, size_t JC)
	{
		auto test = sizeof(Skeleton);
		Animations	= nullptr;
		JointCount	= 0;
		Joints		= (Joint*)		Allocator->_aligned_malloc(sizeof(Joint)	 * JC, 0x40);
		IPose		= (XMMATRIX*)	Allocator->_aligned_malloc(sizeof(XMMATRIX)  * JC, 0x40); // Inverse Global Space Pose
		JointPoses	= (JointPose*)	Allocator->_aligned_malloc(sizeof(JointPose) * JC, 0x40); // Local Space Pose
		FK_ASSERT(Joints);

		for (auto I = 0; I < JointCount; I++)
		{
			IPose[I]			= DirectX::XMMatrixIdentity();
			Joints[I].mID		= nullptr;
			Joints[I].mParent	= JointHandle();
		}
	}


	/************************************************************************************************/


	JointHandle Skeleton::FindJoint(const char* ID)
	{
		for (size_t I = 0; I < JointCount; ++I)
		{
			if (!strcmp(Joints[I].mID, ID))
				return I;
		}

		return -1;
	}

	/************************************************************************************************/


	void Skeleton_PushAnimation(Skeleton* S, StackAllocator* Allocator, AnimationClip AC)
	{
		auto& AL	= Allocator->allocate_aligned<Skeleton::AnimationList>();
		AL.Next		= nullptr;
		AL.Clip		= AC;

		auto I		= &S->Animations;

		while (*I != nullptr)
			I = &(*I)->Next;

		(*I) = &AL;
	}

	void Skeleton_PushAnimation(Skeleton* S, BlockAllocator* Allocator, AnimationClip AC)
	{
		auto& AL	= Allocator->allocate_aligned<Skeleton::AnimationList>();
		AL.Next		= nullptr;
		AL.Clip		= AC;

		auto I		= &S->Animations;

		while (*I != nullptr)
			I = &(*I)->Next;

		(*I) = &AL;
	}


	/************************************************************************************************/


	EPLAY_ANIMATION_RES PlayAnimation(FlexKit::Entity* E, const char* Animation, FlexKit::BlockAllocator* MEM, bool ForceLoop)
	{
		using FlexKit::EntityPoseState;
		if (!E || !E->Mesh)
			return EPLAY_ANIMATION_RES::EPLAY_INVALID_PARAM;
		if (!E->Mesh->Skeleton)
			return EPLAY_ANIMATION_RES::EPLAY_NOT_ANIMATABLE;

		if (!E->PoseState)
		{
			Skeleton*	S		= (Skeleton*)E->Mesh->Skeleton;
			auto NewPoseState	= FlexKit::CreatePoseState(E, MEM);

			if (!NewPoseState)
				return EPLAY_NOT_ANIMATABLE;

			E->PoseState = NewPoseState;
		}// Create Animation State

		if (!E->AnimationState)
		{
			auto New_EAS = (EntityAnimationState*)MEM->_aligned_malloc(sizeof(EntityAnimationState));
			E->AnimationState	= New_EAS;
			New_EAS->Clips		={};
		}

		auto EPS	= E->PoseState;
		auto EAS	= E->AnimationState;
		auto S		= (Skeleton*)E->Mesh->Skeleton;

		Skeleton::AnimationList* I = S->Animations;

		while (I)
		{
			if (!strcmp(I->Clip.mID, Animation))
			{
				EntityAnimationState::AnimationStateEntry AS;
				AS.Clip		 = &I->Clip;
				AS.T		 = 0;
				AS.Playing	 = true;
				AS.Speed	 = 1.0f;
				AS.ForceLoop = ForceLoop;
				EAS->Clips.push_back(AS);

				E->Posed = true; // Enable Posing or Animation won't do anything
				return EPLAY_ANIMATION_RES::EPLAY_SUCCESS;
			}

			I = I->Next;
		}

		return EPLAY_ANIMATION_RES::EPLAY_ANIMATION_NOT_FOUND;
	}


	/************************************************************************************************/


	EPLAY_ANIMATION_RES SetAnimationSpeed(EntityAnimationState* AE, const char* AnimationID, double Speed)
	{
		using FlexKit::EntityPoseState;

		for(auto& C : AE->Clips)
		{
			if (!strcmp(C.Clip->mID, AnimationID))
			{
				C.Speed = Speed;
				return EPLAY_ANIMATION_RES::EPLAY_SUCCESS;
			}
		}
		return EPLAY_ANIMATION_RES::EPLAY_ANIMATION_NOT_FOUND;
	}


	/************************************************************************************************/


	EPLAY_ANIMATION_RES StopAnimation(FlexKit::Entity* E, const char* Animation)
	{
		if (!E)
			return EPLAY_ANIMATION_RES::EPLAY_INVALID_PARAM;
		if (!E->Mesh | !E->Mesh->Skeleton)
			return EPLAY_ANIMATION_RES::EPLAY_NOT_ANIMATABLE;

		if (!E->Posed)
		{
			return EPLAY_ANIMATION_RES::EPLAY_SUCCESS;
		}

		auto EAS	= E->AnimationState;
		auto S		= E->Mesh->Skeleton;
		
		for (auto& C : EAS->Clips)
		{
			if (!strcmp(C.Clip->mID, Animation)) {
				C.Playing = false;
				return EPLAY_ANIMATION_RES::EPLAY_SUCCESS;
			}
		}
		return EPLAY_ANIMATION_RES::EPLAY_SUCCESS;
	}


	/************************************************************************************************/


	void UpdateAnimation(RenderSystem* RS, FlexKit::Entity* E, double dT, StackAllocator* TEMP)
	{
		using DirectX::XMMatrixIdentity;
		using DirectX::XMMatrixInverse;

		if (E && E->Posed)
		{
			if (!E->PoseState || !E->AnimationState){
				E->Posed = false;
				return;
			}

			auto PS = E->PoseState;
			auto AS = E->AnimationState;
			auto S  = E->Mesh->Skeleton;

			size_t BufferSize = S->JointCount * sizeof(DirectX::XMMATRIX);
			DirectX::XMMATRIX*	M    = (DirectX::XMMATRIX*)	TEMP->_aligned_malloc(BufferSize);
			VShaderJoint*		Out  = (VShaderJoint*)		TEMP->_aligned_malloc(sizeof(VShaderJoint) * BufferSize);

			for (size_t I = 0; I < S->JointCount; ++I)
				M[I]  = PS->Joints[I] * GetTransform(S->JointPoses + I);

			bool AnimationPlayed = false;
			for (auto& C : AS->Clips)
			{
				if (C.Playing && C.Clip->FrameCount)
				{
					auto Clip					= C.Clip;
					auto T						= C.T;
					auto Loop					= Clip->isLooping;
					double FrameDuration		=  (1.0f / Clip->FPS)/C.Speed;
					double AnimationDuration	=  FrameDuration * Clip->FrameCount;
					size_t FrameIndex			= size_t(T / FrameDuration) % Clip->FrameCount;
					auto& CurrentFrame			= Clip->Frames[FrameIndex];

					if (!(Loop | C.ForceLoop) && C.T > AnimationDuration)
						C.Playing = false;
					else if(C.T > AnimationDuration)
						C.T = 0.0f;
					else {
						C.T += dT;
					}

					for (size_t I = 0; I < CurrentFrame.JointCount; ++I) 
						M[I] = M[I] * XMMatrixInverse(nullptr, GetTransform(S->JointPoses + I)) * GetTransform(CurrentFrame.Poses + I);
						//M[I] = M[I] * GetTransform(CurrentFrame.Poses + I);
					
					AnimationPlayed = true;
				}

			}

			if (AnimationPlayed || PS->Dirty)
			{
				for (size_t I = 0; I < S->JointCount; ++I)
				{
					auto temp = GetTransform(&S->JointPoses[I]);
					auto P = (S->Joints[I].mParent != 0xFFFF) ? M[S->Joints[I].mParent] : XMMatrixIdentity();
					M[I]   = M [I] * P;
					Out[I] = {S->IPose[I], M[I]};
				}
				
				PS->Dirty = false;
				
				if (!PS->ShaderResource | !PS->StateBuffer)
				{	// Create Animation State with Data as Initial
					FlexKit::PoseState_DESC	Desc ={ S->JointCount };
					auto RES = FlexKit::InitiatePoseState(RS, PS, Desc, Out);
					FK_ASSERT(RES, "ANIMATION STATE FAILED TO CREATE!");
				} else {
					FK_ASSERT(0);
					//FlexKit::MapWriteDiscard(RS, (char*)Out, sizeof(VShaderJoint) * S->JointCount, PS->StateBuffer);
				}
			}
		}
	}


	/************************************************************************************************/


	DirectX::XMMATRIX GetTransform(JointPose* P)
	{
		return	DirectX::XMMatrixRotationQuaternion(P->r) *
			DirectX::XMMatrixScalingFromVector(DirectX::XMVectorSet(P->ts[3], P->ts[3], P->ts[3], 0.0f)) *
			DirectX::XMMatrixTranslationFromVector(DirectX::XMVectorSet(P->ts[0], P->ts[1], P->ts[2], 0.0f));
	}

	JointPose GetPose(DirectX::XMMATRIX& M)
	{
		auto Q = DirectX::XMQuaternionRotationMatrix(M);
		auto P = M.r[3];

		return{ Q, float4(P, 1) };
	}

	void PrintChildren(FlexKit::Skeleton* S, JointHandle J, size_t Depth = 0)
	{
		for (size_t II = 0; II < S->JointCount; II++)
		if (S->Joints[II].mParent == J)
		{
			for (size_t I = 0; I < Depth; I++)
				std::cout << "    ";

			std::cout << S->Joints[II].mID << " -- Children: \n";
			PrintChildren(S, II, Depth + 1);
		}
	}

	void PrintSkeletonHierarchy(FlexKit::Skeleton* S)
	{
		for (size_t I = 0; I < S->JointCount; I++)
		{
			if (S->Joints[I].mParent == 0XFFFF)
			{
				std::cout << S->Joints[I].mID << " -- Children: \n";
				PrintChildren(S, I, 1);
			}
		}
	}

	/************************************************************************************************/
}