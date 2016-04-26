/**********************************************************************

Copyright (c) 2015 - 2016 Robert May

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

#ifndef PHYSICSUTILITIES_H
#define PHYSICSUTILITIES_H

#include "..\buildsettings.h"
#include "..\graphicsutilities\graphics.h"

#include "CommonStructs.h"
#include <typeinfo>

#ifdef _DEBUG
#pragma comment(lib,	"LowLevelDebug.lib"						)
#pragma comment(lib,	"LowLevelClothDEBUG.lib"				)
#pragma comment(lib,	"PhysX3DEBUG_x64.lib"					)
#pragma comment(lib,	"PhysX3CharacterKinematicDebug_x64.lib"	)
#pragma comment(lib,	"PhysX3CommonDEBUG_x64.lib"				)
#pragma comment(lib,	"PhysX3ExtensionsDEBUG.lib"				)
#pragma comment(lib,	"PhysXProfileSDKDEBUG.lib"				)
#pragma comment(lib,	"PhysXVisualDebuggerSDKDEBUG.lib"		)
#pragma comment(lib,	"legacy_stdio_definitions.lib"			)
#pragma comment(lib,	"SceneQueryDEBUG.lib"					)
#pragma comment(lib,	"SimulationControllerDEBUG.lib"			)

#else
#pragma comment(lib,	"LowLevel.lib"						)
#pragma comment(lib,	"LowLevelCloth.lib"					)
#pragma comment(lib,	"PhysX3_x64.lib"					)
#pragma comment(lib,	"PhysX3CharacterKinematic_x64.lib"	)
#pragma comment(lib,	"PhysX3Common_x64.lib"				)
#pragma comment(lib,	"PhysX3Cooking_x64.lib"				)
#pragma comment(lib,	"PhysX3Extensions.lib"				)
#pragma comment(lib,	"PhysXProfileSDK.lib"				)
#pragma comment(lib,	"PhysX3Vehicle.lib"					)
#pragma comment(lib,	"PhysXVisualDebuggerSDK.lib"		)
#pragma comment(lib,	"legacy_stdio_definitions.lib"		)
#endif

using FlexKit::CubeDesc;
using FlexKit::Entity;
using FlexKit::float2;
using FlexKit::float3;
using FlexKit::float4;
using FlexKit::NodeHandle;
using FlexKit::SceneNodes;
using FlexKit::static_vector;

static physx::PxDefaultErrorCallback	gDefaultErrorCallback;
static physx::PxDefaultAllocator		gDefaultAllocatorCallback;

/************************************************************************************************/

struct PhysicsSystem
{
	physx::PxFoundation*			Foundation;
	physx::PxProfileZoneManager*	ProfileZoneManager;
	physx::PxPhysics*				Physx;
	physx::PxCooking*				Oven;

	physx::PxDefaultCpuDispatcher*	CPUDispatcher;


	bool								RemoteDebuggerEnabled;
	physx::PxVisualDebugger*			VisualDebugger;
	physx::PxVisualDebuggerConnection*	VisualDebuggerConnection;
	physx::PxGpuDispatcher*				GPUDispatcher;

	physx::PxMaterial*			DefaultMaterial;
};

/************************************************************************************************/

struct PScene
{
	physx::PxScene*				Scene;
	physx::PxControllerManager*	ControllerManager;
	physx::PxClientID	CID;

	double			T;
	bool			UpdateColliders;

	struct Collider
	{
		Collider(physx::PxActor* c) { collider = c; }
		physx::PxActor* collider;
		NodeHandle		Node;
	};
	static_vector<Collider, 64> Colliders;
};

/************************************************************************************************/

struct SceneDesc
{
	physx::PxTolerancesScale		tolerances;
	physx::PxDefaultCpuDispatcher*	CPUDispatcher;
};

/************************************************************************************************/

size_t	CreateCubeActor		(physx::PxMaterial* material, PScene* scene, float l, float3 initialP = float3(), FlexKit::Quaternion initialQ = FlexKit::Quaternion::Identity(), float3 InitialV = {0, 0, 0});
size_t	CreatePlaneCollider	(physx::PxMaterial* material, PScene* scene);
void	CreateScene			(PhysicsSystem* System, PScene* scn );
size_t	CreateSphereActor	(physx::PxMaterial* material, PScene* scene, float3 initialP = float3(), FlexKit::Quaternion initialQ = FlexKit::Quaternion::Identity(), float3 InitialV = {0, 0, 0});

void	CleanupPhysics	(PhysicsSystem* Physics);
void	CleanUpScene	(PScene* mat);

void	InitiatePhysics	(PhysicsSystem* Physics, uint32_t CoreCount);
void	MakeCube		(CubeDesc& cdesc, SceneNodes* Nodes, PScene* scene, physx::PxMaterial* Material, Entity* E, NodeHandle node, float3 initialP = {0, 0, 0}, FlexKit::Quaternion initialQ = FlexKit::Quaternion::Identity());

typedef void (*FNPSCENECALLBACK_POSTUPDATE)(void*);
typedef void (*FNPSCENECALLBACK_PREUPDATE) (void*);

void	UpdateScene		(PScene* scn, double dt, FNPSCENECALLBACK_POSTUPDATE, FNPSCENECALLBACK_PREUPDATE, void* P);
void	UpdateColliders	(PScene* scn, FlexKit::SceneNodes* nodes);

/************************************************************************************************/

#endif//PHYSICSUTILITIES_H