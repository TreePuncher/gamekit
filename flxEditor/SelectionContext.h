#pragma once

#include "../FlexKitResourceCompiler/Common.h"
#include "EditorProject.h"

#include <any>
#include <optional>

using FlexKit::ResourceBuilder::ResourceList;

using SelectionTypeID = std::uint32_t;

constexpr SelectionTypeID ViewportObjectList_ID = GetCRCGUID(ViewportObjectList);

class SelectionContext
{
public:
    SelectionTypeID GetSelectionType() const
    {
        return type;
    }

    template<typename TY>
    auto GetSelection()
    {
        return std::any_cast<TY>(selection);
    }

    std::any        selection;
    SelectionTypeID type;
};

