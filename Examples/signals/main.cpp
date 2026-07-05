#include <ExampleFramework.hpp>
#include <TriggerComponent.hpp>
#include <imgui.h>

using namespace FlexKit;

struct TestInput
{
	double t;
};

constexpr uint32_t SlotID		= GetCRC32("Slot0");
constexpr uint32_t TriggerID	= GetCRC32("Trigger0");
constexpr uint32_t InputTypeID	= GetTypeGUID(TestInput);

struct SignalExampleState final : ExampleState
{
	SignalExampleState() 
	{
		auto& triggerView	= object0->AddView<TriggerView>();

		triggerView->CreateTrigger(TriggerID);
		triggerView->CreateSlot(
			SlotID,
			[](void* _ptr, uint32_t typeID)
			{
				if (_ptr != nullptr && typeID == InputTypeID)
				{
					ImGui::Text("Signal was triggered!");
				    ImGui::Text("Signaled at t= %lf!", static_cast<TestInput*>(_ptr)->t);
				}
			});

		triggerView->Connect(TriggerID, SlotID);
	}

	virtual ~SignalExampleState() override
	{
		ReleaseGameObject(*object0);
	}

	struct UpdateTask* Update(struct EngineCore& core, struct UpdateDispatcher& dispatcher, double dt) override
	{
		t += dt;
		return nullptr;
	}

	void DrawUI() override
	{
		Trigger(*object0, TriggerID, TestInput{ .t = t }, InputTypeID);
	}

	double		t = 0.0f;
    GameObject* object0 = &AllocateGameObject();
};

int main()
{
	return RunExample<SignalExampleState>();
}


/**********************************************************************

Copyright (c) 2015 - 2026 Robert May

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
