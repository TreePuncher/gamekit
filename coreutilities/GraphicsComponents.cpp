#include "GraphicScene.h"

namespace FlexKit
{
	/************************************************************************************************/


	struct CameraSystem
	{
		CameraSystem(iAllocator* IN_Memory) :
			Cameras		{IN_Memory},
			DirtyFlags	{IN_Memory},
			Memory		{IN_Memory}{}

		~CameraSystem()
		{}

		Vector<bool>	DirtyFlags;
		Vector<Camera>	Cameras;
		iAllocator*		Memory;
	}*CameraTable;


	/************************************************************************************************/


	void InitiateCameraTable(iAllocator* Memory)
	{
		if(!CameraTable)
			CameraTable = &Memory->allocate<CameraSystem>(Memory);
	}


	/************************************************************************************************/


	void ReleaseCameraTable()
	{
		FK_ASSERT(CameraTable != nullptr);

		CameraTable->~CameraSystem();
		CameraTable->Memory->free(CameraTable);

		CameraTable = nullptr;
	}


	/************************************************************************************************/


	CameraHandle CreateCamera(
		float	FOV,			
		float	AspectRatio,
		float	Near,	
		float	Far,			
		bool	Invert)
	{
		FK_ASSERT(CameraTable != nullptr);

		Camera NewCamera;
		NewCamera.FOV			= FOV;
		NewCamera.Far			= Far;
		NewCamera.Near			= Near;
		NewCamera.invert		= Invert;
		NewCamera.AspectRatio	= AspectRatio;
		NewCamera.Node			= NodeHandle{(unsigned int)-1};

		CameraTable->DirtyFlags.push_back(true);
		CameraTable->Cameras.push_back(NewCamera);

		return CameraHandle{ (unsigned int)CameraTable->Cameras.size() - 1 };
	}


	/************************************************************************************************/


	void SetCameraAspectRatio(CameraHandle Handle, float A)
	{
		FK_ASSERT(CameraTable != nullptr);
		CameraTable->Cameras[Handle].AspectRatio = A;
	}


	/************************************************************************************************/


	void MarkDirty(CameraHandle handle)
	{
		CameraTable->DirtyFlags[handle] = true;
	}


	/************************************************************************************************/


	void SetCameraNode(CameraHandle Handle, NodeHandle Node)
	{
		FK_ASSERT(CameraTable != nullptr);
		CameraTable->Cameras[Handle].Node = Node;
	}


	/************************************************************************************************/


	void SetCameraFOV(CameraHandle Handle, float f)
	{
		FK_ASSERT(CameraTable != nullptr);
		CameraTable->Cameras[Handle].FOV = f;
	}


	void SetCameraNear(CameraHandle Handle, float f)
	{
		FK_ASSERT(CameraTable != nullptr);
		CameraTable->Cameras[Handle].Near = f;
	}


	/************************************************************************************************/


	void SetCameraFar(CameraHandle Handle, float f)
	{
		FK_ASSERT(CameraTable != nullptr);
		CameraTable->Cameras[Handle].Far = f;

	}


	/************************************************************************************************/


	float GetCameraAspectRatio(CameraHandle Handle)
	{
		FK_ASSERT(CameraTable != nullptr);
		return CameraTable->Cameras[Handle].AspectRatio;
	}


	/************************************************************************************************/


	float GetCameraFar(CameraHandle Handle)
	{
		FK_ASSERT(CameraTable != nullptr);
		return CameraTable->Cameras[Handle].Far;
	}


	/************************************************************************************************/


	float GetCameraFOV(CameraHandle Handle)
	{
		FK_ASSERT(CameraTable != nullptr);
		return CameraTable->Cameras[Handle].FOV;
	}


	/************************************************************************************************/


	float GetCameraNear(CameraHandle Handle)
	{
		FK_ASSERT(CameraTable != nullptr);
		return CameraTable->Cameras[Handle].Near;
	}


	/************************************************************************************************/


	NodeHandle GetCameraNode(CameraHandle Handle)
	{
		FK_ASSERT(CameraTable != nullptr);
		return CameraTable->Cameras[Handle].Node;
	}


	/************************************************************************************************/


	float GetCameraFov(CameraHandle Handle)
	{
		FK_ASSERT(CameraTable != nullptr);
		return CameraTable->Cameras[Handle].FOV;
	}


	/************************************************************************************************/


	Camera::CameraConstantBuffer GetCameraConstants(CameraHandle Handle)
	{
		FK_ASSERT(CameraTable != nullptr);
		return CameraTable->Cameras[Handle].GetConstants();
	}


	/************************************************************************************************/


	UpdateTask& QueueCameraUpdate(UpdateDispatcher& Dispatcher, UpdateTask& TransformDependency)
	{
		struct UpdateData
		{};

		auto& task = Dispatcher.Add<UpdateData>(
			[&](auto& Builder, auto& Data)
			{
				Builder.SetDebugString("QueueCameraUpdate");
				Builder.AddInput(TransformDependency);
			},
			[](auto& Data)
			{
				FK_ASSERT(CameraTable != nullptr);
				FK_LOG_INFO("Updating Cameras");

				size_t End = CameraTable->Cameras.size();
				for (size_t I = 0; I < End; ++I)
				{
					if (CameraTable->DirtyFlags[I])
					{
						CameraTable->Cameras[I].UpdateMatrices();
						CameraTable->DirtyFlags[I] = false;
					}
				}

				return;
			}
			);

		return task;
	}


	/************************************************************************************************/
}// nemespace FlexKit


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
