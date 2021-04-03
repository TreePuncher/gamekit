#pragma once

#include "MathUtils.h"

namespace boost {
    namespace serialization {

        template<class Archive>
        void serialize(Archive& ar, FlexKit::uint2& g, const unsigned int version)
        {
            ar& g[0];
            ar& g[1];
        }

    } // namespace serialization
} // namespace boost
