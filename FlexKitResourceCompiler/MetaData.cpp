/**********************************************************************

Copyright (c) 2015 - 2017 Robert May

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


#include "stdafx.h"
#include "MetaData.h"

namespace FlexKit
{
	/************************************************************************************************/


	MeshTokenList GetMetaDataTokens(char* Buffer, size_t BufferSize, iAllocator* Memory)
	{
		MeshTokenList Tokens;

		size_t StartPos = 0;
		size_t CurrentPos = 0;

		auto SkipWhiteSpaces = [&]()
		{
			while (CurrentPos < BufferSize)
			{
				char CurrentChar = Buffer[CurrentPos];
				switch (CurrentChar)
				{
				case 0x20:
				case 0x00:
				case '\t':
				case '\n':
					++CurrentPos;
					break;
				default:
					return;
					break;
				}
			}
		};

		while (CurrentPos < BufferSize)
		{
			auto C						= Buffer[CurrentPos];

			if (Buffer[CurrentPos] == ' ' || Buffer[CurrentPos] == '\n' || Buffer[CurrentPos] == '\t' || Buffer[CurrentPos] == '\0')
			{
				std::string_view NewToken = { Buffer + StartPos, CurrentPos - StartPos };

				if( NewToken.size() )
					Tokens.push_back(NewToken);
				
				SkipWhiteSpaces();

				StartPos = CurrentPos;
			}
			else
				++CurrentPos;
		}

		return Tokens;
	}


	/************************************************************************************************/


	Resource* MetaDataToBlob(MetaData* Meta, iAllocator* Mem)
	{
		Resource* Result = nullptr;
		switch (Meta->type)
		{
		case MetaData::EMETAINFOTYPE::EMI_TEXTURESET:
		{
			TextureSet_MetaData* TextureSet = (TextureSet_MetaData*)Meta;
			auto& NewTextureSet = Mem->allocate<TextureSetBlob>();

			NewTextureSet.GUID = TextureSet->Guid;
			NewTextureSet.ResourceSize = sizeof(TextureSetBlob);

			for (size_t I = 0; I < 2; ++I) {
				memcpy(NewTextureSet.Textures[I].Directory, TextureSet->Textures.TextureLocation[I].Directory, 64);
				NewTextureSet.Textures[I].guid = TextureSet->Textures.TextureID[I];
			}

			Result = (Resource*)&NewTextureSet;
			break;
		}
		default:
			break;
		}
		return Result;
	}

	
	/************************************************************************************************/


	size_t SkipBrackets(const MeshTokenList& Tokens, size_t StartingPosition)
	{
		size_t itr2			= StartingPosition;
		size_t bracketCount = 1;

		for (; itr2 < Tokens.size(); ++itr2)
		{
			auto T = Tokens[itr2];

			if (T.size() && (T == "};" || T == "}"))
				bracketCount--;

			if(bracketCount == 0)
				return itr2;
		}

		// Malformed Document if code reaches here
		FK_ASSERT(0);
		return -1;
	}


	/************************************************************************************************/


	FlexKit::Pair<ValueList, size_t> ParseDeclarations(const MeshTokenList& Tokens, size_t StartingPosition, size_t endPosition)
	{
		ValueList Values;
		size_t itr2 = StartingPosition;

		for (; itr2 < endPosition; ++itr2)
		{
			auto T = Tokens[itr2];

			if (T.size())
				if (T == "int")
				{
					Value NewValue;
					NewValue.Type = ValueType::INT;

					auto IDToken    = Tokens[itr2 - 2];
					auto ValueToken = Tokens[itr2 + 2];

					char ValueBuffer[16];
					strncpy_s(ValueBuffer, &ValueToken.front(), ValueToken.size());
					
					int V				= atoi(ValueBuffer);
					NewValue.Data.I		= V;
					NewValue.ID			= IDToken;

					Values.push_back(NewValue);
				}
				else if (T == "string")
				{
					Value NewValue;
					NewValue.Type = ValueType::STRING;

					auto IDToken		= Tokens[itr2 - 2];
					auto ValueToken		= Tokens[itr2 + 2];

					NewValue.ID		= IDToken;
					NewValue.Data.S	= ValueToken != "_" ? ValueToken : "";

					Values.push_back(NewValue);
				}
				else if (T == "float")
				{
					Value NewValue;
					NewValue.Type = ValueType::FLOAT;

					auto IDToken	= Tokens[itr2 - 2];
					auto ValueToken = Tokens[itr2 + 2];

					char ValueBuffer[16];
					strncpy_s(ValueBuffer, &ValueToken.front(), ValueToken.size());

					NewValue.Data.F = atof(ValueBuffer);
					NewValue.ID		= IDToken; 

					Values.push_back(NewValue);
				}
				else if (T == "{")
					itr2 = SkipBrackets(Tokens, StartingPosition);
				else if (T == "};")
					return{ Values, itr2 };
		}

		// Should Be Un-reachable
		return{ Values, itr2 };
	}


	/************************************************************************************************/


	const Value* FindValue(const std::vector<Value>& Values, char* ValueID)
	{
		auto res = 
			FlexKit::find(
				Values,
				[&](auto& V) { return (V.ID == ValueID); });

		return { (res != Values.end() ? &*res : nullptr) };
	}


	/************************************************************************************************/


	std::pair<bool,size_t>  DetectSubContainer(const MeshTokenList& Tokens, const size_t begin, const size_t end)
	{
		for (size_t itr = begin; itr < end; ++itr)
		{
			if (Tokens[itr] == "MetaDataCollection")
				return { true, itr };
		}

		return { false, 0 };
	}


	/************************************************************************************************/


	MetaData* ParseScene(const MeshTokenList& Tokens, const ValueList& values, const size_t begin, const size_t end)
	{
		auto Target				= Tokens[begin - 2];
		auto AssetGUID			= FindValue(values, "AssetGUID");
		auto AssetID			= FindValue(values, "AssetID");

		Scene_MetaData* scene	= new Scene_MetaData;
		scene->ID				= Target;

		size_t itr2 = begin;
		while(true)
		{
			auto [res, begin] = DetectSubContainer(Tokens, itr2, end);

			if (Tokens[begin + 1] != "{")
				break;

			if (res) 
			{
				size_t end = SkipBrackets(Tokens, begin + 1);
				ParseTokens(SceneParser, Tokens, scene->sceneMetaData, begin + 2, end - 1);
				itr2 = end;
			}
			else
				break;
		}

		if(AssetGUID != nullptr && AssetGUID->Type == ValueType::INT)
			scene->Guid = AssetGUID->Data.I;

		if (AssetID != nullptr && AssetID->Type == ValueType::STRING)
			scene->SceneID = AssetID->Data.S;

		return scene;
	}
	

	/************************************************************************************************/


	MetaData* ParseModel(const MeshTokenList& Tokens, const ValueList& values, const size_t begin, const size_t end)
	{
		auto AssetID			= FindValue(values, "AssetID");
		auto AssetGUID			= FindValue(values, "AssetGUID");
		auto ColliderGUID		= FindValue(values, "ColliderGUID");
		auto Target				= Tokens[begin - 2];

#if _DEBUG
		FK_ASSERT((AssetID != nullptr), "MISSING ID!");
		FK_ASSERT((AssetID->Type == ValueType::STRING));

		FK_ASSERT((AssetGUID != nullptr), "MISSING GUID!");
		FK_ASSERT((AssetGUID->Type == ValueType::INT));
#else
		if ((!AssetID || AssetID->Type != ValueType::STRING) || (!AssetGUID || AssetGUID->Type != ValueType::INT))
			return false;
#endif

		Mesh_MetaData* model = new Mesh_MetaData;

		if (ColliderGUID != nullptr && ColliderGUID->Type == ValueType::INT)
			model->ColliderGUID = ColliderGUID->Data.I;
		else
			model->ColliderGUID = INVALIDHANDLE;

		model->MeshID	= AssetID->Data.S;
		model->guid		= AssetGUID->Data.I;
		model->ID		= Target;

		return model;
	}


	/************************************************************************************************/


	MetaData* ParseSkeleton(const MeshTokenList& tokens, const ValueList& values, const size_t begin, const size_t end)
	{
		auto AssetID			= FindValue(values, "AssetID");		
		auto AssetGUID			= FindValue(values, "AssetGUID");	

#if _DEBUG
		FK_ASSERT((AssetID			!= nullptr), "MISSING ID!");
		FK_ASSERT((AssetID->Type	== ValueType::STRING));

		FK_ASSERT((AssetGUID		!= nullptr), "MISSING GUID!");
		FK_ASSERT((AssetGUID->Type	== ValueType::INT));
#else
		if ((!AssetID || AssetID->Type != ValueType::STRING) || (!AssetGUID || AssetGUID->Type != ValueType::INT))
			return false;
#endif

		Skeleton_MetaData* Skeleton = new Skeleton_MetaData;

		auto Target = tokens[ begin - 2];

		Skeleton->SkeletonID	= AssetID->Data.S;
		Skeleton->SkeletonGUID	= AssetGUID->Data.I;
		Skeleton->ID			= Target;

		return Skeleton;
	}


	/************************************************************************************************/


	MetaData* ParseAnimationClip(const MeshTokenList& tokens, const ValueList& values, const size_t begin, const size_t end)
	{
		auto ID					= FindValue(values, "ID");
		auto beginFrame			= FindValue(values, "Begin");
		auto endFrame			= FindValue(values, "End");
		auto GUID				= FindValue(values, "AssetGUID");

		// Check for ill formed data
		FK_ASSERT((ID			!= nullptr), "MISSING ID TAG!");
		FK_ASSERT((beginFrame	!= nullptr), "MISSING Begin	Value!");
		FK_ASSERT((endFrame		!= nullptr), "MISSING End Value!");
		FK_ASSERT((GUID			!= nullptr), "MISSING GUID!");
		FK_ASSERT((ID->Type			== ValueType::STRING));
		FK_ASSERT((beginFrame->Type == ValueType::FLOAT));
		FK_ASSERT((endFrame->Type   == ValueType::FLOAT));
		FK_ASSERT((GUID->Type		== ValueType::INT));

		AnimationClip_MetaData* NewAnimationClip = new AnimationClip_MetaData;

		auto Target = tokens[begin - 2];

		NewAnimationClip->ClipID	= ID->Data.S;
		NewAnimationClip->ID		= Target;
		NewAnimationClip->T_Start	= beginFrame->Data.F;
		NewAnimationClip->T_End		= endFrame->Data.F;
		NewAnimationClip->guid		= GUID->Data.I;

		return NewAnimationClip;
	}


	/************************************************************************************************/


	MetaData* ParseTextureSet(const MeshTokenList& tokens, const ValueList& values, const size_t begin, const size_t end)
	{
		FK_ASSERT(0);

		auto AssetID	  = tokens[begin - 2];
		auto AssetGUID	  = FindValue(values, "AssetGUID");
		auto Albedo		  = FindValue(values, "Albedo");
		auto AlbedoID	  = FindValue(values, "AlbedoGUID");
		auto RoughMetal	  = FindValue(values, "RoughMetal");
		auto RoughMetalID = FindValue(values, "RoughMetalGUID");

		TextureSet_MetaData* TextureSet_Meta = new TextureSet_MetaData;

		if (AssetGUID && AssetGUID->Type == ValueType::INT)
			TextureSet_Meta->Guid = AssetGUID->Data.I;

		if (Albedo && Albedo->Type == ValueType::STRING){
			//auto dest = TextureSet_Meta->Textures.TextureLocation[ETT_ALBEDO].Directory;
			//strncpy(dest, Albedo->Data.S.S, Albedo->Data.S.size);
		}

		if (AlbedoID && AlbedoID->Type == ValueType::INT) {
			//TextureSet_Meta->Textures.TextureID[ETT_ALBEDO] = AlbedoID->Data.I;
		} else {
			//TextureSet_Meta->Textures.TextureID[ETT_ALBEDO] = INVALIDHANDLE;
		}

		if (RoughMetal && RoughMetal->Type == ValueType::STRING){
			//auto dest = TextureSet_Meta->Textures.TextureLocation[ETT_ROUGHSMOOTH].Directory;
			//strncpy(dest, RoughMetal->Data.S.S, RoughMetal->Data.S.size);
		}

		if (RoughMetalID && RoughMetalID->Type == ValueType::INT) {
			//TextureSet_Meta->Textures.TextureID[ETT_ROUGHSMOOTH] = RoughMetalID->Data.I;
		} else {
			//TextureSet_Meta->Textures.TextureID[ETT_ROUGHSMOOTH] = INVALIDHANDLE;
		}

		return TextureSet_Meta;
	}


	/************************************************************************************************/


	MetaData* ParseFontSet(const MeshTokenList& tokens, const ValueList& values, const size_t begin, const size_t end)
	{
		auto AssetID			= tokens[begin - 2];
		auto AssetGUID			= FindValue(values,	"AssetGUID");
		auto FontFile			= FindValue(values, "File");

		// Check for ill formed data
		FK_ASSERT(FontFile);
		FK_ASSERT(AssetGUID);

		Font_MetaData* FontData		= new Font_MetaData;
		FontData->UserType			= MetaData::EMETA_RECIPIENT_TYPE::EMR_NONE;
		FontData->type				= MetaData::EMETAINFOTYPE::EMI_FONT;
		FontData->ID				= AssetID;
		FontData->FontFile			= FontFile->Data.S;
		FontData->Guid				= AssetGUID->Data.I;

		return FontData;
	}


	/************************************************************************************************/


	MetaData* ParseCollider(const MeshTokenList& tokens, const ValueList& values, const size_t begin, const size_t end)
	{
		auto Target				= tokens[begin - 2];
		auto AssetGUID			= FindValue(values,	"AssetGUID");
		auto AssetID			= FindValue(values,	"AssetID");

		Collider_MetaData* Collider = new Collider_MetaData;
		Collider->ID = Target;

		if (AssetGUID != nullptr && AssetGUID->Type == ValueType::INT)
			Collider->Guid = AssetGUID->Data.I;

		if (AssetID != nullptr && AssetID->Type == ValueType::STRING)
			Collider->ColliderID = AssetID->Data.S;

		return Collider;
	}


	/************************************************************************************************/


	MetaData* ParseTerrainColliderAsset(const MeshTokenList& tokens, const ValueList& values, const size_t begin, const size_t end)
	{
		auto Target				= tokens[begin - 2];
		auto AssetGUID			= FindValue(values, "AssetGUID");
		auto AssetID			= FindValue(values, "AssetID");
		auto BitMapLoc			= FindValue(values, "BitMapLocation");

		TerrainCollider_MetaData* TerrainCollider = new TerrainCollider_MetaData;

		if (AssetGUID != nullptr && AssetGUID->Type == ValueType::INT)
			TerrainCollider->Guid = AssetGUID->Data.I;

		if (AssetID != nullptr && AssetID->Type == ValueType::STRING)
			TerrainCollider->ColliderID	= AssetID->Data.S;

		if (BitMapLoc != nullptr && BitMapLoc->Type == ValueType::STRING)
			TerrainCollider->BitmapFileLoc = BitMapLoc->Data.S;

		return TerrainCollider;
	}

	
	/************************************************************************************************/


	const MetaDataParserTable CreateDefaultParser()
	{
		return {
				{ "AnimationClip",		ParseAnimationClip			},
				{ "Collider",			ParseCollider				},
				{ "Font",				ParseFontSet				},
				{ "Model",				ParseModel					},
				{ "Scene",				ParseScene					},
				{ "Skeleton",			ParseSkeleton				},
				{ "TerrainCollider",	ParseTerrainColliderAsset	},
				//table["TextureSet"]		= ParseTextureSet;
				//table["Test"]				= CreateParser(0);
			};
	}


	/************************************************************************************************/


	MetaData* ParseSceneEntity(const MeshTokenList& tokens, const ValueList& values, const size_t begin, const size_t end)
	{
		FK_ASSERT(0);

		return nullptr;
	}


	/************************************************************************************************/


	MetaData* ParseSceneLight(const MeshTokenList& tokens, const ValueList& values, const size_t begin, const size_t end)
	{
		FK_ASSERT(0);

		return nullptr;
	}


	/************************************************************************************************/


	MetaData* ParseSceneNode(const MeshTokenList& tokens, const ValueList& values, const size_t begin, const size_t end)
	{
		FK_ASSERT(0);

		return nullptr;
	}


	/************************************************************************************************/


	MetaData* ParseSceneTerrainCollider(const MeshTokenList& tokens, const ValueList& values, const size_t begin, const size_t end)
	{
		FK_ASSERT(0);

		return nullptr;
	}


	/************************************************************************************************/

	MetaData* ParseSceneComponentRequirement(const MeshTokenList& tokens, const ValueList& values, const size_t begin, const size_t end)
	{
		FK_ASSERT(0);

		return nullptr;
	}


	/************************************************************************************************/


	const MetaDataParserTable CreateSceneParser()
	{
		return {
				{ "Entity",		ParseSceneEntity				},
				{ "Light",		ParseSceneLight					},
				{ "Node",		ParseSceneTerrainCollider		},
				{ "Terrain",	ParseSceneTerrainCollider		},
				{ "Component",	ParseSceneComponentRequirement	},
		};
	}


	/************************************************************************************************/


	bool ParseTokens(const MetaDataParserTable& parser, const MeshTokenList& Tokens, MetaDataList& MD_Out, size_t begin, size_t end)
	{
		struct Value
		{
			enum TYPE
			{
				INT,
				STRING,
				FLOAT,
			}Type;

			Token T;
		};

		// Metadata Format
		// String					:= {{ A-Z, a-z, 0-9 }...}
		// Number					:= { 0-9... "." 0-9... }
		// Identifier				:= Any String that is not a keyword or operator symbol
		// keyword_valuetype		:= { integer, string, float }
		// keyword_metatype			:= { AnimationClip, Collider, Font, Model, Skeleton, Scene, TerrainCollider }
		// keyword_containertype	:= { MetaDataCollection[parent container is Scene] }
		// Value_Declaration		= "[Identifier] : keyword_valuetype	= [String,Number];"
		// MetaDataDeclaration		= "[Identifier] : [keyword_metatype] = { [Value_Declaration, SubContainerDeclaration];... };"
		// SubContainerDeclaration	= "{ [Identifier], _ } : [keyword_containertype] ] { [MetaDataDeclaration];... };"

		for (size_t itr = begin; itr < end && itr < Tokens.size() - 3; ++itr)
		{
			if (std::string str = std::string(Tokens[itr]); 
				parser.find(string(Tokens[itr])) == parser.end()		&& // Check Identifer
				Tokens[itr + 1] == ":"									&& // Check for Operator
				parser.find(string(Tokens[itr + 2])) != parser.end()	&& // Check keyword_metatype
				Tokens[itr + 3] == "=")
			{
				auto [Values, innerEnd] = ParseDeclarations(Tokens, itr + 5, end);

				if(auto res = parser.at(string(Tokens[itr + 2]))(Tokens, Values, itr + 2, innerEnd); res != nullptr)
					MD_Out.push_back(std::shared_ptr<MetaData>{ res });

				itr = innerEnd;
			}
			else if (Tokens[itr] =="{")
				itr = SkipBrackets(Tokens, itr);
		}

		return true;
	}


	/************************************************************************************************/


	bool ReadMetaData(const char* Location, iAllocator* Memory, iAllocator* TempMemory, MetaDataList& MD_Out)
	{
		size_t BufferSize = GetFileSize(Location);

#if USING(RESCOMPILERVERBOSE)
		if(!BufferSize)
			std::cout << "Empty File Found!\nThis is Probably an Error.\n";
		else
			std::cout << "Loaded a MetaData File of size: " << BufferSize << "\n";

#endif

		char* Buffer = (char*)TempMemory->malloc(BufferSize);
		LoadFileIntoBuffer(Location, (byte*)Buffer, BufferSize);

		auto Tokens = GetMetaDataTokens(Buffer, BufferSize, TempMemory);
		auto res	= ParseTokens(DefaultParser, Tokens, MD_Out, 0, Tokens.size());

		TempMemory->free(Buffer);

		return res;
	}


	/************************************************************************************************/


	void PrintMetaDataList(const MetaDataList& MD)
	{
		for (auto& MetaData : MD)
		{
			std::cout << MetaData->ID << " : Type: ";
			switch (MetaData->type)
			{
			case MetaData::EMETAINFOTYPE::EMI_COLLIDER:				
				std::cout << "Collider\n";				break;
			case MetaData::EMETAINFOTYPE::EMI_MESH:					
				std::cout << "Mesh\n";					break;
			case MetaData::EMETAINFOTYPE::EMI_SCENE:				
				std::cout << "Scene\n";					break;
			case MetaData::EMETAINFOTYPE::EMI_SKELETAL:				
				std::cout << "Skeletal\n";				break;
			case MetaData::EMETAINFOTYPE::EMI_SKELETALANIMATION:	
				std::cout << "Skeletal Animation\n";	break;
			case MetaData::EMETAINFOTYPE::EMI_ANIMATIONCLIP:		
				std::cout << "Animation Clip\n";		break;
			case MetaData::EMETAINFOTYPE::EMI_ANIMATIONEVENT:		
				std::cout << "Animation Event\n";		break;
			case MetaData::EMETAINFOTYPE::EMI_TEXTURESET:			
				std::cout << "TexureSet\n";				break;
			default:
				break;
			}
		}
	}


	/************************************************************************************************/


	MetaDataList FindRelatedMetaData(const MetaDataList& MetaData, const MetaData::EMETA_RECIPIENT_TYPE Type, const std::string& ID)
	{
		MetaDataList RelatedData;

		for (auto meta : MetaData)
		{
			if (meta->UserType == Type && ID == meta->ID)
					RelatedData.push_back(meta);
		}

		return RelatedData;
	}


	/************************************************************************************************/


	std::shared_ptr<Mesh_MetaData> GetMeshMetaData(MetaDataList& MetaData)
	{
		for (auto meta : MetaData)
			if (meta->type == MetaData::EMETAINFOTYPE::EMI_MESH)
				return std::static_pointer_cast<Mesh_MetaData>(meta);

		return nullptr;
	}

	/************************************************************************************************/




	MetaDataList ScanForRelated(const MetaDataList& metaData, const MetaData::EMETAINFOTYPE type)
	{
		return FilterList(
			[&](auto metaData)
			{
				return (metaData->type == type);
			},
			metaData);
	}

	/************************************************************************************************/
}// Namespace Flexkit