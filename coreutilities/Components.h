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
#include "..\coreutilities\type.h"
#include "..\coreutilities\MemoryUtilities.h"
#include "..\coreutilities\logging.h"
#include "..\coreutilities\ThreadUtilities.h"

#include <iostream>
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
			for (BehaviorBase* behaviors : behaviors)
				behaviors->~BehaviorBase();
		}


		void AddBehavior(BehaviorBase* behavior)
		{
			behaviors.push_back(behavior);
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


	template<typename TY_component>
	bool hasSources(GameObject& go)
	{
		return go.hasBehavior(TY_component::GetComponentID());
	}


	template<typename TY_component, typename TY_PACKED_ARGS>
	bool hasSources(GameObject& go)
	{
		return go.hasBehavior(TY_component::GetComponentID()) && hasSources<TY_PACKED_ARGS>(go);
	}


	template<typename TY_COMPONENT>
	std::tuple<TY_COMPONENT*> GetSources(GameObject& go)
	{
		return std::tuple<TY_COMPONENT*>(
			static_cast<TY_COMPONENT*>(go.GetBehavior(TY_COMPONENT::GetComponentID())));
	}


	template<typename TY_COMPONENT, typename TY_PACKED_ARGS>
	auto GetSources(GameObject& go)
	{
		return std::tuple_cat(
			std::tuple<TY_COMPONENT*>(static_cast<TY_COMPONENT*>(go.GetBehavior(TY_COMPONENT::GetComponentID()))), 
			GetSources<TY_PACKED_ARGS>(go));
	}


	template<typename ... TY_PACKED_ARGS, typename FN>
	bool Execute(GameObject& go, FN fn)
	{
		if (!hasSources<TY_PACKED_ARGS...>(go))
			return false;

		fn(GetSources<TY_PACKED_ARGS...>(go));

		return true;
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
			GetComponent().entities[handle].x++;
		}

		Sample2Handle handle;
	};
	

	/************************************************************************************************/


	void GameOjectTest(iAllocator* allocator)
	{
		SampleComponent		sample1(allocator);
		SampleComponent2	sample2(allocator);

		GameObject go;
		go.AddBehavior(&allocator->allocate<SampleBehavior>());
		go.AddBehavior(&allocator->allocate<SampleBehavior2>());

		Execute<SampleBehavior, SampleBehavior2>(go,
			[](auto& inputs)
			{
				SampleBehavior*		sample1 = get<SampleBehavior*>(inputs);
				SampleBehavior2*	sample2 = get<SampleBehavior2*>(inputs);

				sample1->DoSomething();
				sample2->DoSomething();

				// do things with behaviors
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