/**********************************************************************

Copyright (c) 2017 Robert May

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

#ifndef CONSOLESUBSTATE_H
#define CONSOLESUBSTATE_H

#include "Console.h"
#include "GameFramework.h"


namespace FlexKit
{	// Will eventually be a Quake Style Console
	struct ConsoleSubState : public FrameworkState
	{
		ConsoleSubState(GameFramework* framework) :
			FrameworkState(framework) 
		{
			Console              = &Framework->Console;
			Framework            = Framework;
			Core				 = Framework->Core;
			PauseBackgroundLogic = true;
		}

		~ConsoleSubState()
		{
			Framework->ConsoleActive = false;
			Console->Memory->free(this); // Not sure what to do about this. Seems like a poor design implication
		}

		bool			PauseBackgroundLogic;
		size_t			RecallIndex;
		Console*		Console;
		EngineCore*		Core;

		void IncrementRecallIndex();
		void DecrementRecallIndex();

		bool  Update			(EngineCore* Engine, double dT) override;
		bool  DebugDraw			(EngineCore* Engine, double dT) override;
		bool  PreDrawUpdate		(EngineCore* Engine, double dT) override;
		bool  PostDrawUpdate	(EngineCore* Engine, double dT) override;

		bool  EventHandler		(Event evt)	override;

	};

}
#endif