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
#include "..\graphicsutilities\graphics.h"
#include "..\coreutilities\GraphicScene.h"
#include "..\coreutilities\containers.h"
#include "..\coreutilities\Handle.h"
#include "..\coreutilities\MathUtils.h"

#ifndef COMPONENT_H
#define COMPONENT_H

namespace FlexKit
{
	// GOAL: Help reduce code involved to do shit which is currently excessive 
	/************************************************************************************************/

	// Components store all data needed for a component
	typedef FlexKit::Handle_t<16> ComponentHandle;
	typedef size_t EventTypeID;

	enum ComponentType : uint16_t
	{
		CT_Transform,
		CT_Renderable,
		CT_Collider,
		CT_Player,
		CT_Unknown
	};

	class ComponentSystemInterface
	{
	public:
		virtual void ReleaseHandle	(ComponentHandle Handle) = 0;
	};

	struct Component
	{
		typedef void FN_RELEASECOMPONENT(Component* _ptr);

		Component()
		{
			ComponentSystem		= nullptr;
			ReleaseComponent	= nullptr;
			Type				= ComponentType::CT_Unknown;
		}


		Component(
			ComponentSystemInterface*	CS,
			FN_RELEASECOMPONENT*		RC,
			FlexKit::Handle_t<16>		CH,
			ComponentType				T
		) :
			ComponentSystem		(CS),
			ReleaseComponent	(RC),
			ComponentHandle		(CH),
			Type				(T)	{}


		Component& operator = (Component&& RHS)
		{
			ComponentSystem			= RHS.ComponentSystem;
			ReleaseComponent		= RHS.ReleaseComponent;
			ComponentHandle			= RHS.ComponentHandle;
			Type					= RHS.Type;
			RHS.ReleaseComponent	= 0;

			return *this;
		}


		Component(const Component& RValue)
		{
			ComponentHandle		= RValue.ComponentHandle;
			ReleaseComponent	= RValue.ReleaseComponent;
			ComponentHandle		= RValue.ComponentHandle;
			Type				= RValue.Type;
		}


		~Component()
		{
			if (ReleaseComponent)
				ReleaseComponent(this);
		}


		ComponentSystemInterface*	ComponentSystem;
		FN_RELEASECOMPONENT*		ReleaseComponent;
		FlexKit::Handle_t<16>		ComponentHandle;
		ComponentType				Type;
	};


	/************************************************************************************************/


	template<size_t COMPONENTCOUNT = 6>
	struct FLEXKITAPI GameObject
	{
		Component	Components[COMPONENTCOUNT];
		uint16_t	LastComponent;
		uint16_t	ComponentCount;
		
		static const size_t MaxComponentCount = COMPONENTCOUNT;
		
		GameObject()
		{
			LastComponent	= 0;
			ComponentCount	= 0;
		}

		~GameObject()
		{
			for (size_t I = 0; I < ComponentCount; ++I)
				if(Components[I].ReleaseComponent)
					Components[I].ReleaseComponent(Components + I);
		}


		bool Full()
		{
			return (MaxComponentCount <= ComponentCount);
		}


		Component*	FindComponent(ComponentType T)
		{
			if (LastComponent != -1) {
				if (Components[LastComponent].Type == T)
				{
					return &Components[LastComponent];
				}
			}

			for (size_t I = 0; I < ComponentCount; ++I)
				if (Components[I].Type == T) {
					LastComponent = I;
					return &Components[I];
				}

			return nullptr;
		}
	};


	/************************************************************************************************/


	template<typename ... TY_ARGS>
	void CreateComponent(GameObject<>& GO)
	{
	}


	void InitiateGameObject(GameObject<>& GO) {}

	template<typename TY, typename ... TY_ARGS>
	void InitiateGameObject(GameObject<>& GO, TY Component, TY_ARGS ... Args)
	{
		CreateComponent(GO, Component);
		InitiateGameObject(GO, Args...);
	}


	/************************************************************************************************/
}

#endif