#include <type_traits>
#include <memory>
#include <ResourceHandles.hpp>

namespace FlexKit
{
	struct EngineCore;
	struct ExampleResources;
	struct Event;
	struct FrameGraph;
	struct UpdateTask;
	struct UpdateDispatcher;

	struct ExampleState
	{
		virtual ~ExampleState() {}

	    struct DrawExampleContext
	    {
		    struct	UpdateTask*			update;
		            FrameResourceHandle	renderTarget;
	    };

		virtual UpdateTask* Update(EngineCore& core, UpdateDispatcher& dispatcher, double dt) { return nullptr; }
	    virtual void		DrawUI() {}
		virtual UpdateTask* Draw(EngineCore& core, UpdateDispatcher& dispatcher, double dt, FrameGraph& frameGraph) { return nullptr; }
		virtual bool		EventHandler(Event& evt) { return false; }

		static struct IRenderSystem&	GetRenderSystem();
		static struct iAllocator&		GetAllocator();
		static struct iAllocator&		GetAllocatorMT();
		static struct iAllocator&		GetTempAllocator();
		static struct iAllocator&		GetTempAllocatorMT();
		static struct ThreadManager&	GetThreads();
		static struct IRenderWindow&	GetRenderWindow();
		static		  uint2				GetWH();
		static struct MouseInputState&	GetMouseState();

		static void						ToggleMouse(bool);

		static struct GameObject&		AllocateGameObject();
		static void						ReleaseGameObject(GameObject&);
    };

	struct ExampleApplication;
	struct ExampleDescription
	{
		uint2 WH = { 800, 600 };
		const char* windowName = "Example";
	};

    void	InitiateExampleFramework(const ExampleDescription&);
	int		RunExampleApplication();
	void	ReleaseExampleApplication();
    void	SetExampleState(std::unique_ptr<ExampleState>);

    template<typename TY> requires (std::is_base_of_v<ExampleState, TY>)
    int RunExample(const ExampleDescription desc = ExampleDescription{})
    {
		InitiateExampleFramework(desc);

		auto state = std::make_unique<TY>();
		SetExampleState(std::move(state));
		int rc = RunExampleApplication();
		ReleaseExampleApplication();

		return rc;
    }
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
