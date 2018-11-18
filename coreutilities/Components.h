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

			UpdateTask(iAllocator* Memory) :
				Inputs		{ Memory	},
				Executed	{ false		},
				DebugID		{ nullptr	}{}

			Vector<UpdateTask*> Inputs;
			UpdateID_t			ID;
			FN_NodeAction		Update;
			char*				Data;
			const char*			DebugID;
			bool				Executed;
		};

		using			FN_NodeAction = UpdateTask::FN_NodeAction;

		UpdateDispatcher(iAllocator* IN_Memory) :
			Nodes	{ IN_Memory },
			Memory	{ IN_Memory }{}

		static void VisitInputs(UpdateTask* Node, Vector<UpdateTask*>& out)
		{
			if (Node->Executed)
				return;

			Node->Executed = true;

			for (auto& Node : Node->Inputs)
				VisitInputs(Node, out);

			out.push_back(Node);
		};

		void Execute()
		{
			// TODO: Detect multi-threadable sections

			Vector<UpdateTask*> NodesSorted{Memory};

			for (auto& Node : Nodes)
				VisitInputs(Node, NodesSorted);

			for (auto& Node : NodesSorted) {

				if(Node->DebugID)
					FK_LOG_INFO("Running task %s", Node->DebugID);

				Node->Update(*Node);
			}
		}

		class UpdateBuilder
		{
		public:
			UpdateBuilder(UpdateTask& IN_Node) :
				NewNode{IN_Node}{}

			void SetDebugString(const char* str)
			{
				NewNode.DebugID = str;
			}

			void AddOutput(UpdateTask& Node)
			{
				Node.Inputs.push_back(&NewNode);
			}

			void AddInput(UpdateTask& Node)
			{
				NewNode.Inputs.push_back(&Node);
			}

		private:
			UpdateTask& NewNode;
		};

		template<
			typename TY_NODEDATA,
			typename FN_LINKAGE,
			typename FN_UPDATE>
		UpdateTask&	Add(FN_LINKAGE LinkageSetup, FN_UPDATE UpdateFN)
		{
			auto& Data = Memory->allocate<TY_NODEDATA>();
			UpdateTask& NewNode = Memory->allocate<UpdateTask>(Memory);

			UpdateBuilder Builder{ NewNode };
			LinkageSetup(Builder, Data);

			NewNode.Data	= (char*)&Data;
			NewNode.Update	= [UpdateFN](UpdateTask& Node)
				{
					TY_NODEDATA& Data = *(TY_NODEDATA*)Node.Data;
					UpdateFN(Data);
				};

			Nodes.push_back(&NewNode);

			return NewNode;
		}

		Vector<UpdateTask*> Nodes;
		iAllocator*			Memory;
	};


	using DependencyBuilder = UpdateDispatcher::UpdateBuilder;
	using UpdateTask		= UpdateDispatcher::UpdateTask;


}	/************************************************************************************************/

#endif