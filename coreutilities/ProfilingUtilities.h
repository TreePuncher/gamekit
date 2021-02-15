/**********************************************************************

Copyright (c) 2015 - 2019 Robert May

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

        void BeginFrame()
        {
            activeFrames.clear();
            completedFrames.clear();
        }

        void EndFrame()
        {
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
            return { completedFrames };
        }

        void Push(const char* func, uint64_t Id, TimePoint tp)
        {
            const auto parentID = activeFrames.size() ? activeFrames.back().profileID : -1;

            if (activeFrames.size()) {
                auto& parent = activeFrames.back();
                parent.children.push_back(Id);
            }

            FrameTiming timing{ 
                .Function   = func,
                .profileID  = Id,
                .parentID   = parentID,
                .begin      = tp,
            };

            activeFrames.push_back(timing);
        }

        void Pop(uint64_t Id, TimePoint tp)
        {
            auto& frames = activeFrames;

            if(!frames.size() || Id != frames.back().profileID) // Discard, frame changed
                return;

            auto currentFrame = frames.back();
            frames.pop_back();

            currentFrame.end = tp;
            completedFrames.push_back(currentFrame);
        }

        std::vector<FrameTiming> activeFrames{ 128 };
        std::vector<FrameTiming> completedFrames{ 128 };
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
            for (auto& threadProfiler : threadProfilers)
                threadProfiler->BeginFrame();
        }

        void EndFrame()
        {
            for (auto& threadProfiler : threadProfilers)
                threadProfiler->EndFrame();

            auto frameStats = std::make_shared<ProfilingStats>();

            for (auto& threadProfiler : threadProfilers)
                frameStats->Threads.push_back(threadProfiler->GetStats());

            stats.push_back(std::move(frameStats));
        }

        std::shared_ptr<ProfilingStats>  GetStats()
        {
            if(stats.size())
                return stats.back();
            else
                return {};
        }

        void DrawProfiler();

        std::mutex m;

        bool showLabels = true;

        std::shared_ptr<ProfilingStats>                     pausedFrame;
        CircularBuffer<std::shared_ptr<ProfilingStats>>     stats;
        std::vector<std::unique_ptr<ThreadProfiler>>        threadProfilers;
    };

    inline EngineProfiling profiler;

	
    template<typename TY>
    FLEXKITAPI decltype(auto) _TimeBlock(TY function, const char* id)
    {
        std::chrono::high_resolution_clock Clock;
        auto Before = Clock.now();

        EXITSCOPE(
            auto After = Clock.now();
            auto Duration = std::chrono::duration_cast<std::chrono::microseconds>(After - Before);
        FK_LOG_0("Function %s executed in %umicroseconds.", id, Duration.count()); );

        return function();
    }

    inline thread_local ThreadProfiler& threadProfiler = profiler.GetThreadProfiler();

    class _ProfileFunction
    {
    public:
        _ProfileFunction(const char* FunctionName, uint64_t IN_profileID) :
            function    { FunctionName },
            profileID   { IN_profileID }
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

#define ProfileFunction() const auto PROFILELABEL_ = _ProfileFunction(__FUNCTION__, GETLINEHASH(STRINGIFY(__LINE__) __FUNCTION__))
#define TIMEBLOCK(A, B) _TimeBlock([&]{ return A(); }, B)
}

/************************************************************************************************/

#endif // ! PROFILINGUTILITIES
