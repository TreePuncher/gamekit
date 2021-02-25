#include "stdafx.h"
#include "Animation.h"


namespace FlexKit::ResourceBuilder
{   /************************************************************************************************/


    JointHandle GetJoint(const JointList& joints, const char* ID)
    {
        for (size_t I = 0; I < joints.size(); ++I)
            if (joints[I].Joint.mID == ID)
                return (JointHandle)I;

        return InvalidHandle_t;
    }


    /************************************************************************************************/


    MetaDataList GetAllAnimationClipMetaData(const MetaDataList& metaDatas)
    {
        return FilterList(
                [](auto metaData) 
                { 
                    return (metaData->type == MetaData::EMETAINFOTYPE::EMI_SKELETALANIMATION); 
                }, 
                metaDatas);
    }


    /************************************************************************************************/


    CutList GetAnimationCuts(const MetaDataList& metaData, const std::string& ID)
    {
        auto related	= FindRelatedMetaData(metaData, MetaData::EMETA_RECIPIENT_TYPE::EMR_SKELETALANIMATION, ID);

        CutList out     = transform_stl(
            GetAllAnimationClipMetaData(related),
            [](auto& ClipMetaData) -> AnimationCut
            {
                auto clip = std::static_pointer_cast<SkeletalAnimationClip_MetaData> (ClipMetaData);
                return {
                    clip->T_Start,
                    clip->T_End,
                    clip->ClipID,
                    clip->guid,
                    { clip } };
            });

        return out;
    }


    /************************************************************************************************/


    std::shared_ptr<Skeleton_MetaData> GetSkeletonMetaData(const MetaDataList& metaDatas)
    {
        for (auto metaData : metaDatas)
            if (metaData->type == MetaData::EMETAINFOTYPE::EMI_SKELETAL)
                return std::static_pointer_cast<Skeleton_MetaData>(metaData);

        return nullptr;
    }


}   /************************************************************************************************/


/**********************************************************************

Copyright (c) 2015 - 2012 Robert May

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

