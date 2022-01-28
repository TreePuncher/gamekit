#ifndef ANIMATION_H
#define ANIMATION_H

#include "buildsettings.h"
#include "Common.h"
#include "MetaData.h"
#include "ResourceUtilities.h"
#include "XMMathConversion.h"


namespace FlexKit
{   /************************************************************************************************/


    using FlexKit::Joint;
    using FlexKit::JointPose;
    using FlexKit::JointHandle;

    using FlexKit::GUID_t;

    using namespace FlexKit;

    struct SkeletonJointAnimation
    {
        struct Pose
        {
            JointPose	JPose;
            double		T;
            double		_PAD;
        };

        std::vector<Pose> Poses;

        size_t FPS;
        size_t FrameCount;
        size_t _PAD;
    };


    /************************************************************************************************/


    struct SkeletonJoint
    {
        std::string         mID;
        JointHandle			mParent     = JointHandle(0);
        float               limbLength  = 0.0f;
    };


    /************************************************************************************************/


    struct SkeletonJointInfo
    {
        SkeletonJoint           Joint;
        SkeletonJointAnimation	Animation;
        XMMATRIX		        Inverse;
    };


    /************************************************************************************************/


    struct AnimationCut
    {
        double		    T_Start;
        double		    T_End;
        std::string	    ID;
        GUID_t		    guid;
        MetaDataList    metaData;
    };


    /************************************************************************************************/


    struct SkeletonResource;

    using JointList				= std::vector<SkeletonJointInfo>;
    using CutList				= std::vector<AnimationCut>;
    using SkeletonResource_ptr	= std::shared_ptr<SkeletonResource>;


    /************************************************************************************************/


    struct AnimationTrackTarget
    {
        std::string Channel;
        std::string Target;

        TrackType   type;
    };


    void Serialize(auto& ar, AnimationTrackTarget& track)
    {
        ar& track.Channel;
        ar& track.Target;
        ar& track.type;
    }


    struct AnimationResource :
        public Serializable<AnimationResource, iResource, GetTypeGUID(AnimationResource)>
    {
        void Serialize(auto& ar)
        {
            ar& ID;
            ar& guid;
            ar& tracks;
        }

        ResourceBlob        CreateBlob();

        const std::string&  GetResourceID()     const noexcept { return ID; }
        const uint64_t      GetResourceGUID()   const noexcept { return guid; }
        const ResourceID_t  GetResourceTypeID() const noexcept { return AnimationResourceTypeID; }

        struct Track
        {
            AnimationTrackTarget            targetChannel;
            std::vector<AnimationKeyFrame>  KeyFrames;
        };


        std::string                 ID;
        GUID_t                      guid = rand();
        std::vector<Track>          tracks;
    };


    void Serialize(auto& ar, AnimationResource::Track& track)
    {
        ar& track.targetChannel;
        ar& track.KeyFrames;
    }


    /************************************************************************************************/


    void Serialize(auto& ar, SkeletonJoint& joint)
    {
        ar& joint.mID;
        ar& joint.mParent;
        ar& joint.limbLength;
    }


    void Serialize(auto& ar, JointPose& pose)
    {
        ar& pose.r;
        ar& pose.ts;
    }

    struct SkeletonResource :
        public Serializable<SkeletonResource, iResource, GetTypeGUID(SkeletonResource)>
    {

        void Serialize(auto& ar)
        {
            ar& JointCount;
            ar& guid;
            ar& ID;
            ar& IPoses;
            ar& joints;
            ar& jointIDs;
            ar& jointPoses;
            ar& metaData;
        }

        ResourceBlob CreateBlob() override;


        JointHandle	FindJoint(const std::string& id) const noexcept;

        void AddJoint(const SkeletonJoint joint, const XMMATRIX IPose);
        void SetJointID(JointHandle joint, std::string& ID);

        const std::string&  GetResourceID()     const noexcept override { return ID; }
        const uint64_t      GetResourceGUID()   const noexcept override { return guid; }
        const ResourceID_t  GetResourceTypeID() const noexcept override { return SkeletonResourceTypeID; }

        size_t								JointCount;
        GUID_t								guid;
        std::string                         ID;

        std::vector<DirectX::XMMATRIX>		IPoses; // Global Inverse Space Pose
        std::vector<SkeletonJoint>			joints;
        std::vector<std::string>			jointIDs;
        std::vector<JointPose>				jointPoses;
        MetaDataList                        metaData;
    };


    /************************************************************************************************/


    struct AnimationProperties
    {
        double frameRate = 60;
    };


    /************************************************************************************************/


    CutList				    GetAnimationCuts			(const MetaDataList& MD,  const std::string& id);
    JointHandle             GetJoint					(const JointList& joints, const char* ID);
    SkeletonMetaData_ptr    GetSkeletonMetaData		    (const MetaDataList& metaDatas);


}   /************************************************************************************************/


/**********************************************************************

Copyright (c) 2015 - 2021 Robert May

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


#endif
