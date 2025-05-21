#include <Application.hpp>
#include <Win32Graphics.hpp>

namespace Experiments
{
	using namespace FlexKit;

	class RTExperiment : public FrameworkState
	{
	public:
		RTExperiment(GameFramework& framework) : FrameworkState{ framework }
		{
		}

		RenderWindow* renderWindow;
	};
}
