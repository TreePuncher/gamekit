

#ifndef TEXTURESTREAMINGTEST_H
#define TEXTURESTREAMINGTEST_H

#include "..\coreutilities\WorldRender.h"
#include "..\coreutilities\GameFramework.h"
#include "BaseState.h"


class TextureStreamingTest final : FlexKit::FrameworkState
{
public:
    TextureStreamingTest(FlexKit::GameFramework& IN_framework, BaseState& IN_base) :
        FrameworkState  { IN_framework  },
        base            { IN_base       },
        readBackBuffer  { IN_framework.core.RenderSystem.CreateReadBackBuffer(2048) } {}

    void Update         (EngineCore&, UpdateDispatcher&, double dT)             override;
    void Draw           (EngineCore&, UpdateDispatcher&, double, FrameGraph&)   override;

    void EventHandler   (Event evt) override;

    ReadBackResourceHandle  readBackBuffer;
    BaseState&              base;
};


#endif

/**********************************************************************

Copyright (c) 2015 - 2019 Robert May

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
