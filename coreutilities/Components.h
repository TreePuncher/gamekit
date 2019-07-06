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
#include "..\coreutilities\logging.h"
#include "..\coreutilities\MathUtils.h"
#include "..\coreutilities\MemoryUtilities.h"
#include "..\coreutilities\ThreadUtilities.h"
#include "..\coreutilities\type.h"


#include <iostream>
#include <type_traits>
#include <tuple>

#ifndef COMPONENT_H
#define COMPONENT_H

namespace FlexKit
{	/************************************************************************************************/

	using ComponentID = uint32_t;
	constexpr ComponentID InvalidComponentID = -1;


	class ComponentBase
	{
	protected:

		struct ComponentEntry
		{
			ComponentBase*	component;
			ComponentID		ID;
		};

		virtual ComponentID	GetID() { return -1; }

		ComponentBase() {}
		// no moving
		ComponentBase& operator =	(ComponentBase&&	rhs)	= delete; 
		ComponentBase				(const ComponentBase&&)		= delete;

		// no copying
		ComponentBase& operator =	(ComponentBase&	rhs)		= delete; 
		ComponentBase				(const ComponentBase&)		= delete;


		void AddComponent(ComponentBase& system)
		{
			const auto ID = system.GetID();
			auto res = find(Systems, [&](auto value)
				{
					return value.ID == ID;
				});

			if (res != Systems.end())
				FK_LOG_WARNING("Component System Adding already added System!");

			Systems.push_back({ &system, system.GetID() });
		}


		void RemoveComponent(ComponentBase& system) noexcept
		{
			const auto ID = system.GetID();
			auto res = find(Systems, [&](auto value)
				{
					return value.ID == ID;
				});

			if (res != Systems.end()) 
			{
				*res = Systems.back();
				Systems.pop_back();
			}
			else
				FK_LOG_WARNING("Component System Releasing Already Released System!");
		}

	public:
		static ComponentBase& GetComponent(ComponentID ID)
		{
			auto res = find(
				Systems,
				[&](auto e)
				{
					return e.ID == ID;
				});

			if (res == Systems.end())
			{
				FK_LOG_ERROR("Component System Not Initialized!");
				throw(std::runtime_error("Component System Not Initialized!"));
			}

			return *(*res).component;
		}


		static static_vector<ComponentEntry, 128> GetSystemList()
		{
			return Systems;
		}


		inline static static_vector<ComponentEntry, 128> Systems = static_vector<ComponentBase::ComponentEntry, 128>();
	};


	/************************************************************************************************/


	template<typename TY, typename TY_Handle, ComponentID ID = -1>
	class Component : public ComponentBase
	{
	public:
		using ComponetHandle_t = TY_Handle;

	protected:
		inline static TY* component	= nullptr;


		Component() 
		{
			FK_ASSERT(component != nullptr, "Component System error: double creation detected!");

			component = static_cast<TY*>(this);
			ComponentBase::AddComponent(*static_cast<ComponentBase*>(this));
		}


		~Component()
		{
			ComponentBase::RemoveComponent(*static_cast<ComponentBase*>(this));
			component = nullptr;
		}

	public:

		ComponentID GetID() override		{ return ID; }

		static ComponentID GetComponentID()	{ return ID; }

		static TY& GetComponent()
		{
			if (!component)
				throw(std::runtime_error("Component::GetSystem() : ERROR : Component not initialized!"));

			return *static_cast<TY*>(component);
		}

		static bool isAvailable()
		{
			return component != nullptr;
		}
	};


	/************************************************************************************************/
	

	class BehaviorBase
	{
	public:
		BehaviorBase(ComponentID IN_ID) :
			ID{ IN_ID } {}

		ComponentBase& GetComponentRef()	{ return ComponentBase::GetComponent(ID); }

		ComponentID ID;
	};


	template<typename ComponentTY>
	class Behavior_t : public BehaviorBase
	{
	public:
		Behavior_t() : BehaviorBase{ ComponentTY::GetComponentID() } {}

		static ComponentID	GetComponentID()	{ return ComponentTY::GetComponentID(); }
		static auto			GetComponent() -> decltype(ComponentTY::GetComponent())& { return ComponentTY::GetComponent(); }
	};


	/************************************************************************************************/


	class GameObject
	{
	public:
		GameObject(iAllocator* allocator = SystemAllocator) : 
			behaviors{ allocator } {}


		~GameObject()
		{
			Release();
		}


		void AddBehavior(BehaviorBase* behavior)
		{
			behaviors.push_back(behavior);
		}


		void AddBehavior(BehaviorBase& behavior)
		{
			behaviors.push_back(&behavior);
		}


