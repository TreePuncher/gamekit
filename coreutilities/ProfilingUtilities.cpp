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

#include "ProfilingUtilities.h"

#include <imgui.h>
#include <Windows.h>
#include <iostream>

namespace FlexKit
{

    void EngineProfiling::DrawProfiler(iAllocator& temp)
    {
        if (auto stats = pausedFrame ? pausedFrame : profiler.GetStats(); stats)
        {
            if (ImGui::Begin("Profiler"))
            {
                if (ImGui::Button("Pause"))
                {
                    if (pausedFrame)
                        pausedFrame = nullptr;
                    else
                        pausedFrame = stats;
                }

                ImGui::SameLine();

                if (ImGui::Button("Toggle Labels"))
                    showLabels = !showLabels;

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
                    const double fDuration  = double(duration.count()) / 100000000.0;

                    const int   maxDepth    = 4;
                    const float barWidth    = 25.0f;
                    const float areaH       = stats->Threads.size() * maxDepth * barWidth;

                    const ImVec2 windowPOS  = ImGui::GetWindowPos();
                    const ImVec2 windowSize = ImGui::GetWindowSize();

                    struct imDrawText
                    {
                        ImVec2      pos;
                        ImColor     color;
                        const char* string;
                    };

                    Vector<imDrawText> textstack{ &temp };
                    size_t drawCount = 0;

                    if(ImGui::BeginChild(GetCRCGUID("PROFILEGRAPH" + threadID++), ImVec2(windowSize.x, areaH)))
                    {
                        const auto contentBegin = ImGui::GetCursorScreenPos();

                        uint32_t threadOffsetCounter = 0;

                        for (size_t threadID = 0; threadID < stats->Threads.size(); threadID++)
                        {
                            auto& thread = stats->Threads[threadID];
                            auto& profilings = thread.timePoints;

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
                            };

                            draw_list->AddRectFilled(threadBoxMin, threadBoxMax, colors[threadOffsetCounter % 2], 0);


                            if (pMax.y > 0.0f)
                            {
                                auto VisitChildren =
                                    [&](uint64_t nodeID, auto& _Self, uint32_t maxDepth, uint32_t currentDepth) -> void
                                    {
                                        FrameTiming& node = GetChild(nodeID);
                                        // Render Current Profile Sample
                                        const float fbegin  = node.GetRelativeTimePointBegin(begin, duration);
                                        const float fend    = node.GetRelativeTimePointEnd(begin, duration);

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

                                            //ImGui::CalcTextSize();

                                            if(showLabels || ImGui::IsMouseHoveringRect(pMin, pMax, true))
                                                textstack.emplace_back(
                                                    pTxt,
                                                    textColor,
                                                    node.Function
                                                );
                                        }
                                    };

                                for (auto profile : profilings)
                                    if (profile.parentID == uint64_t(-1))
                                        VisitChildren(profile.profileID, VisitChildren, maxDepth, 0);
                            }
                        }


                        for(auto& txt : textstack)
                            draw_list->AddText(txt.pos, txt.color, txt.string);

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
    }


}
