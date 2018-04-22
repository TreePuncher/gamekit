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
#include "..\coreutilities\containers.h"
#include "..\coreutilities\GraphicScene.h"
#include "..\coreutilities\Events.h"
#include "..\coreutilities\Handle.h"
#include "..\coreutilities\MathUtils.h"
#include "..\coreutilities\type.h"



#ifndef COMPONENT_H
#define COMPONENT_H

namespace FlexKit
{
	// GOAL: Help reduce code involved to do shit which is currently excessive 
	/************************************************************************************************/

	// Components store all data needed for a component
	typedef FlexKit::Handle_t<16> ComponentHandle;

	const ComponentHandle InvalidComponentHandle(-1);

	typedef uint32_t EventTypeID;

	typedef uint32_t ComponentType;

	struct Component;
	struct ComponentListInterface;

	const uint32_t UnknownComponentID = GetTypeGUID(Component);

	class ComponentSystemInterface
	{
	public:
		virtual ~ComponentSystemInterface() {}

		virtual void ReleaseHandle	(ComponentHandle Handle) = 0;
		virtual void HandleEvent	(ComponentHandle Handle, ComponentType EventSource, ComponentSystemInterface* System, EventTypeID, ComponentListInterface* GO) {}
		virtual void ObjectMoved	(ComponentHandle Handle, ComponentSystemInterface* System, ComponentListInterface* GO)	{}
	};


	struct Component
	{
		Component() :
			ComponentSystem	(nullptr),
			Type			(GetTypeGUID(Component)){}


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
		ComponentType				Type;
		Handle_t<16>				ComponentHandle;
	};


	/************************************************************************************************/


	template<size_t COMPONENTCOUNT = 6>
	struct FLEXKITAPI ComponentList
	{
		typedef ComponentList<COMPONENTCOUNT> ThisType_t;

		uint16_t	LastComponent;
		uint16_t	ComponentCount;
		Component	Components[COMPONENTCOUNT];
		
		static const size_t MaxComponentCount = COMPONENTCOUNT;
		
		ComponentList()
		{
			LastComponent	= 0;
			ComponentCount	= 0;
		}

		~ComponentList()
		{
			for (size_t I = 0; I < ComponentCount; ++I)
				Components[I].Release();
		}


		ThisType_t& operator = (ThisType_t&& RHS)
		{
			for (size_t I = 0; I < ComponentCount; ++I) {
				Components[I] = RHS.Components[I];
				Components[I]->ComponentSystem->ObjectMoved(Components[I].ComponentHandle, Components[I].ComponentSystem, *this);
			}

			return *this;
		}
		

		bool Full()
		{
			return (MaxComponentCount <= ComponentCount);
		}

		bool AddComponent(Component&& NewC)
		{
			if (!Full())
			{
				for (auto& C : Components) {
					if (C.Type == UnknownComponentID) 
					{
						C = std::move(NewC);
						break;
					}
				}

				++ComponentCount;
				return true;
			}
			return false;
		}

		void Release()
		{
			for (size_t I = 0; I < ComponentCount; ++I) {
				if (Components[I].ComponentSystem)
					Components[I].Release();
			}
		}


		operator ComponentListInterface* ()
		{
			return (ComponentListInterface*)this;
		}

		operator ComponentListInterface& ()
		{
			return *(ComponentListInterface*)this;
		}
	};


	struct FLEXKITAPI ComponentListInterface
	{
		ComponentListInterface				(const ComponentListInterface&) = delete;
		ComponentListInterface& operator = (const ComponentListInterface&) = delete;


		uint16_t	LastComponent;
		uint16_t	ComponentCount;
		Component	Components[];

		operator ComponentListInterface* () { return this; }
	};


	Component*	FindComponent(ComponentListInterface* GO, ComponentType T)
	{
		if (GO->LastComponent != -1) {
			if (GO->Components[GO->LastComponent].Type == T)
			{
				return &GO->Components[GO->LastComponent];
			}
		}

		for (size_t I = 0; I < GO->ComponentCount; ++I)
			if (GO->Components[I].Type == T) {
				GO->LastComponent = I;
				return &GO->Components[I];
			}

		return nullptr;
	}	


	/************************************************************************************************/


	void NotifyAll(ComponentListInterface* GO, ComponentType Source, EventTypeID EventID, ComponentSystemInterface* System = nullptr)
	{
		for (size_t I = 0; I < GO->ComponentCount; ++I)
		{
			if (GO->Components[I].ComponentSystem)
				GO->Components[I].ComponentSystem->HandleEvent(
					GO->Components[I].ComponentHandle,
					Source, System, EventID, GO);
		}
	}


	/************************************************************************************************/


	template<typename TY_GO>
	void CreateComponent(TY_GO& GO)
	{
	}


	template<typename TY_GO>
	void InitiateComponentList(TY_GO& GO) {}


	template<size_t COUNT, typename TY, typename ... TY_ARGS>
	void InitiateComponentList(ComponentList<COUNT>& GO, TY Component, TY_ARGS ... Args)
	{
		CreateComponent(GO, Component);
		InitiateComponentList(GO, Args...);
	}


	/************************************************************************************************/


	class BehaviorBase
	{
	public:
		BehaviorBase(ComponentListInterface& go, size_t BehaviorID_IN) : GO{ go }, BehaviorID{ BehaviorID_IN } {}
		
		virtual bool Validate()
		{
			return(&GO != nullptr);
		}

		const size_t			BehaviorID;
		ComponentListInterface&	GO;
	};


	class iUpdatable
	{
	public:
		virtual void Update(const double dt) {}
	};


	class iEventReceiver
	{
	public:
		virtual void Handle(const Event& evt) {}
	};



}	/************************************************************************************************/

#endif