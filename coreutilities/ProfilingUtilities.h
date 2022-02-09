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

#ifndef PROFILINGUTILITIES
#define PROFILINGUTILITIES

#include "buildsettings.h"
#include "containers.h"
#include "type.h"

#include <chrono>
#include <mutex>
#include <memory>
#include <source_location>


/************************************************************************************************/


namespace FlexKit
{
    using TimePoint     = std::chrono::high_resolution_clock::time_point;
    using TimeDuration  = std::chrono::high_resolution_clock::duration;


    struct FrameTiming
    {
        const char* Function;
        uint64_t    profileID;
        uint64_t    parentID;
        TimePoint   begin;
        TimePoint   end;

        TimeDuration GetDuration() const
        {
            return end - begin;
        }

        double GetDurationDouble()
        {
            auto duration = end - begin;
            auto fDuration = double(duration.count()) / 100000000.0;

            return fDuration;
        }

        double GetRelativeTimePointBegin(const TimePoint rel_begin, const TimeDuration duration)
        {
            auto relativeTimePoint = double((begin - rel_begin).count()) / double(duration.count());

            return relativeTimePoint;
        }

        double GetRelativeTimePointEnd(const TimePoint rel_begin, const TimeDuration duration)
        {
            auto relativeTimePoint = double((end - rel_begin).count()) / double(duration.count());

            return relativeTimePoint;
        }

        static_vector <uint64_t> children;
    };


    struct ThreadStats
    {
        std::vector<FrameTiming> timePoints;
    };


	struct ThreadProfiler
	{
        ThreadProfiler()
        {
            activeFrames.reserve(1024);
            completedFrames.reserve(1024);
        }

        void BeginFrame()
        {
            std::scoped_lock lock(m);

            activeFrames.clear();
            completedFrames.clear();
        }

        void EndFrame()
        {
            std::scoped_lock lock(m);

            for (auto& frame : activeFrames)
                Pop(frame.profileID, std::chrono::high_resolution_clock::now());

            std::sort(
                completedFrames.begin(),
                completedFrames.end(),
                [](FrameTiming& lhs, FrameTiming& rhs)
                {
                    return lhs.begin < rhs.begin;
                });
        }

        ThreadStats GetStats()
        {
            if (completedFrames.size())
            {
                std::vector<FrameTiming> temp{ std::move(completedFrames) };
                completedFrames.reserve(1024);
                return { std::move(temp) };
            }
            else return {};
        }

        void Clear()
        {
            activeFrames.clear();
            completedFrames.clear();

            activeFrames.reserve(1024);
            completedFrames.reserve(1024);
        }

        void Push(const char* func, uint64_t Id, TimePoint tp);
        void Pop(uint64_t Id, TimePoint tp);

        void Release()
        {
            activeFrames.clear();
            completedFrames.clear();

            activeFrames.reserve(1024);
            completedFrames.reserve(1024);
        }

        std::vector<FrameTiming> activeFrames{};
        std::vector<FrameTiming> completedFrames{};
        std::mutex              m;
	};


    class ProfilingStats
    {
    public:

        std::vector<ThreadStats> Threads;
    };

    struct EngineProfiling
    {
        ThreadProfiler& GetThreadProfiler()
        {
            std::scoped_lock lock{ m };

            threadProfilers.emplace_back(std::make_unique<ThreadProfiler>());

            return *threadProfilers.back().get();
        }

        void BeginFrame()
        {
#if USING(ENABLEPROFILER)
            for (auto& threadProfiler : threadProfilers)
                threadProfiler->BeginFrame();
#endif
        }

        void EndFrame()
        {
#if USING(ENABLEPROFILER)
            for (auto& threadProfiler : threadProfilers)
                threadProfiler->EndFrame();

            auto frameStats = std::make_shared<ProfilingStats>();

            for (auto& threadProfiler : threadProfilers)
                frameStats->Threads.push_back(threadProfiler->GetStats());

            if (!paused)
                stats.push_back(std::move(frameStats));
#endif
        }

        void Release()
        {
            for (auto& threadProfiler : threadProfilers)
                threadProfiler->Release();

            stats.Release();
        }

        std::shared_ptr<ProfilingStats>  GetStats()
        {
            if(stats.size())
                return stats[stats.size() - 1 - frameOffset];
            else
                return {};
        }

        void DrawProfiler(iAllocator& temp);

        std::mutex m;

        bool paused     = false;
        bool showLabels = true;
        size_t frameOffset = 0;

        std::shared_ptr<ProfilingStats>                     pausedFrame;
        CircularBuffer<std::shared_ptr<ProfilingStats>>     stats;
        std::vector<std::unique_ptr<ThreadProfiler>>        threadProfilers;
    };

    inline EngineProfiling profiler;

	
    template<typename TY>
    FLEXKITAPI decltype(auto) _TimeBlock(TY function, const char* id)
    {
        static const std::chrono::high_resolution_clock Clock;
        const auto Before = Clock.now();

        EXITSCOPE(
            const auto After = Clock.now();
            const auto Duration = std::chrono::duration_cast<std::chrono::microseconds>(After - Before);
        FK_LOG_0("Function %s executed in %umicroseconds.", id, Duration.count()); );

        return function();
    }

    inline thread_local ThreadProfiler& threadProfiler = profiler.GetThreadProfiler();

    class _ProfileFunction
    {
    public:
        _ProfileFunction(const char* FunctionName, uint64_t IN_profileID) :
            function    { FunctionName },
            profileID   { IN_profileID + rand() }
        {
            threadProfiler.Push(FunctionName, profileID, std::chrono::high_resolution_clock::now());
        }

        ~_ProfileFunction()
        {
            threadProfiler.Pop(profileID, std::chrono::high_resolution_clock::now());
        }

    private:
        std::chrono::high_resolution_clock::time_point before = std::chrono::high_resolution_clock::now();
        const char*     function = "Unnamed function";
        const uint64_t  profileID;
    };

#define PROFILELABEL_(a) MERGECOUNT_(PROFILE_ID_, a)
#define PROFILELABEL PROFILELABEL_(__LINE__)

#define _STRINGIFY(A) #A
#define STRINGIFY(A) _STRINGIFY(A)

#define GETLINEHASH(A) FlexKit::GenerateTypeGUID<sizeof(A)>(A)

#if USING(ENABLEPROFILER)
#define ProfileFunction()       const auto PROFILELABEL_ = _ProfileFunction(std::source_location::current().function_name().c_str() , GETLINEHASH(STRINGIFY(__LINE__) __FUNCTION__ ))
#define ProfileFunction(LABEL)  const auto PROFILELABEL_##LABEL = _ProfileFunction(__FUNCTION__":"#LABEL, GETLINEHASH(STRINGIFY(__LINE__) __FUNCTION__ ))
#else
#define ProfileFunction()
#endif
#define TIMEBLOCK(A, B) _TimeBlock([&]{ return A(); }, B)


    template<typename TY>
    FLEXKITAPI decltype(auto) TimeFunction(TY&& function)
    {
        static const std::chrono::high_resolution_clock Clock;
        const auto Before = Clock.now();

        function();

        const auto After = Clock.now();

        return After - Before;
    }
}

/************************************************************************************************/

#endif // ! PROFILINGUTILITIES
