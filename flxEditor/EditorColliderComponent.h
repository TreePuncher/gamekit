#pragma once

#include <vector>
#include "Components.h"
#include "EditorSceneResource.h"
#include "physicsutilities.h"
#include "type.h"



/************************************************************************************************/


struct Collider
{
	FlexKit::StaticBodyShape	shape;
	std::string					shapeName;

	void Serialize(auto& ar)
	{
		ar& shape;
		ar& shapeName;
	}
};


struct StaticColliderEditorData
{
	std::vector<Collider>	colliders;
	uint64_t				editorId;
};


/************************************************************************************************/


class EditorColliderComponent :
	public FlexKit::Serializable<EditorColliderComponent, FlexKit::EntityComponent, FlexKit::StaticBodyComponentID>
{
public:
	EditorColliderComponent() :
		Serializable{ FlexKit::StaticBodyComponentID } {}

	void Serialize(auto& ar)
	{
		EntityComponent::Serialize(ar);

		ar& version;
		ar& colliders;
	}

	FlexKit::Blob GetBlob() override;

	uint32_t				version = 1;
	uint32_t				editorId;
	std::vector<Collider>	colliders;

	inline static RegisterConstructorHelper<EditorColliderComponent, FlexKit::StaticBodyComponentID> registered{};
};


/************************************************************************************************/


class EditorRigidBodyComponent :
	public FlexKit::Serializable<EditorRigidBodyComponent, FlexKit::EntityComponent, FlexKit::RigidBodyComponentID>
{
public:
	EditorRigidBodyComponent() :
		Serializable{ FlexKit::RigidBodyComponentID } {}

	void Serialize(auto& ar)
	{
		EntityComponent::Serialize(ar);

		ar& version;
	}

	struct Version1_RigidBodyCollider
	{

	};

	FlexKit::Blob GetBlob() override;

	uint32_t				version = 1;
	FlexKit::ResourceHandle ColliderResourceID;

	inline static RegisterConstructorHelper<EditorColliderComponent, FlexKit::RigidBodyComponentID> registered{};
};


/************************************************************************************************/


class EditorViewport;
class EditorProject;


void RegisterColliderInspector  (EditorViewport& viewport, EditorProject& project);
void RegisterRigidBodyInspector (EditorViewport& viewport, EditorProject& project);


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
