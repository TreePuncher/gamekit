#pragma once

#ifndef COMPONENTBLOBS_H
#define COMPONENTBLOBS_H

#include "buildsettings.h"
#include "MathUtils.h"

namespace FlexKit
{
    enum SceneBlockType
    {
        NodeTable = 1337,
        ComponentRequirementTable,
        Entity,
        EntityComponent,
    };


    struct SceneBlock
    {
        uint32_t CRC32;
        uint32_t blockType;
        uint32_t blockSize;
        byte	 buffer[];
    };

    // SceneNodeBlock must always be defined before any entity Blocks
    struct SceneNodeBlock
    {
        struct Header
        {
            uint32_t CRC32;
            uint32_t blockType = NodeTable;
            uint32_t blockSize;
            uint32_t nodeCount;
        } header;
        static const size_t GetHeaderSize() { return sizeof(Header); }

        struct SceneNode
        {
            float3		position;		// 16
            Quaternion	orientation;	// 16
            float3		scale;			// 16
            size_t		parent;			// 8
            size_t		pad;			// 8
        }nodes[];
    };


    struct ComponentRequirementBlock
    {
        uint32_t CRC32;
        uint32_t blockType = ComponentRequirementTable;
        uint32_t blockSize;

        uint32_t count;
        uint32_t componentID[];
    };


    struct ComponentBlock
    {
        struct Header
        {
            uint32_t CRC32;
            uint32_t blockType = EntityComponent;
            uint32_t blockSize;
            uint32_t componentID;
        } header;

        char buffer[];
    };


    struct EntityBlock
    {
        struct Header
        {
            uint32_t CRC32;
            uint32_t blockType = Entity;
            size_t	 blockSize;

            // Temporary Values
            char	ID[64];

            size_t componentCount = 0;
        } header;
        char buffer[]; // Components stored here
    };


    struct PointLightComponentBlob
    {
        ComponentBlock::Header  header = {
           0,
           EntityComponent,
           sizeof(PointLightComponentBlob),
           GetTypeGUID(PointLightID)
        };

        float2 IR;
        float3 K;
    };


    struct DrawableComponentBlob
    {
        ComponentBlock::Header  header = {
           0,
           EntityComponent,
           sizeof(DrawableComponentBlob),
           GetTypeGUID(DrawableID)
        };

        GUID_t                  resourceID;

        float4 albedo_smoothness;
        float4 specular_metal;
    };


    struct IDComponentBlob
    {
        ComponentBlock::Header  header = {
            0,
            EntityComponent,
            sizeof(IDComponentBlob),
            GetTypeGUID(StringID)
        };

        char ID[64];
    };


    struct SceneNodeComponentBlob
    {
        ComponentBlock::Header  header = {
            0,
            EntityComponent,
            sizeof(SceneNodeComponentBlob),
            GetTypeGUID(TransformComponent)
        };

        uint32_t    nodeIdx             = 0;
        bool        excludeFromScene    = false;
    };
}   // namespace FlexKit


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

#endif
