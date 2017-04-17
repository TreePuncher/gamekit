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

#include "..\graphicsutilities\graphics.h"
#include "GraphicsComponents.h"

namespace FlexKit
{
	/********************************* Implentation of Transform Components *************************/
	/************************************************************************************************/

	void SceneNodeComponentSystem::InitiateSystem(byte* Memory, size_t BufferSize)
	{
		InitiateSceneNodeBuffer(&Nodes, Memory, BufferSize);
		Root = GetZeroedNode(Nodes);
	}

	void SceneNodeComponentSystem::ReleaseHandle(ComponentHandle Handle)
	{
		FlexKit::ReleaseNode(Nodes, Handle);
	}

	NodeHandle SceneNodeComponentSystem::GetRoot() 
	{ 
		return Root; 
	}

	NodeHandle SceneNodeComponentSystem::GetNewNode() 
	{ 
		return FlexKit::GetNewNode(Nodes); 
	}


	void TansformComponent::Roll(float r)
	{
		FlexKit::Roll(*static_cast<SceneNodeComponentSystem*>(ComponentSystem), static_cast<NodeHandle>(ComponentHandle), r);
	}


	/************************************************************************************************/


	void TansformComponent::Pitch(float r)
	{
		FlexKit::Pitch(*static_cast<SceneNodeComponentSystem*>(ComponentSystem), static_cast<NodeHandle>(ComponentHandle), r);
	}


	/************************************************************************************************/


	void TansformComponent::Yaw(float r)
	{
		FlexKit::Yaw(*static_cast<SceneNodeComponentSystem*>(ComponentSystem), static_cast<NodeHandle>(ComponentHandle), r);
	}


	/************************************************************************************************/


	float3 TansformComponent::GetLocalPosition()
	{
		FlexKit::LT_Entry LT(FlexKit::GetLocal(*static_cast<SceneNodeComponentSystem*>(ComponentSystem), static_cast<NodeHandle>(ComponentHandle)));
		return LT.T;
	}


	/************************************************************************************************/


	float3 TansformComponent::GetWorldPosition()
	{
		return FlexKit::GetPositionW(*static_cast<SceneNodeComponentSystem*>(ComponentSystem), static_cast<NodeHandle>(ComponentHandle));
	}


	/************************************************************************************************/


	void TansformComponent::SetLocalPosition(const float3& xyz)
	{
		FlexKit::LT_Entry LT(FlexKit::GetLocal(*static_cast<SceneNodeComponentSystem*>(ComponentSystem), static_cast<NodeHandle>(ComponentHandle)));
		LT.T = xyz.pfloats;
		FlexKit::SetLocal(*static_cast<SceneNodeComponentSystem*>(ComponentSystem), static_cast<NodeHandle>(ComponentHandle), &LT);
	}


	/************************************************************************************************/


	void TansformComponent::SetWorldPosition(const float3& xyz)
	{
		FlexKit::SetPositionW(*static_cast<SceneNodeComponentSystem*>(ComponentSystem), static_cast<NodeHandle>(ComponentHandle), xyz);
	}


	/************************************************************************************************/


	FlexKit::Quaternion TansformComponent::GetOrientation()
	{
		Quaternion Q;
		FlexKit::GetOrientation(*static_cast<SceneNodeComponentSystem*>(ComponentSystem), static_cast<NodeHandle>(ComponentHandle), &Q);
		return Q;
	}


	/************************************************************************************************/


	void TansformComponent::SetOrientation(FlexKit::Quaternion Q)
	{
		FlexKit::SetOrientation(*static_cast<SceneNodeComponentSystem*>(ComponentSystem), static_cast<NodeHandle>(ComponentHandle), Q);
	}


	/************************************************************************************************/
}