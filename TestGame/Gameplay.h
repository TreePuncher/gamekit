#pragma once


/************************************************************************************************/


#include "containers.h"
#include "Components.h"
#include "Events.h"
#include "Enemy1.h"
#include "FrameGraph.h"
#include "GameAssets.h"
#include "GameFramework.h"
#include "MathUtils.h"
#include "Particles.h"
#include "Player.h"
#include "physicsutilities.h"
#include <regex>


/************************************************************************************************/


typedef uint64_t MultiplayerPlayerID_t;


inline MultiplayerPlayerID_t GeneratePlayerID()
{
	std::random_device generator;
	std::uniform_int_distribution<MultiplayerPlayerID_t> distribution(
		std::numeric_limits<MultiplayerPlayerID_t>::min(),
		std::numeric_limits<MultiplayerPlayerID_t>::max());

	return distribution(generator);
}


/************************************************************************************************/


using FlexKit::Event;
using FlexKit::FrameGraph;
using FlexKit::GameFramework;

using FlexKit::iAllocator;
using FlexKit::CameraHandle;
using FlexKit::ConstantBufferHandle;
using FlexKit::Handle_t;
using FlexKit::VertexBufferHandle;
using FlexKit::ResourceHandle;
using FlexKit::static_vector;

using FlexKit::CameraView;

using FlexKit::int2;
using FlexKit::uint2;
using FlexKit::float2;
using FlexKit::float3;
using FlexKit::float4;
using FlexKit::Quaternion;



/************************************************************************************************/


enum PLAYER_INPUT_EVENTS : int64_t
{
	PLAYER_UP				= GetCRCGUID(PLAYER_UP),
	PLAYER_LEFT				= GetCRCGUID(PLAYER_LEFT),
	PLAYER_DOWN				= GetCRCGUID(PLAYER_DOWN),
	PLAYER_RIGHT			= GetCRCGUID(PLAYER_RIGHT),

	PLAYER_ACTION1			= GetTypeGUID(PLAYER_ACTION1),
	PLAYER_ACTION2			= GetTypeGUID(PLAYER_ACTION2),
	PLAYER_ACTION3			= GetTypeGUID(PLAYER_ACTION3),
	PLAYER_ACTION4			= GetTypeGUID(PLAYER_ACTION4),

	PLAYER_HOLD				= GetCRCGUID(PLAYER_HOLD),

	PLAYER_ROTATE_LEFT		= GetCRCGUID(PLAYER_ROTATE_LEFT),
	PLAYER_ROTATE_RIGHT		= GetCRCGUID(PLAYER_ROTATE_RIGHT),

	DEBUG_PLAYER_UP			= GetCRCGUID(DEBUG_PLAYER_UP),
	DEBUG_PLAYER_LEFT		= GetCRCGUID(DEBUG_PLAYER_LEFT),
	DEBUG_PLAYER_DOWN		= GetCRCGUID(DEBUG_PLAYER_DOWN),
	DEBUG_PLAYER_RIGHT		= GetCRCGUID(DEBUG_PLAYER_RIGHT),
	DEBUG_PLAYER_ACTION1	= GetCRCGUID(DEBUG_PLAYER_ACTION1),
	DEBUG_PLAYER_HOLD		= GetCRCGUID(DEBUG_PLAYER_HOLD),

	PLAYER_UNKNOWN,
};


enum DEBUG_EVENTS : int64_t
{
	TOGGLE_DEBUG_CAMERA  = GetCRCGUID(TOGGLE_DEBUG_CAMERA),
	TOGGLE_DEBUG_OVERLAY = GetCRCGUID(TOGGLE_DEBUG_OVERLAY)
};


/************************************************************************************************/


using CardTypeID_t = uint32_t;

class GameWorld;

struct GadgetInterface
{
	CardTypeID_t	CardID				= (CardTypeID_t)-1;
	const char*		cardName			= "!!!!!";
	const char*		description			= "!!!!!";

	uint16_t		requiredPowerLevel	= 0;

	TypeErasedCallable<void (GameWorld&, GameObject&), 16> OnActivate    = [](GameWorld& world, GameObject& player){};
};


struct FlashLight : public GadgetInterface
{
	FlashLight();

	static CardTypeID_t ID() { return GetTypeGUID(FlashLight); };
};


struct PlayerDesc
{
	MultiplayerPlayerID_t	player;
	LayerHandle				layer;
	Scene&					scene;

	float h = 1.0f;
	float r = 1.0f;
};


struct GadgetData
{
	GameObject*				gameObject;

	uint32_t				owner;
	GadgetInterface*		gadgetState;
};

struct PlayerInputState
{
	float Y     = 0;
	float X     = 0;
	float up    = 0;
	float down  = 0;

