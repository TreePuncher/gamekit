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


#include "stdafx.h"
#include "MetaData.h"

namespace FlexKit::ResourceBuilder
{
	/************************************************************************************************/


	MetaTokenList GetMetaDataTokens(char* Buffer, size_t BufferSize, iAllocator* Memory)
	{
		MetaTokenList Tokens;

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


	size_t SkipBrackets(const MetaTokenList& Tokens, size_t StartingPosition)
	{
		size_t itr2			= StartingPosition;
		size_t bracketCount = 0;

		for (; itr2 < Tokens.size(); ++itr2)
		{
			auto T = Tokens[itr2];

			if (T == "{")
				bracketCount++;
			else if (T == "};" || T == "}")
				bracketCount--;

			if(bracketCount == 0)
				return itr2;
		}

		// Malformed Document if code reaches here
		FK_ASSERT(0);
		return -1;
	}


	/************************************************************************************************/


	FlexKit::Pair<ValueList, size_t> ParseDeclarations(const MetaTokenList& Tokens, size_t StartingPosition, size_t endPosition)
	{
		ValueList Values;
		size_t itr2 = StartingPosition;

		for (; itr2 < endPosition;)
		{
			auto T = Tokens[itr2];

			if (T.size())
			{
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

					itr2 += 3;
				}
				else if (T == "uint")
				{
					Value NewValue;
					NewValue.Type = ValueType::UINT;

					auto IDToken = Tokens[itr2 - 2];
					auto ValueToken = Tokens[itr2 + 2];

					char ValueBuffer[16];
					strncpy_s(ValueBuffer, &ValueToken.front(), ValueToken.size());

					auto V              = atol(ValueBuffer);
					NewValue.Data.UI    = V;
					NewValue.ID         = IDToken;

					Values.push_back(NewValue);

					itr2 += 3;
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

					itr2 += 3;
				}
				else if (T == "float")
				{
					Value NewValue;
					NewValue.Type = ValueType::FLOAT;

					auto IDToken	= Tokens[itr2 - 2];
					auto ValueToken = Tokens[itr2 + 2];

					char ValueBuffer[16];
					strncpy_s(ValueBuffer, &ValueToken.front(), ValueToken.size());

					NewValue.Data.F = (float)atof(ValueBuffer);
					NewValue.ID		= IDToken; 

					Values.push_back(NewValue);

					itr2 += 3;
				}
				else if (T == "float3")
				{
					Value NewValue;
					NewValue.Type = ValueType::FLOAT;

					auto IDToken			        = Tokens[itr2 - 2];
					std::string_view ValueTokens[]	= { Tokens[itr2 + 3], Tokens[itr2 + 5], Tokens[itr2 + 7] };

					for (size_t itr3 = 0; itr3 < 3; ++itr3) {
						char ValueBuffer[16];
						strncpy_s(ValueBuffer, ValueTokens[itr3].data(), ValueTokens[itr3].size());
						NewValue.Data.F3[itr3] = (float)atof(ValueBuffer);
					}

					NewValue.ID = IDToken;

					Values.push_back(NewValue);

					itr2 += 8;
				}
				else if (T == "{")
					itr2 = SkipBrackets(Tokens, itr2);
				else if (T == "};")
					return{ Values, itr2 };
				else
					itr2++;
			}
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


	std::pair<bool,size_t>  DetectSubContainer(const MetaTokenList& Tokens, const size_t begin, const size_t end)
	{
		for (size_t itr = begin; itr < end; ++itr)
		{
			if (Tokens[itr] == "Set")
				return { true, itr };
		}

		return { false, 0 };
	}


	/************************************************************************************************/

	MetaDataList ParseSubContainer(const MetaDataParserTable& parser, const MetaTokenList& Tokens, const size_t begin, const size_t end)
	{
		MetaDataList metaData;

		size_t itr = begin;
		while (true)
		{
			auto [res, begin] = DetectSubContainer(Tokens, itr, end);

			if (Tokens[begin + 1] != "{")
				break;

			if (res)
			{
				size_t end = SkipBrackets(Tokens, begin + 1);
				ParseTokens(parser, Tokens, metaData, begin + 2, end - 1);
				itr = end;
			}
			else
				break;
			}

		return metaData;
	}


	/************************************************************************************************/


	MetaData* ParseCubeMapMipLevel(const MetaTokenList& tokens, const ValueList& values, const size_t begin, const size_t end)
	{
		auto level      = tokens[begin - 2];
		auto front      = FindValue(values, "Front");
		auto back       = FindValue(values, "Back");
		auto left       = FindValue(values, "Left");
		auto right      = FindValue(values, "Right");
		auto top        = FindValue(values, "Top");
		auto bottom     = FindValue(values, "Bottom");

		auto* mipLevel = new TextureCubeMapMipLevel_MetaData;

		mipLevel->level = atoi(level.data());

		if (left != nullptr && left->Type == ValueType::STRING)
			mipLevel->TextureFiles[0] = left->Data.S;

		if (right != nullptr && right->Type == ValueType::STRING)
			mipLevel->TextureFiles[1] = right->Data.S;

		if (top != nullptr && top->Type == ValueType::STRING)
			mipLevel->TextureFiles[2] = top->Data.S;

		if (bottom != nullptr && bottom->Type == ValueType::STRING)
			mipLevel->TextureFiles[3] = bottom->Data.S;

		if (back != nullptr && back->Type == ValueType::STRING)
			mipLevel->TextureFiles[4] = back->Data.S;

		if (front != nullptr && front->Type == ValueType::STRING)
			mipLevel->TextureFiles[5] = front->Data.S;

		return mipLevel;
	}


	/************************************************************************************************/


	MetaData* ParseCubeMapTexture(const MetaTokenList& tokens, const ValueList& values, const size_t begin, const size_t end)
	{
		auto AssetID    = tokens[begin - 2];
		auto AssetGUID  = FindValue(values, "AssetGUID");
		auto Format     = FindValue(values, "Format");

		auto* cubemapTexture = new TextureCubeMap_MetaData;

		// Parse Mip levels
		cubemapTexture->Guid        = AssetGUID->Data.I;
		cubemapTexture->ID          = AssetID;
		cubemapTexture->format      = Format->Data.S;
		cubemapTexture->mipLevels   = ParseSubContainer(CubeMapParser, tokens, begin, end);

		return cubemapTexture;
	}


	/************************************************************************************************/


	MetaData* ParseTextureMipLevel(const MetaTokenList& tokens, const ValueList& values, const size_t begin, const size_t end)
	{
		auto level  = tokens[begin - 2];
		auto File   = FindValue(values, "File");

		auto* mipLevel = new TextureMipLevel_MetaData;

		mipLevel->level = atoi(level.data());
		mipLevel->file  = File ? std::string(File->Data.S) : std::string("");

		return mipLevel;
	}


	/************************************************************************************************/


	MetaData* ParseTexture(const MetaTokenList& tokens, const ValueList& values, const size_t begin, const size_t end)
	{
		auto stringID           = tokens[begin - 2];
		auto assetID            = FindValue(values, "AssetGUID");
		auto Format             = FindValue(values, "Format");
		auto File               = FindValue(values, "File");
		auto GenerateMips       = FindValue(values, "GenerateMips");
		auto CompressionQuality = FindValue(values, "CompressionQuality"); // float
		auto CompressTexture    = FindValue(values, "CompressTexture"); // bool


		auto* texture = new Texture_MetaData;

		texture->compressionQuality = CompressionQuality ? CompressionQuality->Data.F : texture->compressionQuality;
		texture->compressTexture    = CompressTexture ? (bool)(CompressTexture->Data.S == "True") : texture->compressTexture;

		texture->assetID            = assetID ? assetID->Data.I : rand();
		texture->stringID           = stringID;
		texture->format             = Format->Data.S;
		texture->file               = File ? (File->Type == ValueType::STRING ? File->Data.S : "") : texture->file;

		if(GenerateMips && GenerateMips->Type == ValueType::STRING && GenerateMips->Data.S == "False")
			texture->generateMipMaps = false;

		texture->mipLevels          = ParseSubContainer(TextureParser, tokens, begin, end);

		return texture;
	}


	/************************************************************************************************/


	MetaData* ParseScene(const MetaTokenList& tokens, const ValueList& values, const size_t begin, const size_t end)
	{
		auto Target				= tokens[begin - 2];
		auto AssetGUID			= FindValue(values, "AssetGUID");
		auto AssetID			= FindValue(values, "AssetID");

		Scene_MetaData* scene	= new Scene_MetaData;
		scene->ID				= Target;

		scene->sceneMetaData    = ParseSubContainer(SceneParser, tokens, begin, end);

		if(AssetGUID != nullptr && AssetGUID->Type == ValueType::INT)
			scene->Guid = AssetGUID->Data.I;

		if (AssetID != nullptr && AssetID->Type == ValueType::STRING)
			scene->SceneID = AssetID->Data.S;

		return scene;
	}
	

	/************************************************************************************************/


	MetaData* ParseModel(const MetaTokenList& Tokens, const ValueList& values, const size_t begin, const size_t end)
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


	MetaData* ParseSkeleton(const MetaTokenList& tokens, const ValueList& values, const size_t begin, const size_t end)
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


	MetaData* ParseSkeletalAnimationClip(const MetaTokenList& tokens, const ValueList& values, const size_t begin, const size_t end)
	{
		auto ID					= FindValue(values, "ID");
		auto beginFrame			= FindValue(values, "Begin");
		auto endFrame			= FindValue(values, "End");
		auto GUID				= FindValue(values, "AssetGUID");
		auto frameRate          = FindValue(values, "FrameRate");

		// Check for ill formed data
		FK_ASSERT((ID			!= nullptr), "MISSING ID TAG!");
		FK_ASSERT((beginFrame	!= nullptr), "MISSING Begin	Value!");
		FK_ASSERT((endFrame		!= nullptr), "MISSING End Value!");
		FK_ASSERT((GUID			!= nullptr), "MISSING GUID!");
		FK_ASSERT((ID->Type			== ValueType::STRING));
		FK_ASSERT((beginFrame->Type == ValueType::FLOAT));
		FK_ASSERT((endFrame->Type   == ValueType::FLOAT));
		FK_ASSERT((GUID->Type		== ValueType::INT));

		SkeletalAnimationClip_MetaData* newSkeletalAnimationClip = new SkeletalAnimationClip_MetaData;

		auto Target = tokens[begin - 2];

		newSkeletalAnimationClip->ClipID	= ID->Data.S;
		newSkeletalAnimationClip->ID		= Target;
		newSkeletalAnimationClip->T_Start	= beginFrame->Data.F;
		newSkeletalAnimationClip->T_End		= endFrame->Data.F;
		newSkeletalAnimationClip->guid		= GUID->Data.I;

		if(frameRate && frameRate->Type == ValueType::FLOAT)
			newSkeletalAnimationClip->frameRate	= frameRate->Data.F;

		return newSkeletalAnimationClip;
	}


	/************************************************************************************************/


	MetaData* ParseTextureSet(const MetaTokenList& tokens, const ValueList& values, const size_t begin, const size_t end)
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


	MetaData* ParseFontSet(const MetaTokenList& tokens, const ValueList& values, const size_t begin, const size_t end)
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


	MetaData* ParseCollider(const MetaTokenList& tokens, const ValueList& values, const size_t begin, const size_t end)
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


	MetaData* ParseTerrainColliderAsset(const MetaTokenList& tokens, const ValueList& values, const size_t begin, const size_t end)
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
				{ "Collider",			    ParseCollider				},
				{ "Font",				    ParseFontSet				},
				{ "Model",				    ParseModel					},
				{ "Scene",				    ParseScene					},
				{ "Skeleton",			    ParseSkeleton				},
				{ "SkeletalAnimationClip",	ParseSkeletalAnimationClip  },
				{ "TerrainCollider",	    ParseTerrainColliderAsset	},
				{ "CubeMapTexture",	        ParseCubeMapTexture         },
				{ "Texture",	            ParseTexture                },
			};
	}


