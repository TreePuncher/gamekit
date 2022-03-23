#pragma once

#include <type.h>
#include <any>
#include <optional>

using SelectionTypeID = std::uint32_t;

constexpr SelectionTypeID ViewportObjectList_ID = GetCRCGUID(ViewportObjectList);
constexpr SelectionTypeID AnimatorObject_ID     = GetCRCGUID(AnimatorObject);

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

