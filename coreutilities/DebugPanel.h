#ifndef CONSOLESUBSTATE_H
#define CONSOLESUBSTATE_H

#include "Console.h"
#include "GameFramework.h"


namespace FlexKit
{	// Will eventually be a Quake Style Console
	struct DebugPanel : public FrameworkState
	{
		DebugPanel(GameFramework& framework, FrameworkState& IN_topState) :
            FrameworkState  { framework      },
            topState        { IN_topState    },
            core            { framework.core }

		{
			console              = &framework.console;
			pauseBackgroundLogic = true;
		}

		~DebugPanel()
		{
			framework.consoleActive = false;
			console->allocator->free(this); // Not sure what to do about this. Seems like a poor design implication
		}

		bool			pauseBackgroundLogic;
		size_t			recallIndex;
		Console*		console;
		EngineCore&		core;
        FrameworkState& topState;

		void IncrementRecallIndex();
		void DecrementRecallIndex();

		void Update			(EngineCore& Engine, UpdateDispatcher& Dispatcher, double dT) override;
		void DebugDraw			(EngineCore& Engine, UpdateDispatcher& Dispatcher, double dT) override;
		void Draw				(EngineCore& Engine, UpdateDispatcher& Dispatcher, double dT, FrameGraph& Graph) override;
		void PostDrawUpdate 	(EngineCore& Engine, UpdateDispatcher& Dispatcher, double dT, FrameGraph& Graph) override;

		void EventHandler		(Event evt)	override;
	};

}

/**********************************************************************

Copyright (c) 2019 Robert May

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
#endif