	/************************************************************************************************/


	const MetaDataParserTable CreateTextureParser()
	{
		return { { "MipLevel", ParseTextureMipLevel } };
	}


	/************************************************************************************************/


	const MetaDataParserTable CreateCubeMapParser()
	{
		return {{ "CubeMipLevel", ParseCubeMapMipLevel }};
	}


	/************************************************************************************************/


	MetaData* ParseSceneEntity(const MetaTokenList& tokens, const ValueList& values, const size_t begin, const size_t end)
	{
		auto Target		= tokens[begin - 2];
		auto id			= FindValue(values, "ID");

		SceneElementMeta* element  = new SceneElementMeta;
		*element = SceneElementMeta::Entity();

		element->id			= (id != nullptr && id->Type == ValueType::STRING)? id->Data.S : "";
		element->metaData	= ParseSubContainer(EntityParser, tokens, begin, end);

		return element;
	}


	/************************************************************************************************/


	MetaData* ParseSceneLight(const MetaTokenList& tokens, const ValueList& values, const size_t begin, const size_t end)
	{
		FK_ASSERT(0);

		return nullptr;
	}


	/************************************************************************************************/


	MetaData* ParseSceneNode(const MetaTokenList& tokens, const ValueList& values, const size_t begin, const size_t end)
	{
		auto Target		= tokens[begin - 2];
		auto id			= FindValue(values, "ID");

		SceneElementMeta* element  = new SceneElementMeta;
		*element = SceneElementMeta::Node();

		element->id			= (id != nullptr && id->Type == ValueType::STRING)? id->Data.S : "";
		element->metaData	= ParseSubContainer(NodeParser, tokens, begin, end);

		return element;
	}


