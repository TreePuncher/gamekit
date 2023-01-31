#include "PCH.h"
#include "ModelViewerWidget.h"
#include "DXRenderWindow.h"
#include "EditorRenderer.h"

ModelViewerWidget::ModelViewerWidget(EditorRenderer& IN_renderer, QWidget* parent) :
	QWidget         { parent },
	menuBar         { new QMenuBar },
	renderer        { IN_renderer },
	renderWindow    { *IN_renderer.CreateRenderWindow() },
	gbuffer         {{ 400, 300 }, IN_renderer.framework.GetRenderSystem() },
	depthBuffer     {IN_renderer.framework.GetRenderSystem().CreateDepthBuffer({ 400, 300 }, true) }
{
	ui.setupUi(this);

	auto layout = findChild<QBoxLayout*>("verticalLayout");
	layout->setContentsMargins(0, 0, 0, 0);
	layout->setMenuBar(menuBar);
	layout->addWidget(&renderWindow);

	renderWindow.SetOnDraw(
		[&](FlexKit::UpdateDispatcher& dispatcher, double dT, TemporaryBuffers& temporaries, FlexKit::FrameGraph& frameGraph, FlexKit::ResourceHandle renderTarget, FlexKit::ThreadSafeAllocator& allocator)
		{
			OnDraw(dispatcher, dT, temporaries, frameGraph, renderTarget);
		});

	renderWindow.SetOnResize(
		[&](FlexKit::uint2 WH)
		{
			//gbuffer.Resize(WH);
			//renderer.framework.GetRenderSystem().ReleaseResource(depthBuffer);
			//depthBuffer = IN_renderer.framework.GetRenderSystem().CreateDepthBuffer(WH, true);
		}
	);
}


/************************************************************************************************/


ModelViewerWidget::~ModelViewerWidget()
{
}


/************************************************************************************************/


void ModelViewerWidget::OnDraw(FlexKit::UpdateDispatcher& Dispatcher, double dT, TemporaryBuffers&, FlexKit::FrameGraph& frameGraph, FlexKit::ResourceHandle renderTarget)
{
	FlexKit::ClearGBuffer(frameGraph, gbuffer);
	FlexKit::ClearBackBuffer(frameGraph, depthBuffer, { 0, 1, 0, 1 });
}


/************************************************************************************************/