	enum class Event
	{
		Action1,
		Action2,
		Action3,
		Action4,
	};

	static_vector<Event> events;

	FlexKit::float2 mousedXY = { 0, 0 };
};


struct PlayerFrameState
{
	MultiplayerPlayerID_t		player;
	float3						pos;
	float3						velocity;
	float3						forwardVector;
	Quaternion					orientation;

	PlayerInputState			inputState;
	ThirdPersonCameraFrameState	cameraState;
};


struct LocalPlayerData
{
	PlayerHandle						playerGameState;
	MultiplayerPlayerID_t				playerID;
	CircularBuffer<PlayerFrameState>	inputHistory;
	//Vector<iNetEvent>					pendingEvent;
};

struct RemotePlayerData
{
	GameObject*				gameObject;
	PlayerHandle			playerGameState;
	ConnectionHandle		connection;
	MultiplayerPlayerID_t	playerID;

	void Update(PlayerFrameState state)
	{
		SetControllerPosition(*gameObject, state.pos);
		SetControllerOrientation(*gameObject, state.orientation);

		Apply(
			*gameObject,
			[&](PlayerView& player)
			{
				player->forward		= state.forwardVector;
				player->position	= GetControllerPosition(*gameObject);
			});
	}

	PlayerFrameState    GetFrameState() const
	{
		PlayerFrameState out
		{
			.pos			= GetWorldPosition(*gameObject),
			.velocity		= { 0, 0, 0 },
			.orientation	= GetOrientation(*gameObject),
		};

		return out;
	}
};


inline static const ComponentID LocalPlayerComponentID  = GetTypeGUID(LocalPlayerData);

using LocalPlayerHandle		= Handle_t<32, LocalPlayerComponentID>;
using LocalPlayerComponent	= BasicComponent_t<LocalPlayerData, LocalPlayerHandle, LocalPlayerComponentID>;
using LocalPlayerView		= LocalPlayerComponent::View;

inline static const ComponentID RemotePlayerComponentID  = GetTypeGUID(RemotePlayerData);

using RemotePlayerHandle	= Handle_t<32, RemotePlayerComponentID>;											// this is a handle to an instance
using RemotePlayerComponent	= BasicComponent_t<RemotePlayerData, RemotePlayerHandle, RemotePlayerComponentID>;	// This defines a new component
using RemotePlayerView		= typename RemotePlayerComponent::View;												// This defines an interface to access data in the component in a easy manner 

inline static const ComponentID GadgetComponentID = GetTypeGUID(GadgetComponent);

using GadgetHandle		= Handle_t<32, GadgetComponentID>;
using GadgetComponent	= BasicComponent_t<GadgetData, GadgetHandle, GadgetComponentID>;
using GadgetView		= typename GadgetComponent::View;


class GameWorld
{
public:
	GameWorld(EngineCore& IN_core, bool debug = false);
	~GameWorld();

	void Release();

	GameObject& AddLocalPlayer(MultiplayerPlayerID_t multiplayerID);
	GameObject& AddRemotePlayer(MultiplayerPlayerID_t playerID, ConnectionHandle connection = InvalidHandle);
	void		AddCube(float3 POS);

	void		SpawnEnemy_1	(const Enemy_1_Desc& desc);
	GameObject&	CreatePlayer	(const PlayerDesc& desc);

	bool		LoadScene(GUID_t assetID);

	void		UpdatePlayer		(const PlayerFrameState& playerState, const double);
	void		UpdateRemotePlayer	(const PlayerFrameState& playerState, const double);
	UpdateTask&	UpdateGadgets		(UpdateDispatcher& dispathcer, ObjectPool<GameObject>& objectPool, const double dt);


	const Shape				cubeShape;

	EngineCore&				core;
	RenderSystem&			renderSystem;

	Scene					scene;
	LayerHandle				layer;

	Enemy_1_Component		enemy1Component;
	ObjectPool<GameObject>	objectPool;
	iAllocator&				allocator;
};


/************************************************************************************************/


struct WorldAssets
{	// walls
	AssetHandle wallXSegment;
	AssetHandle wallYSegment;
	AssetHandle wallISegment;
	AssetHandle wallEndSegment;
	AssetHandle cornerSegment;

	// Floors
	AssetHandle floor;
	AssetHandle ramp;
};

WorldAssets LoadBasicTiles();


void CreateMultiplayerScene(GameWorld&, const WorldAssets&, iAllocator& allocator, iAllocator& temp);

PlayerFrameState	GetPlayerFrameState	(GameObject& gameObject);
RemotePlayerData*	FindRemotePlayer	(MultiplayerPlayerID_t ID);
GameObject*			FindPlayer			(MultiplayerPlayerID_t ID);


/************************************************************************************************/


/**********************************************************************

Copyright (c) 2022 Robert May

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

