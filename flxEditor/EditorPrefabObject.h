#pragma once

#include "EditorProject.h"
#include "EditorResource.h"
#include "EditorScriptEngine.h"
#include "EditorSceneResource.h"
#include "Serialization.hpp"


/************************************************************************************************/


constexpr FlexKit::ComponentID EditorScriptComponentID = GetTypeGUID(EditorScriptComponentID);


class ScriptResource :
	public FlexKit::Serializable<ScriptResource, FlexKit::iResource, GetTypeGUID(ScriptResource)>
{
public:
	FlexKit::ResourceBlob	CreateBlob()		const			override;
	const	std::string&	GetResourceID()		const noexcept	override;
			uint64_t		GetResourceGUID()	const noexcept	override;
			ResourceID_t	GetResourceTypeID()	const noexcept	override;

	void					SetResourceID	(const std::string& id)	noexcept final;
	void					SetResourceGUID	(uint64_t guid)			noexcept final;

	void Serialize(auto& ar)
	{
		ar& guid;
		ar& source;
		ar& ID;
	}

	uint64_t	guid	= rand();
	std::string	ID		= "ScriptObject";
	std::string	source;
};


using ScriptResource_ptr = std::shared_ptr<ScriptResource>;


/************************************************************************************************/


class PrefabGameObjectResource :
	public FlexKit::Serializable<PrefabGameObjectResource, FlexKit::iResource, GetTypeGUID(PrefabGameObjectResource)>
{
public:
	FlexKit::ResourceBlob	CreateBlob()		const			override;
	const	std::string&	GetResourceID()		const noexcept	override;
			uint64_t		GetResourceGUID()	const noexcept	override;
			ResourceID_t	GetResourceTypeID()	const noexcept	override;

	void					SetResourceID	(const std::string& id) noexcept override;
	void					SetResourceGUID	(uint64_t GUID) noexcept;

	FlexKit::EntityComponent_ptr FindComponent(FlexKit::ComponentID id);

	void Serialize(auto& ar)
	{
		ar& guid;
		ar& ID;
		ar& entity;
	}

	FlexKit::SceneEntity entity;

	std::string ID   = "PrefabGameObject";
	uint64_t    guid = rand();
};


using PrefabGameObjectResource_ptr = std::shared_ptr<PrefabGameObjectResource>;


struct LoadEntityContextInterface
{
	virtual FlexKit::GameObject&			GameObject()										= 0;
	virtual FlexKit::NodeHandle				GetNode(uint32_t idx)								= 0;
	virtual ProjectResource_ptr				FindSceneResource(uint64_t assetIdx)				= 0;
	virtual FlexKit::TriMeshHandle			LoadTriMeshResource(ProjectResource_ptr resource)	= 0;
	virtual FlexKit::MaterialHandle			DefaultMaterial() const								= 0;
	virtual FlexKit::Scene*					Scene()												= 0;
	virtual FlexKit::LayerHandle			LayerHandle()										= 0;
	virtual FlexKit::SceneEntity*			Resource()											= 0;
};

void LoadEntity(FlexKit::SceneEntity& components, LoadEntityContextInterface& ctx);


/**********************************************************************

Copyright (c) 2021 - 2022 Robert May

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
