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
			typedef std::function<void(UpdateTask& Node)>	FN_NodeAction;

			UpdateTask(ThreadManager* IN_manager, iAllocator* IN_allocator) :
				Inputs		{ IN_allocator	},
				visited		{ false			},
				DebugID		{ nullptr		},
				threadTask	{ this, IN_manager, IN_allocator }{}

			// No Copy
			UpdateTask(const UpdateTask&)				= delete;
			UpdateTask& operator = (const UpdateTask&)	= delete;

			class UpdateThreadTask : public iWork
			{
			public:
				UpdateThreadTask(UpdateTask* IN_task, ThreadManager* threads, iAllocator* memory) :
					iWork	{ memory		},
					task	{ IN_task		},
					wait	{ *this, threads, memory }{}

				// No Copy
				UpdateThreadTask				(const UpdateThreadTask&) = delete;
				UpdateThreadTask& operator =	(const UpdateThreadTask&) = delete;

				UpdateTask*	task;

				void Run() override
				{
					task->Run();
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
			FN_NodeAction		Update;
			char*				Data;
			const char*			DebugID;
			bool				visited;
			Vector<UpdateTask*> Inputs;
		};

		using			FN_NodeAction = UpdateTask::FN_NodeAction;


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
			if(true)
			{
				// Multi Thread
				WorkBarrier barrier{allocator};

				for (auto& node : nodes) {
					barrier.AddDependentWork(&node->threadTask);
					VisitAndScheduleLeafs(node, threads, allocator);
				}

				barrier.Wait();
			}
			else
			{
				// Single Thread
				Vector<UpdateTask*> NodesSorted{ allocator };
				for (auto& node : nodes)
					VisitInputs(node, NodesSorted);

				for (auto& node : NodesSorted) 
				{
					if (node->DebugID)
						FK_LOG_9("Running task %s", node->DebugID);

					node->Update(*node);
				}
			}
		}


		class UpdateBuilder
		{
		public:
			UpdateBuilder(UpdateTask& IN_node) :
				newNode{ IN_node	} {}

			void SetDebugString(const char* str)
			{
				newNode.DebugID = str;
			}

			void AddOutput(UpdateTask& node)
			{
				node.AddInput(newNode);
			}

			void AddInput(UpdateTask& node)
			{
				newNode.AddInput(node);
			}

		private:
			UpdateTask& newNode;
		};

		template<
			typename TY_NODEDATA,
			typename FN_LINKAGE,
			typename FN_UPDATE>
		UpdateTask&	Add(FN_LINKAGE LinkageSetup, FN_UPDATE UpdateFN)
		{
			auto& data			= allocator->allocate<TY_NODEDATA>();
			UpdateTask& newNode = allocator->allocate<UpdateTask>(threads, allocator);

			UpdateBuilder Builder{ newNode };
			LinkageSetup(Builder, data);

			newNode.Data	= (char*)&data;
			newNode.Update	= 
				[UpdateFN](UpdateTask& node)
				{
					TY_NODEDATA& data = *(TY_NODEDATA*)node.Data;
					UpdateFN(data);
				};

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