		void Release()
		{
			for (BehaviorBase* behaviors : behaviors)
				behaviors->~BehaviorBase();

			behaviors.clear();
		}


		BehaviorBase* GetBehavior(ComponentID id)
		{
			for (auto behavior : behaviors)
				if (behavior->ID == id)
					return behavior;

			return nullptr;
		}


		bool hasBehavior(ComponentID id)
		{
			for (auto behavior : behaviors)
				if (behavior->ID == id)
					return true;

			return false;
		}


	private:
		Vector<BehaviorBase*>	behaviors;	// component + Code
	};


	template<typename TY_COMPONENT_ARG, typename ... TY_PACKED_ARGS>
	bool hasBehaviors(GameObject& go)
	{
		using TY_COMPONENT = typename std::remove_pointer<TY_COMPONENT_ARG>::type;
		static_assert(std::is_base_of<BehaviorBase, TY_COMPONENT>::value, "Parameter that is not a behavior type detected, behavior types only!");

		if constexpr (sizeof...(TY_PACKED_ARGS) == 0)
			return go.hasBehavior(TY_COMPONENT::GetComponentID());
		else
			return go.hasBehavior(TY_COMPONENT::GetComponentID()) && hasBehaviors<TY_PACKED_ARGS...>(go);
	}


	template<typename TY_COMPONENT>
	auto GetBehavior(GameObject& go)
	{
		return static_cast<TY_COMPONENT*>(go.GetBehavior(TY_COMPONENT::GetComponentID()));
	}

	template<typename TY_COMPONENT_ARG, typename ... TY_PACKED_ARGS>
	auto GetBehaviors(GameObject& go)
	{
		using TY_COMPONENT = typename std::remove_pointer<TY_COMPONENT_ARG>::type;
		static_assert(std::is_base_of<BehaviorBase, TY_COMPONENT>::value, "Parameter that is not a behavior type detected, behavior types only!");

		if constexpr (sizeof...(TY_PACKED_ARGS) == 0)
			return std::tuple<TY_COMPONENT*>{ GetBehavior<TY_COMPONENT>(go) };
		else
			return std::tuple_cat(
				std::tuple<TY_COMPONENT*>{ GetBehavior<TY_COMPONENT>(go) },
				GetBehaviors<TY_PACKED_ARGS...>(go) );
	}


	template<typename ... TY_PACKED_ARGS, typename FN>
	bool Execute(GameObject& go, FN fn)
	{
		if (!hasBehaviors<TY_PACKED_ARGS...>(go))
			return false;

		std::apply(fn, GetBehaviors<TY_PACKED_ARGS...>(go));

		return true;
	}

	// Thank Columbo for this idea, https://stackoverflow.com/a/28213747

	template <typename T>
	struct ExecuterProxy : ExecuterProxy<decltype(&T::operator())> {};

	#define REM_CTOR(...) __VA_ARGS__
	#define SPEC(cv, var, is_var)										\
	template <typename C, typename R, typename... Args>					\
	struct ExecuterProxy<R (C::*) (Args... REM_CTOR var) cv>			\
	{																	\
		template<typename TY> static bool run(GameObject& go, TY& fn)	\
		{																\
			return Execute<Args...>(go, fn);							\
		}																\
	};																	\

	SPEC(const, (,...), 1)
	SPEC(const, (), 0)
	SPEC(, (,...), 1)
	SPEC(, (), 0)


	/************************************************************************************************/


	template<typename FN>
	bool Apply(GameObject& go, FN& fn)
	{
		return ExecuterProxy<FN>::run(go, fn);
	}


	/************************************************************************************************/


	constexpr ComponentID SampleComponentID = GetTypeGUID(SampleComponentID);
	using SampleHandle = Handle_t <32, GetTypeGUID(SampleHandle)>;


	class SampleComponent : public Component<SampleComponent, SampleHandle, SampleComponentID>
	{
	public:
		SampleComponent(iAllocator* allocator = SystemAllocator) : entities{ allocator } { }

		ComponetHandle_t CreateComponent()
		{
			return ComponetHandle_t{ unsigned int(entities.push_back({})) };
		}

		void ReleaseEntity(SampleHandle handle)
		{
			// do release
		}

		struct Entity
		{
			int x;
		};

		Vector<Entity> entities;
	};


	constexpr ComponentID SampleComponent2ID = GetTypeGUID(SampleComponent2ID);
	using Sample2Handle = Handle_t <32, GetTypeGUID(Sample2Handle)>;


	class SampleComponent2 : public Component<SampleComponent2, Sample2Handle, SampleComponent2ID>
	{
	public:
		SampleComponent2(iAllocator* allocator = SystemAllocator) : entities{ allocator } { }