	/************************************************************************************************/


	MetaData* ParseSceneTerrainCollider(const MetaTokenList& tokens, const ValueList& values, const size_t begin, const size_t end)
	{
		FK_ASSERT(0);

		return nullptr;
	}


	/************************************************************************************************/

	MetaData* ParseSceneComponentRequirement(const MetaTokenList& tokens, const ValueList& values, const size_t begin, const size_t end)
	{
		FK_ASSERT(0);

		return nullptr;
	}


	/************************************************************************************************/


	MetaData* ParseSceneEntityComponent(const MetaTokenList& tokens, const ValueList& values, const size_t begin, const size_t end)
	{
		auto						componentIDValue = FindValue(values, "ComponentID");
		FNComponentBlobFormatter	formatter	= nullptr;
		uint32_t					componentID = 0;

		if (componentIDValue == nullptr)
			return nullptr;

		if (componentIDValue->Type == ValueType::UINT)
		{
			auto res =  ComponentBlobFormatters.find(componentIDValue->Data.UI);
			if (res == ComponentBlobFormatters.end())
				return nullptr;

			componentID = componentIDValue->Data.UI;
		}
		if (componentIDValue->Type == ValueType::STRING)
		{
			auto res = ComponentIDMap.find(std::string(componentIDValue->Data.S));
			
			if (res == ComponentIDMap.end())
				return nullptr;

			componentID = res->second;
		}

		auto res2 = ComponentBlobFormatters.find(componentID);
		if (res2 == ComponentBlobFormatters.end())
			return nullptr;

		formatter = res2->second;

		SceneComponentMeta* component = new SceneComponentMeta;
		component->metaData		= ParseSubContainer(NodeParser, tokens, begin, end);
		component->CreateBlob	= formatter;

		return component;
	}

