#pragma once

#include "MemoryUtilities.hpp"
#include "RenderSystemInterface.hpp"
#include "Scene.hpp"
#include "WorldRender.hpp"

namespace FlexKit
{   /************************************************************************************************/


    class iHLAS
    {

    };


    class iBLAS
    {

    };


    /************************************************************************************************/


    class iRayTracer
    {
    public:
        virtual ~iRayTracer() {}

        virtual void Release() = 0;
        virtual void Update(const SceneDescription& scene) = 0;
    };


    /************************************************************************************************/


    class NullRayTracer : public iRayTracer
    {
    public:
        NullRayTracer() {}

        void Release() override {}
        void Update(const SceneDescription& scene) override {}
    };


    /************************************************************************************************/


    class RTX_RayTracer : public iRayTracer
    {
    public:
        RTX_RayTracer(RenderSystem& renderSystem, iAllocator* allocator)
        {
        }

        ~RTX_RayTracer()
        {

        }

        void Release() override {}
        void Update(const SceneDescription& scene) override {}


    private:
        void    LoadLibraries();
    };


}   /************************************************************************************************/
