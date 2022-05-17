#pragma once
#include "ModifiableShape.h"


namespace FlexKit
{   /************************************************************************************************/

    enum GregoryQuadPatchPoint
    {
        p0 = 0,
        p1 = 1,
        p2 = 2,
        p3 = 3,

        e0Minus = 4,
        e0Plus = 5,
        e1Minus = 6,
        e1Plus = 7,
        e2Minus = 8,
        e2Plus = 9,
        e3Minus = 10,
        e3Plus = 11,

        r0Minus = 12,
        r0Plus = 13,

        r1Minus = 14,
        r1Plus = 15,

        r2Minus = 16,
        r2Plus = 17,

        r3Minus = 18,
        r3Plus = 19
    };


    struct Patch
    {
        Patch()
        {
            memset(inputVertices, 0xff, sizeof(inputVertices));
        }

        struct ControlPointWeights
        {
            ControlPointWeights();

            void    Scale(float s);
            void    AddWeight(uint32_t idx, float w);
            void    ScaleWeight(uint32_t idx, float s);
            float   WeightSum() const;

            ControlPointWeights& operator += (const ControlPointWeights& rhs);

            float       weights[32] = { 0.0f };
            uint32_t    indices[32];

        } controlPoints[20];

        uint32_t C_0;
        uint32_t C_idx[8];


        uint32_t inputVertices[32];
        uint32_t vertCount = 0;
    };


    struct GPUVertex
    {
        float P[3];
    };


    struct QuadPatch
    {
        uint4 vertexIds;
    };


    struct VertexNeighorList
    {
        static_vector<uint32_t> patches;
    };


    struct GPUPatch
    {
        struct controlPoint
        {
            uint16_t weights[32];
        } controlPoints[20];

        uint Pn;
    };


    struct PatchGroups
    {
        std::vector<uint32_t> QuadPatches;
        std::vector<uint32_t> TriPatches;

        std::vector<uint32_t> irregularTri;
        std::vector<uint32_t> irregularQuad;

        std::vector<uint32_t> edgeQuadPatches;
        std::vector<uint32_t> edgeTriPatches;
    };

    float3 ApplyWeights(Patch& patch, ModifiableShape& shape, uint32_t Idx);
    float3 ApplyWeights(const Patch::ControlPointWeights& CP, const ModifiableShape& shape);

    PatchGroups ClassifyPatches(ModifiableShape& shape);

    std::vector<Patch>  CreateGregoryPatches(PatchGroups& classifiedPatches, ModifiableShape& shape);
    void                CreateGPUPatch(const Patch& p, ModifiableShape& shape, Vector<GPUPatch>& patches, Vector<uint32_t>& indices);


}   /************************************************************************************************/
