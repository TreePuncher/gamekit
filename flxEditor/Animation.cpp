#include "PCH.h"
#include "Animation.h"


namespace FlexKit
{   /************************************************************************************************/


    JointHandle GetJoint(const JointList& joints, const char* ID)
    {
        for (size_t I = 0; I < joints.size(); ++I)
            if (joints[I].Joint.mID == ID)
                return (JointHandle)I;

        return InvalidHandle;
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


    /************************************************************************************************/


    ResourceBlob AnimationResource::CreateBlob() const
    {
        AnimationResourceBlob::AnimationResourceHeader header = {
            .Type       = EResourceType::EResource_Animation,
            .GUID       = (uint64_t)guid,
            .trackCount = (uint32_t)tracks.size(),
        };

        strncpy_s(header.ID, ID.c_str(), ID.size());

        Blob resourceBlob;

        for (auto& track : tracks)
        {
            Blob keyFrameBlob;

            for (auto& frame : track.KeyFrames)
                keyFrameBlob += Blob{ frame };

            AnimationTrackHeader trackHeader = {
                .frameCount = (uint32_t)track.KeyFrames.size(),
                .byteSize   = (uint32_t)(sizeof(trackHeader) + keyFrameBlob.size()),
            };

            auto& target    = track.targetChannel.Target;
            auto& trackName = track.targetChannel.Channel;
            strncpy_s(trackHeader.target,       target.c_str(),     target.size());
            strncpy_s(trackHeader.trackName,    trackName.c_str(),  trackName.size());

            resourceBlob += Blob{ trackHeader } + keyFrameBlob;
        }

        header.ResourceSize = sizeof(header) + resourceBlob.size();

        ResourceBlob out;
        auto [buffer_ptr, bufferSize] = (Blob{ header } + resourceBlob).Release();
        out.buffer          = (char*)buffer_ptr;
        out.bufferSize      = bufferSize;
        out.resourceType    = EResourceType::EResource_Animation;
        out.GUID            = guid;
        out.ID              = ID;

        return out;
    }


    /************************************************************************************************/


    ResourceBlob SkeletonResource::CreateBlob() const
    {
        Blob jointBlob;
        for (size_t jointIdx = 0; jointIdx < joints.size(); ++jointIdx)
        {
            SkeletonResourceBlob::JointEntry jointOut;
            auto& jointLinkage = joints[jointIdx];

                
            jointOut.IPose     = FlexKit::XMMatrixToFloat4x4(&IPoses[jointIdx]);
            jointOut.Parent    = jointLinkage.mParent;
            jointOut.Pose      = jointPoses[jointIdx];
            strncpy_s(jointOut.ID, jointLinkage.mID.c_str(), jointLinkage.mID.size());

            jointBlob += jointOut;
        }

        SkeletonResourceBlob::Header header;

        header.GUID         = guid;
        strncpy_s(header.ID, ID.c_str(), ID.size());
        header.JointCount   = joints.size();
        header.ResourceSize = sizeof(header) + jointBlob.size();
        header.Type         = EResourceType::EResource_Skeleton;

        Blob resource = Blob{ header } + jointBlob;
        ResourceBlob out;

        out.buffer          = (char*)malloc(resource.size());
        out.bufferSize      = resource.size();
        out.resourceType    = EResourceType::EResource_SkeletalAnimation;
        out.GUID            = guid;
        out.ID              = ID;

        memcpy(out.buffer, resource.data(), resource.size());

        return out;
    }


    /************************************************************************************************/


    JointHandle	SkeletonResource::FindJoint(const std::string& id) const noexcept
    {
        for (size_t itr = 0; itr < joints.size(); itr++)
        {
            auto& joint = joints[itr];

            if (joint.mID == id)
                return JointHandle(itr);
        }

        return InvalidHandle;
    }


    /************************************************************************************************/


    void SkeletonResource::AddJoint(const SkeletonJoint joint, const XMMATRIX IPose)
    {
        const auto parentIPose  = joint.mParent != InvalidHandle ? IPoses[joint.mParent] : DirectX::XMMatrixIdentity();
        const auto pose         = DirectX::XMMatrixInverse(nullptr, IPose);

        IPoses.push_back(IPose);
        jointPoses.push_back(GetPose(pose * parentIPose));
        joints.push_back(joint);
        jointIDs.push_back("");

        JointCount++;
    }


    /************************************************************************************************/


    void SkeletonResource::SetJointID(JointHandle joint, std::string& ID)
    {
        jointIDs[joint] = ID;
    }


}   /************************************************************************************************/


/**********************************************************************

Copyright (c) 2015 - 2022 Robert May

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

