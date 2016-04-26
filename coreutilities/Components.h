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

#include "..\buildsettings.h"
#include "..\coreutilities\containers.h"
#include "..\coreutilities\Handle.h"
#include "..\coreutilities\MathUtils.h"

namespace FlexKit
{
	// GOAL: Help reduce code involved to do shit which is currently excessive 
	/************************************************************************************************/

	// Components store all data needed for a component
	struct Component
	{
		FlexKit::Handle_t<16>	Handle;
		void*					ComponentSystem; // Component Use Only

		enum ComponentType
		{
			Transform,
			Renderable,
			Collider
		}Type;

	};

	/************************************************************************************************/
	// Component Derivatives 
	// only define Methods
	// All data is fetched using handle and System Pointer
	struct FLEXKITAPI TansformComponent : public Component
	{
		void Roll	(float r);
		void Pitch	(float r);
		void Yaw	(float r);

		float3 GetLocalPosition();
		float3 GetWorldPosition();

		void SetLocalPosition(const float3&);
		void SetWorldPosition(const float3&);

		FlexKit::Quaternion GetOrientation();
		void				SetOrientation(FlexKit::Quaternion Q);
	};

	struct SceneNodes;
	TansformComponent	CreateTransformComponent	(SceneNodes*);
	void				CleanupTransformComponent	(TansformComponent* tc);

	/************************************************************************************************/

	struct FLEXKITAPI GameObject
	{
		static_vector<Component, 3> Components;

		void		AddComponent(Component C);

		Component*	FindComponent(Component::ComponentType T)
		{	
			for(auto& C : Components)
				if (C.Type == T)
					return &C;
			return nullptr;
		}

		template<typename Ty_>
		Ty_*				GetComponent()
		{	
			// Unknown Type
			return nullptr;
		}

		template<>
		TansformComponent*	GetComponent()
		{	
#ifdef _DEBUG
			auto _ptr = FindComponent(Component::Transform);
			FK_ASSERT(_ptr);
			return static_cast<TansformComponent*>(_ptr);
#else
			return static_cast<TansformComponent*>(FindComponent(Component::Transform));
#endif
		}
	};

	/************************************************************************************************/
}