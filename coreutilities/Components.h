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
	typedef uint32_t EventTypeID;

	enum ComponentType : uint16_t
	{
		CT_Input,
		CT_Transform,
		CT_Renderable,
		CT_PointLight,
		CT_Collider,
		CT_Player,
		CT_GameObject,
		CT_Unknown
	};

	class ComponentSystemInterface
	{
	public:
		virtual void ReleaseHandle	(ComponentHandle Handle) = 0;
		virtual void HandleEvent	(ComponentHandle Handle, ComponentType EventSource, EventTypeID) {}
	};

	struct Component
	{
		Component()
		{
			ComponentSystem		= nullptr;
			Type				= ComponentType::CT_Unknown;
		}


		Component(
			ComponentSystemInterface*	CS,
			FlexKit::Handle_t<16>		CH,
			ComponentType				T
		) :
			ComponentSystem		(CS),
			ComponentHandle		(CH),
			Type				(T)	{}


		Component& operator = (Component&& RHS)
		{
			ComponentSystem			= RHS.ComponentSystem;
			ComponentHandle			= RHS.ComponentHandle;
			Type					= RHS.Type;

			RHS.ComponentSystem		= nullptr;

			return *this;
		}


		Component(const Component& RValue)
		{
			ComponentHandle		= RValue.ComponentHandle;
			ComponentHandle		= RValue.ComponentHandle;
			Type				= RValue.Type;
		}


		~Component()
		{
			Release();
		}


		void Release()
		{
			if (ComponentSystem)
				ComponentSystem->ReleaseHandle(ComponentHandle);

			ComponentSystem = nullptr;
		}

		ComponentSystemInterface*	ComponentSystem;
		Handle_t<16>				ComponentHandle;
		ComponentType				Type;
	};


	/************************************************************************************************/


	template<size_t COMPONENTCOUNT = 6>
	struct FLEXKITAPI GameObject
	{
		uint16_t	LastComponent;
		uint16_t	ComponentCount;
		Component	Components[COMPONENTCOUNT];
		
		static const size_t MaxComponentCount = COMPONENTCOUNT;
		
		GameObject()
		{
			LastComponent	= 0;
			ComponentCount	= 0;
		}

		~GameObject()
		{
			for (size_t I = 0; I < ComponentCount; ++I)
				Components[I].Release();
		}


		bool Full()
		{
			return (MaxComponentCount <= ComponentCount);
		}


		void NotifyAll(ComponentType Source, EventTypeID EventID)
		{
			for (size_t I = 0; I < ComponentCount; ++I) {
				if(Components[I].ComponentSystem)
					Components[I].ComponentSystem->HandleEvent(Components[I].ComponentHandle, Source, EventID);
			}
		}


		void Release()
		{
			for (size_t I = 0; I < ComponentCount; ++I) {
				if (Components[I].ComponentSystem)
					Components[I].Release();
			}
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


	template<typename TY_GO>
	void CreateComponent(TY_GO& GO)
	{
	}


	template<typename TY_GO>
	void InitiateGameObject(TY_GO& GO) {}


	template<size_t COUNT, typename TY, typename ... TY_ARGS>
	void InitiateGameObject(GameObject<COUNT>& GO, TY Component, TY_ARGS ... Args)
	{
		CreateComponent(GO, Component);
		InitiateGameObject(GO, Args...);
	}


	/************************************************************************************************/
}

#endif