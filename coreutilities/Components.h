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

#ifndef COMPONENT_H
#define COMPONENT_H

namespace FlexKit
{	/************************************************************************************************/


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
				Update		{ IN_updateFn	},
				Inputs		{ IN_allocator	},
				visited		{ false			},
				completed	{ 0				},
				threadTask	{ this, IN_manager, IN_allocator } {}

			// No Copy
			UpdateTask(const UpdateTask&)				= delete;
			UpdateTask& operator = (const UpdateTask&)	= delete;

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

			bool isLeaf()
			{
				return Inputs.size() == 0;
			}


			void MarkVisited()
			{
				visited = true;
			}


			bool Visited()
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

			void SetDebugString(const char* str)
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