#include "Components.h"
#include "ComponentBlobs.h"

namespace FlexKit
{   /************************************************************************************************/


    StringIDHandle StringIDComponent::Create(const char* initial, size_t length)
    {
        auto handle = handles.GetNewHandle();

        StringID newID;
        newID.ID[length] = '\0';
        strncpy_s(newID.ID, initial, Min(sizeof(StringID), length));

        handles[handle] = static_cast<index_t>(IDs.push_back(newID));

        return handle;
    }


    /************************************************************************************************/


    void StringIDComponent::Remove(StringIDHandle handle)
    {
        auto lastElement = IDs.back();
        IDs[handles[handle]] = lastElement;
        IDs.pop_back();

        handles[lastElement.handle] = handles[handle];
        handles.RemoveHandle(handle);
    }


    /************************************************************************************************/


    void StringIDComponent::AddComponentView(GameObject& GO, const std::byte* buffer, const size_t bufferSize, iAllocator* allocator)
    {
        IDComponentBlob blob;
        std::memcpy(&blob, buffer, sizeof(blob));

        auto temp = sizeof(blob.ID);

        const size_t IDLen = strnlen(blob.ID, sizeof(blob.ID));

        GO.AddView<StringIDView>(blob.ID, IDLen);
    }


    /************************************************************************************************/


    void GameOjectTest(iAllocator* allocator)
    {
        SampleComponent		sample1(allocator);
        SampleComponent2	sample2(allocator);
        SampleComponent3	sample3(allocator);

        GameObject go;
        go.AddView<SampleView>();
        go.AddView<SampleView2>();
        go.AddView<TestView>();

        Apply(go,
            [&](	// Function Sources
                SampleView& sample1,
                SampleView2& sample2,
                SampleView3& sample3
                )
            {	// do things with behaviors
                // JK doesn't run, not all inputs satisfied!
                sample1.DoSomething();
                sample2.DoSomething();
                sample3.DoSomething();

                FK_ASSERT(false, "Expected Failure");
            },
            [&]
            {
                FK_ASSERT(true, "SUCCESS");

                // this runs instead!
            });

        go.AddView<SampleView3>();

        Apply(go,
            [](	// Function Sources
                SampleView& sample1,
                SampleView2& sample2,
                SampleView3& sample3,
                TestView& test1)
            {	// do things with behaviors
                sample1.DoSomething();
                sample2.DoSomething();
                sample3.DoSomething();

                auto val = test1.GetData();

                FK_ASSERT(true, "SUCCESS");
            },
            []
            {   // should not run
                FK_ASSERT(false, "Unexpected Failure");
            });
    }


}   /************************************************************************************************/


/**********************************************************************

Copyright (c) 2015 - 2020 Robert May

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