		ComponetHandle_t CreateComponent()
		{
			return ComponetHandle_t{ unsigned int(entities.push_back({})) };
		}

		void ReleaseEntity(Sample2Handle handle)
		{
			// do release
		}

		struct Entity
		{
			int x;
		};

		Vector<Entity> entities;
	};

	constexpr ComponentID SampleComponent3ID = GetTypeGUID(SampleComponent3ID);
	using Sample3Handle = Handle_t <32, GetTypeGUID(Sample3Handle)>;


	class SampleComponent3 : public Component<SampleComponent3, Sample3Handle, SampleComponent3ID>
	{
	public:
		SampleComponent3(iAllocator* allocator = SystemAllocator) : entities{ allocator } { }

		ComponetHandle_t CreateComponent()
		{
			return ComponetHandle_t{ unsigned int(entities.push_back({})) };
		}

		void ReleaseEntity(Sample3Handle handle)
		{
			// do release
		}

		struct Entity
		{
			int x;
		};

		Vector<Entity> entities;
	};



	class SampleBehavior : public Behavior_t<SampleComponent>
	{
	public:
		SampleBehavior() :
			handle{ GetComponent().CreateComponent() } {}


		~SampleBehavior()
		{
			GetComponent().ReleaseEntity(handle);
		}


		void DoSomething()
		{
			std::cout << "SampleBehavior1::DoSomething()\n";
			GetComponent().entities[handle].x++;
		}

		SampleHandle handle;
	};


	class SampleBehavior2 : public Behavior_t<SampleComponent2>
	{
	public:
		SampleBehavior2() :
			handle{ GetComponent().CreateComponent() } {}


		~SampleBehavior2()
		{
			GetComponent().ReleaseEntity(handle);
		}


		void DoSomething()
		{
			std::cout << "SampleBehavior2::DoSomething()\n";
			GetComponent().entities[handle].x++;
		}

		Sample2Handle handle;
	};

	class SampleBehavior3 : public Behavior_t<SampleComponent3>
	{
	public:
		SampleBehavior3() :
			handle{ GetComponent().CreateComponent() } {}


		~SampleBehavior3()
		{
			GetComponent().ReleaseEntity(handle);
		}


		void DoSomething()
		{
			std::cout << "SampleBehavior3::DoSomething()\n";
			GetComponent().entities[handle].x++;
		}

		Sample3Handle handle;
	};
	

	/************************************************************************************************/


	void GameOjectTest(iAllocator* allocator)
	{
		SampleComponent		sample1(allocator);
		SampleComponent2	sample2(allocator);
		SampleComponent3	sample3(allocator);

		GameObject go;
		go.AddBehavior(&allocator->allocate<SampleBehavior>());
		go.AddBehavior(&allocator->allocate<SampleBehavior2>());

		Apply(go,
			[](	// Function Sources
				SampleBehavior*		sample1,
				SampleBehavior2*	sample2,
				SampleBehavior3*	sample3
			)
			{	// do things with behaviors
				// JK doesn't run, not all inputs satisfied!
				sample1->DoSomething();
				sample2->DoSomething();
				sample3->DoSomething();
			});

		go.AddBehavior(&allocator->allocate<SampleBehavior3>());

		Apply(go,
			[](	// Function Sources
				SampleBehavior*		sample1,
				SampleBehavior2*	sample2,
				SampleBehavior3*	sample3	)
			{	// do things with behaviors
				sample1->DoSomething();
				sample2->DoSomething();
				sample3->DoSomething();
			});
	}


	/************************************************************************************************/


	class UpdateDispatcher
	{
	public:
		typedef size_t	UpdateID_t;

		class UpdateTask
		{
		public:
			struct iUpdateFN
			{
				virtual void operator () (UpdateTask& task) = 0;
			};


			UpdateTask(ThreadManager* IN_manager, iUpdateFN& IN_updateFn, iAllocator* IN_allocator) :
				Update		{ IN_updateFn						},
				Inputs		{ IN_allocator						},
				visited		{ false								},
				completed	{ 0									},
				threadTask	{ this, IN_manager, IN_allocator	} {}

			// No Copy
			UpdateTask				(const UpdateTask&)	= delete;
			UpdateTask& operator =	(const UpdateTask&)	= delete;


			class UpdateThreadTask : public iWork
			{
			public:
				UpdateThreadTask(UpdateTask* IN_task, ThreadManager* threads, iAllocator* memory) :
					iWork	{ memory					},
					task	{ IN_task					},
					wait	{ *this, threads, memory	} {}

