#include "ProfilingUtilities.h"

#include <imgui.h>
#include <Windows.h>
#include <iostream>
#include "MathUtils.h"
#include "fmt/format.h"

namespace FlexKit
{

    void ThreadProfiler::Push(const char* func, uint64_t Id, TimePoint tp)
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


    void ThreadProfiler::Pop(uint64_t Id, TimePoint tp)
    {
        auto& frames = activeFrames;

        if (!frames.size() || Id != frames.back().profileID) // Discard, frame changed
            return;

        auto currentFrame = frames.back();
        frames.pop_back();

        currentFrame.end = tp;
        completedFrames.push_back(currentFrame);
    }


    void EngineProfiling::DrawProfiler(uint2 POS, uint2 WH, iAllocator& temp)
    {
#if USING(ENABLEPROFILER)
        if (auto stats = profiler.GetStats(); stats)
        {
            if (ImGui::Begin("Profiler"))
            {
                ImGui::SetWindowPos({ (float)POS[0], (float)POS[1] });
                ImGui::SetWindowSize({ (float)WH[0], (float)WH[1] });

                static int maxDepth = 15;

                if (ImGui::Button("Pause"))
                    paused = !paused;

                ImGui::SameLine();

                if (ImGui::Button("Next"))
                    frameOffset = clamp(size_t(0), frameOffset + 1, this->stats.size() - 1);

                ImGui::SameLine();

                if (ImGui::Button("Previous"))
                    frameOffset = clamp(size_t(0), frameOffset - 1, this->stats.size() - 1);

                ImGui::SameLine();

                if (ImGui::Button("Toggle Labels"))
                    showLabels = !showLabels;

                ImGui::SameLine();

                ImGui::SliderInt("Stack Depth", &maxDepth, 1, 15);

                static float beginRange    = 0.0f;
                static float endRange      = 1.0f;


                //ImGui::SliderFloat("Begin", &beginRange, 0, 1);
                //ImGui::SliderFloat("End", &endRange, 0, 1);
                ImGui::DragFloatRange2("Range", &beginRange,&endRange, 0.01, 0, 1);

                const float range = endRange - beginRange;

                ImDrawList* draw_list       = ImGui::GetWindowDrawList();
                const uint32_t threadCount  = (uint32_t)stats->Threads.size();
                const float scrollY         = ImGui::GetScrollY();

                ImGui::CalcItemWidth();
                ImGui::GetFrameHeight();

                if (stats)
                {
                    TimePoint begin = TimePoint::max();
                    TimePoint end = TimePoint::min();

                    for (auto& thread : stats->Threads)
                    {
                        for (const auto sample : thread.timePoints)
                        {
                            begin = min(sample.begin, begin);
                            end = max(sample.end, end);
                        }
                    }

                    const auto duration     = end - begin;
                    const double fDuration  = double(duration.count()) / 1000000.0;

                    const float barWidth    = 25.0f;
                    const float areaH       = stats->Threads.size() * maxDepth * barWidth;

                    const ImVec2 windowPOS  = ImGui::GetWindowPos();
                    const ImVec2 windowSize = ImGui::GetWindowSize();

                    ImGui::SameLine();
                    ImGui::Text(fmt::format("Capture duration {}ms", fDuration).c_str());

                    struct imDrawText
                    {
                        ImVec2      pos;
                        ImColor     color;
                        std::string string;
                    };

                    Vector<imDrawText> textstack{ &temp };
                    size_t drawCount = 0;

                    if(ImGui::BeginChild(GetCRCGUID("PROFILEGRAPH" + threadID++), ImVec2(windowSize.x, areaH/2)))
                    {
                        const auto contentBegin = ImGui::GetCursorScreenPos();

                        uint32_t threadOffsetCounter = 0;

                        for (size_t threadID = 0; threadID < stats->Threads.size(); threadID++)
                        {
                            auto& thread        = stats->Threads[threadID];
                            auto& profilings    = thread.timePoints;

                            if (!profilings.size())
                                continue;

                            ImColor color(1.0f, 1.0f, 1.0f, 1.0f);

                            auto GetChild = [&](uint64_t childID) -> FrameTiming&
                            {
                                for (auto& timeSample : profilings)
                                    if (timeSample.profileID == childID)
                                        return timeSample;

                                FK_ASSERT(0);
                            };


                            const float threadOffset = threadOffsetCounter++ * barWidth * maxDepth;


                            const ImVec2 pMin = ImVec2{ contentBegin.x,                  contentBegin.y + barWidth + threadOffset - scrollY };
                            const ImVec2 pMax = ImVec2{ contentBegin.x + windowSize.x,   contentBegin.y + maxDepth * barWidth + threadOffset - scrollY };

                            const ImVec2 threadBoxMin = ImVec2{ contentBegin.x,                  contentBegin.y + threadOffset - scrollY };
                            const ImVec2 threadBoxMax = ImVec2{ contentBegin.x + windowSize.x,   contentBegin.y + maxDepth * barWidth + threadOffset - scrollY };

                            static const ImColor colors[] = {
                                ImColor{11, 173, 181},
                                ImColor{73, 127, 130},
                                ImColor{56, 75,  65},
                            };

                            draw_list->AddRectFilled(threadBoxMin, threadBoxMax, colors[threadOffsetCounter % 2], 0);

                            if (pMax.y > 0.0f)
                            {
                                auto VisitChildren =
                                    [&](uint64_t nodeID, auto& _Self, uint32_t maxDepth, uint32_t currentDepth) -> void
                                    {
                                        FrameTiming& node = GetChild(nodeID);
                                        // Render Current Profile Sample
                                        const float fbegin  = node.GetRelativeTimePointBegin(begin, duration) / range - beginRange / range;
                                        const float fend    = node.GetRelativeTimePointEnd(begin, duration) / range - beginRange / range;

                                        const ImVec2 pMin = ImVec2{
                                            contentBegin.x + windowSize.x * fbegin,
                                            contentBegin.y + (currentDepth + 0) * barWidth + threadOffset - scrollY };

                                        const ImVec2 pMax = ImVec2{
                                            contentBegin.x + windowSize.x * fend,
                                            contentBegin.y + (currentDepth + 1) * barWidth + threadOffset - scrollY };

                                        if (pMax.y > 0.0f)
                                        {
                                            static const ImColor colors[] = {
                                                {37, 232, 132},
                                                {235, 96, 115},
                                                {181, 11, 119},
                                            };
                                            draw_list->AddRectFilled(pMin, pMax, colors[drawCount++ % 3], 0);

                                            const ImVec2 pTxt = ImVec2{
                                                contentBegin.x + windowSize.x * fbegin,
                                                contentBegin.y + currentDepth * barWidth + threadOffset - scrollY };

                                            const ImColor textColor(0.0f, 0.0f, 0.0f, 1.0f);


                                            // Visit Child Samples
                                            if (currentDepth + 1 < maxDepth)
                                                for (uint64_t childID : node.children)
                                                    _Self(childID, _Self, maxDepth, currentDepth + 1);


                                            auto txt        = fmt::format("{} [ {}.ms ]", node.Function, node.GetDurationDouble() * 100);
                                            auto txtSize    = ImGui::CalcTextSize(txt.c_str());

                                            if ((showLabels && txtSize.x <= (pMax.x - pMin.x)))
                                            {
                                                textstack.emplace_back(
                                                    pTxt,
                                                    textColor,
                                                    txt.c_str()
                                                );
                                            }
                                            else if (ImGui::IsMouseHoveringRect(pMin, pMax, true))
                                            {
                                                textstack.emplace_back(
                                                    pTxt,
                                                    textColor,
                                                    fmt::format("{} [ {}.ms ]", node.Function, node.GetDurationDouble() * 100)
                                                );

                                                auto parentNode = node.parentID;
                                                for (size_t I = 1; parentNode != uint64_t(-1); I++)
                                                {
                                                    auto& parentNode_ref = GetChild(parentNode);

                                                    const float fbegin  = parentNode_ref.GetRelativeTimePointBegin(begin, duration) / range - beginRange / range;
                                                    const float fend    = parentNode_ref.GetRelativeTimePointEnd(begin, duration) / range - beginRange / range;

                                                    const ImVec2 pTxt = ImVec2{
                                                        contentBegin.x + windowSize.x * fbegin,
                                                        contentBegin.y + (currentDepth - I) * barWidth + threadOffset - scrollY };

                                                    const auto txt      = fmt::format("{} [ {}.ms ]", parentNode_ref.Function, parentNode_ref.GetDurationDouble() * 100);
                                                    const auto size     = windowSize.x * (fend - fbegin);
                                                    const auto txtSize  = ImGui::CalcTextSize(txt.c_str());

                                                    if (txtSize.x > size)
                                                        textstack.emplace_back(
                                                            pTxt,
                                                            textColor,
                                                            txt.c_str());

                                                    parentNode = parentNode_ref.parentID;
                                                }
                                            }
                                        }
                                    };

                                for (auto profile : profilings)
                                    if (profile.parentID == uint64_t(-1))
                                        VisitChildren(profile.profileID, VisitChildren, maxDepth, 0);
                            }
                        }


                        for(auto& txt : textstack)
                            draw_list->AddText(txt.pos, txt.color, txt.string.c_str());

                        ImGui::EndChild();
                    }
                }
                else
                {
                    ImGui::Text("No Profiling Stats Available!");
                }
            }

            ImGui::End();
        }
#endif
    }


}


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
