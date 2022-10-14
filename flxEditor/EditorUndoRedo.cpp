#include "PCH.h"
#include "EditorUndoRedo.h"


/************************************************************************************************/


int currentPosition = 0;
inline alignas(64) FlexKit::CircularBuffer<ObjectState, 256, 16> globalUndoStack = {};


/************************************************************************************************/


void PushState(ObjectState&& objectState)
{
	if (currentPosition != 0)
	{
		for (int i = 0; i < currentPosition; i++)
			globalUndoStack.pop_front();

		currentPosition = 0;
	}

	globalUndoStack.emplace_back(objectState);
}


/************************************************************************************************/


void Undo()
{
	if (currentPosition > globalUndoStack.size())
		return;

	auto itr = globalUndoStack.begin() + currentPosition;
	itr->undo();

	currentPosition++;
}


/************************************************************************************************/


void Redo()
{
	if (currentPosition <= 0)
		return;

	currentPosition--;

	auto itr = globalUndoStack.begin() + currentPosition;
	itr->redo();
}


/************************************************************************************************/


ObjectState& GetCurrentState()
{
	return *(globalUndoStack.begin() + currentPosition);
}


/************************************************************************************************/


uint64_t GetCurrentID()
{
	auto itr = globalUndoStack.begin() + currentPosition;

	return itr->stateID;
}


/************************************************************************************************/



/**********************************************************************

Copyright (c) 2015 - 2022 Robert May

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