				// No Copy
				UpdateThreadTask				(const UpdateThreadTask&) = delete;
				UpdateThreadTask& operator =	(const UpdateThreadTask&) = delete;

				UpdateTask*	task;

				void Run() override
				{
					task->Run();
					task->completed++;
				}

				void Release() override
				{
				}

				WorkDependencyWait wait;
			}threadTask;


			void Run()
			{
				Update(*this);
			}


			void AddInput(UpdateTask& task)
			{
				Inputs.push_back(&task);
				threadTask.wait.AddDependency(task.threadTask);
			}


			bool isLeaf() const noexcept
			{
				return Inputs.size() == 0;
			}


			void MarkVisited() noexcept
			{
				visited = true;
			}


			bool Visited() const noexcept
			{
				return visited;
			}

			UpdateID_t			ID;
			iUpdateFN&			Update;
			char*				Data;
			int					completed;
			bool				visited;
			Vector<UpdateTask*> Inputs;
		};


		UpdateDispatcher(ThreadManager* IN_threads, iAllocator* IN_allocator) :
			nodes		{ IN_allocator	},
			allocator	{ IN_allocator	},
			threads		{ IN_threads	}{}


		// No Copy
		UpdateDispatcher					(const UpdateDispatcher&) = delete;
		const UpdateDispatcher& operator =	(const UpdateDispatcher&) = delete;

		static void VisitInputs(UpdateTask* Node, Vector<UpdateTask*>& out)
		{
			if (Node->visited)
				return;

			Node->visited = true;

			for (auto& Node : Node->Inputs)
				VisitInputs(Node, out);

			out.push_back(Node);
		};


		static void VisitAndScheduleLeafs(UpdateTask* node, ThreadManager* threads, iAllocator* allocator)
		{
			if (node->Visited())
				return;

			node->MarkVisited();

			for (auto& childNode : node->Inputs)
				VisitAndScheduleLeafs(childNode, threads, allocator);

			if (node->isLeaf())
				threads->AddWork(node->threadTask, allocator);
		}


		void Execute()
		{
			if(threads->GetThreadCount())
			{	// Multi Thread
				WorkBarrier barrier{ *threads, allocator };

				for (auto& node : nodes)
					barrier.AddDependentWork(&node->threadTask);

				for (auto& node : nodes)
					VisitAndScheduleLeafs(node, threads, allocator);

				barrier.Join();
				nodes.clear();
			}
			else
			{
				// Single Thread
				Vector<UpdateTask*> nodesSorted{ allocator };
				for (auto& node : nodes)
					VisitInputs(node, nodesSorted);

				for (auto& node : nodesSorted) 
				{
					node->Update(*node);
				}

				nodesSorted.clear();
			}

		}


		class UpdateBuilder
		{
		public:
			UpdateBuilder(UpdateTask& IN_node) :
				newNode{ IN_node	} {}

			void SetDebugString(const char* str) noexcept
			{
				newNode.threadTask.SetDebugID(str);
			}

			void AddOutput(UpdateTask& node)
			{
				node.AddInput(newNode);
			}

			void AddInput(UpdateTask& node)
			{
				newNode.AddInput(node);
			}

			const char* DebugID = "UNIDENTIFIED TASK";
		private:
			UpdateTask& newNode;
		};

		template<
			typename TY_NODEDATA,
			typename FN_LINKAGE,
			typename FN_UPDATE>
		UpdateTask&	Add(FN_LINKAGE LinkageSetup, FN_UPDATE UpdateFN)
		{
			struct data_BoilderPlate : UpdateTask::iUpdateFN
			{
				data_BoilderPlate(FN_UPDATE&& IN_fn) :
					function{ std::move(IN_fn) } {}

				virtual void operator() (UpdateTask& task) override
				{
					function(locals);
				}

				TY_NODEDATA locals;
				FN_UPDATE	function;
			};

			auto& functor		= allocator->allocate_aligned<data_BoilderPlate>(std::move(UpdateFN));
			UpdateTask& newNode = allocator->allocate_aligned<UpdateTask>(threads, functor, allocator);
			newNode.Data		= reinterpret_cast<char*>(&functor.locals);

			UpdateBuilder Builder{ newNode };
			LinkageSetup(Builder, functor.locals);

			//newNode; = Builder.DebugID;

			nodes.push_back(&newNode);

			return newNode;
		}

		ThreadManager*		threads;
		Vector<UpdateTask*> nodes;
		iAllocator*			allocator;
	};


	using DependencyBuilder = UpdateDispatcher::UpdateBuilder;
	using UpdateTask		= UpdateDispatcher::UpdateTask;


}	/************************************************************************************************/

#endif