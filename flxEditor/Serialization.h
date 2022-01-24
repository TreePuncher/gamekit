#pragma once

#include "MathUtils.h"
#include "Serialization.hpp"

namespace FlexKit
{

    namespace serialization
    {
        template<class Archive>
        void serialize(Archive& ar, FlexKit::uint2& g, const unsigned int version)
        {
            ar& g[0];
            ar& g[1];
        }

        template<class Archive>
        void serialize(Archive& ar, FlexKit::uint3& g, const unsigned int version)
        {
            ar& g[0];
            ar& g[1];
            ar& g[2];
        }

        template<class Archive>
        void serialize(Archive& ar, FlexKit::float2& g, const unsigned int version)
        {
            ar& g[0];
            ar& g[1];
        }

        template<class Archive>
        void serialize(Archive& ar, FlexKit::float3& g, const unsigned int version)
        {
            ar& g[0];
            ar& g[1];
            ar& g[2];
        }

        template<class Archive>
        void serialize(Archive& ar, FlexKit::float4& g, const unsigned int version)
        {
            ar& g[0];
            ar& g[1];
            ar& g[2];
            ar& g[3];
        }

        template<class Archive>
        void serialize(Archive& ar, FlexKit::Quaternion& q, const unsigned int version)
        {
            ar& q[0];
            ar& q[1];
            ar& q[2];
            ar& q[3];
        }

        template<class Archive>
        void serialize(Archive& ar, FlexKit::uint4_16& q, const unsigned int version)
        {
            ar& q[0];
            ar& q[1];
            ar& q[2];
            ar& q[3];
        }
    } // namespace serialization
} // namespace boost
