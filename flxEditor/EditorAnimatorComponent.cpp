#include "PCH.h"
#include "AnimationComponents.h"
#include "EditorAnimatorComponent.h"
#include "EditorInspectorView.h"
#include "EditorProject.h"
#include "EditorViewport.h"


/************************************************************************************************/

FlexKit::Blob EditorAnimatorComponent::GetBlob()
{
    return {};
}

/************************************************************************************************/


class SkeletonInspector : public IComponentInspector
{
public:
    SkeletonInspector(EditorProject& IN_project, EditorViewport& IN_viewport)
        : viewport  { IN_viewport }
        , project   { IN_project }{}


    FlexKit::ComponentID ComponentID() override
    {
        return FlexKit::SkeletonComponentID;
    }

    void Inspect(ComponentViewPanelContext& panelCtx, FlexKit::GameObject&, FlexKit::ComponentViewBase& component) override
    {
        panelCtx.PushVerticalLayout("Skeleton", true);
        panelCtx.AddText("TODO: ME!");
        panelCtx.Pop();
    }

    EditorViewport& viewport;
    EditorProject& project;
};


struct SkeletonFactory : public IComponentFactory
{
    FlexKit::ComponentViewBase& Construct(ViewportGameObject& viewportObject, ViewportScene& scene)
    {
        if (!viewportObject.gameObject.hasView(FlexKit::SkeletonComponentID))
            return viewportObject.gameObject.AddView<FlexKit::SkeletonView>(FlexKit::GetBrush(viewportObject.gameObject)->MeshHandle, -1u);
        else
            return *viewportObject.gameObject.GetView(FlexKit::SkeletonComponentID);
    }

    inline static const std::string name = "Skeleton";

    const std::string&      ComponentName() const noexcept { return name; }
    FlexKit::ComponentID    ComponentID()   const noexcept { return FlexKit::SkeletonComponentID; }

    static bool Register()
    {
        EditorInspectorView::AddComponentFactory(std::make_unique<SkeletonFactory>());

        return true;
    }

    inline static bool _registered = Register();
};



/**********************************************************************

Copyright (c) 2021 - 2022 Robert May

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