	/************************************************************************************************/


	const MetaDataParserTable CreateSceneParser()
	{
		return {
				{ "Node",		ParseSceneNode					},
				{ "Terrain",	ParseSceneTerrainCollider		},
				{ "Component",	ParseSceneComponentRequirement	},
		};
	}

	const MetaDataParserTable CreateNodeParser()
	{
		return {
				{ "Entity",		ParseSceneEntity	},
				{ "Light",		ParseSceneLight		},
				{ "Node",		ParseSceneNode		},
		};
	}


	const MetaDataParserTable CreateEntityParser()
	{
		return {
				{ "Component",	ParseSceneEntityComponent		},
		};
	}


	/************************************************************************************************/


	bool ParseTokens(const MetaDataParserTable& parser, const MetaTokenList& Tokens, MetaDataList& MD_Out, size_t begin, size_t end)
	{
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
			if (auto str = std::string_view(Tokens[itr]);
				parser.find(std::string(Tokens[itr])) == parser.end()		&& // Check Identifer
				Tokens[itr + 1] == ":"									&& // Check for Operator
				parser.find(std::string(Tokens[itr + 2])) != parser.end()	&& // Check keyword_metatype
				Tokens[itr + 3] == "=")
			{
				auto [Values, innerEnd] = ParseDeclarations(Tokens, itr + 5, end);

				if(auto res = parser.at(std::string(Tokens[itr + 2]))(Tokens, Values, itr + 2, innerEnd); res != nullptr)
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
		auto _pred = [&](auto meta) { return (meta->UserType == Type && ID == meta->ID); };

		return filter(MetaData, _pred);
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
