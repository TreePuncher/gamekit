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

#include "Components.h"
#include "..\graphicsutilities\graphics.h"

namespace FlexKit
{

	/************************************************************************************************/

	void GameObject::AddComponent(Component C)
	{
		Components.push_back(C);
	}

	// 
	/********************************* Implentation of Transform Components *************************/
	/************************************************************************************************/
	
	void TansformComponent::Roll(float r)
	{
		FlexKit::Roll(reinterpret_cast<SceneNodes*>(ComponentSystem), static_cast<NodeHandle>(Handle) ,r);
	}

	/************************************************************************************************/

	void TansformComponent::Pitch(float r)
	{
		FlexKit::Pitch(reinterpret_cast<SceneNodes*>(ComponentSystem), static_cast<NodeHandle>(Handle) ,r);
	}

	/************************************************************************************************/

	void TansformComponent::Yaw(float r)
	{
		FlexKit::Yaw(reinterpret_cast<SceneNodes*>(ComponentSystem), static_cast<NodeHandle>(Handle) ,r);
	}

	/************************************************************************************************/

	float3 TansformComponent::GetLocalPosition()
	{
		FlexKit::LT_Entry LT(FlexKit::GetLocal(reinterpret_cast<SceneNodes*>(ComponentSystem), static_cast<NodeHandle>(Handle)));
		return LT.T;
	}

	/************************************************************************************************/

	float3 TansformComponent::GetWorldPosition()
	{
		return FlexKit::GetPositionW(reinterpret_cast<SceneNodes*>(ComponentSystem), static_cast<NodeHandle>(Handle));
	}

	/************************************************************************************************/

	void TansformComponent::SetLocalPosition(const float3& xyz)
	{
		FlexKit::LT_Entry LT(FlexKit::GetLocal(reinterpret_cast<SceneNodes*>(ComponentSystem), static_cast<NodeHandle>(Handle)));
		LT.T = xyz.pfloats;
		FlexKit::SetLocal(reinterpret_cast<SceneNodes*>(ComponentSystem), static_cast<NodeHandle>(Handle), &LT);
	}

	/************************************************************************************************/

	void TansformComponent::SetWorldPosition(const float3& xyz)
	{
		FlexKit::SetPositionW(reinterpret_cast<SceneNodes*>(ComponentSystem), static_cast<NodeHandle>(Handle), xyz);
	}

	/************************************************************************************************/

	FlexKit::Quaternion TansformComponent::GetOrientation()
	{
		Quaternion Q;
		FlexKit::GetOrientation(reinterpret_cast<SceneNodes*>(ComponentSystem), static_cast<NodeHandle>(Handle), &Q);
		return Q;
	}

	/************************************************************************************************/

	void TansformComponent::SetOrientation(FlexKit::Quaternion Q)
	{
		FlexKit::SetOrientation(reinterpret_cast<SceneNodes*>(ComponentSystem), static_cast<NodeHandle>(Handle), Q);
	}

	/************************************************************************************************/

	TansformComponent CreateTransformComponent(SceneNodes* Nodes)
	{
		TansformComponent tc;
		tc.ComponentSystem = Nodes;
		tc.Handle = GetNewNode(Nodes);

		ZeroNode(Nodes, tc.Handle);
		return tc;
	}

	/************************************************************************************************/

	void CleanupTransformComponent(TansformComponent* tc)
	{
		FreeHandle(	reinterpret_cast<SceneNodes*>(tc->ComponentSystem), tc->Handle );
	}

	/************************************************************************************************/
}