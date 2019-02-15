#ifndef THIRDPERSONCHARACTERRIG_H
#define THIRDPERSONCHARACTERRIG_H

#include "..\coreutilities\CameraUtilities.h"
#include "..\coreutilities\Components.h"
#include "..\coreutilities\GraphicsComponents.h"
#include "..\coreutilities\GraphicScene.h"


/************************************************************************************************/


namespace FlexKit
{	/************************************************************************************************/


	class ThirdPersonCameraRig
	{
	public:
		ThirdPersonCameraRig(SceneEntityHandle modelHandle, GraphicScene* scene)
		{

		}


		~ThirdPersonCameraRig()
		{

		}


		/************************************************************************************************/


		ThirdPersonCameraRig& operator = (ThirdPersonCameraRig&& rhs)
		{
			characterModel	= std::move(rhs.characterModel);
			camera			= std::move(rhs.camera);
			node			= std::move(rhs.node);
			yaw				= rhs.yaw;
			pitch			= rhs.pitch;

			return *this;
		}


		/************************************************************************************************/


		ThirdPersonCameraRig(ThirdPersonCameraRig&& rhs) : 
			characterModel	{ std::move(rhs.characterModel) },
			camera			{ std::move(rhs.camera)			},
			node			{ std::move(rhs.node)			},
			yaw				{ rhs.yaw						},
			pitch			{ rhs.pitch						} {}


		/************************************************************************************************/


		void Update(float dT)
		{
		}


		/************************************************************************************************/


		void HandleEvents(Event evt)
		{
		}


		/************************************************************************************************/


		enum Events
		{
			Forward,
			Backward,
			Left,
			Right,
			yawView,
			pitchView
		};


		DrawableBehavior	characterModel;
		CameraBehavior		camera;
		SceneNodeBehavior<>	node;

		float yaw;
		float pitch;
	};


	/************************************************************************************************/


	inline ThirdPersonCameraRig CreateCharacterRig(TriMeshHandle modelHandle, GraphicScene* scene)
	{
		ThirdPersonCameraRig rig{scene->CreateSceneEntityAndSetMesh(modelHandle), scene};

		return rig;
	}


	/************************************************************************************************/


	inline UpdateTask* UpdateThirdPersonRig(UpdateDispatcher& dipatcher, ThirdPersonCameraRig& rig, UpdateTask& transform, UpdateTask& cameras, double dT)
	{
		struct Data {
			ThirdPersonCameraRig* rig;
		};

		return &dipatcher.Add<Data>(
			[&](auto& linkageBuilder, auto& data) 
			{
				linkageBuilder.AddOutput(transform);
				linkageBuilder.AddOutput(cameras);

				data.rig = &rig;
			}, 
			[=](auto& data) 
			{
				data.rig->Update(dT);
			});
	}


}	/**********************************************************************

Copyright (c) 2019 Robert May

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

#endif