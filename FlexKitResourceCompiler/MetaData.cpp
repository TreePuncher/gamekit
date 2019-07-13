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
	typedef std::vector<std::string_view> MeshTokenList;


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


	struct Value
	{
		Value() {}

		enum TYPE
		{
			INT,
			STRING,
			FLOAT,
			INVALID
		}Type = INVALID;

		union _
		{
			_() : S{ nullptr, 0 } {}

			float				F;
			int					I;
			std::string_view	S;
		}Data = _{};

		std::string_view		ID;
	};

	typedef std::vector<Value> ValueList;


	
	/************************************************************************************************/


	size_t SkipBrackets(MeshTokenList& Tokens, size_t StartingPosition)
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


	FlexKit::Pair<ValueList, size_t> ProcessDeclaration(MeshTokenList& Tokens, size_t StartingPosition, size_t endPosition)
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
					NewValue.Type = Value::INT;

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
					NewValue.Type = Value::STRING;

					auto IDToken		= Tokens[itr2 - 2];
					auto ValueToken		= Tokens[itr2 + 2];

					NewValue.ID		= IDToken;
					NewValue.Data.S	= ValueToken;

					Values.push_back(NewValue);
				}
				else if (T == "float")
				{
					Value NewValue;
					NewValue.Type = Value::FLOAT;

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


	Value* FindValue(std::vector<Value>& Values, char* ValueID)
	{
		auto res = 
			FlexKit::find(
				Values,
				[&](auto& V) { return (V.ID == ValueID); });

		return { (res != Values.end() ? &*res : nullptr) };
	}


	/************************************************************************************************/


	bool ParseTokens(MeshTokenList& Tokens, MetaDataList& MD_Out, size_t begin, size_t end);

	std::pair<bool,size_t>  DetectSubContainer(MeshTokenList& Tokens, const size_t begin, const size_t end)
	{
		for (size_t itr = begin; itr < end; ++itr)
		{
			if (Tokens[itr] == "MetaDataCollection")
				return { true, itr };
		}

		return { false, 0 };
	}

	/*
	class TokenHandler
	{
	public:
		virtual void Handle(MeshTokenList& Tokens, size_t begin, size_t end) = 0;
	};

	using TokenHandlerTable = std::vector<TokenHandler>;
	*/

	bool ParseTokens(MeshTokenList& Tokens, MetaDataList& MD_Out, size_t begin, size_t end)
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

		// Metadata Formats
		// MetaData Declaration			= "[Identifier] : [type] = [Value(s)];"
		// Sub Container definition		= [Identifier] : [ContainerTypeIdentier] { [MetaData Declaration]; };


		ValueList Values;
		for (size_t itr = begin; itr < end && itr < Tokens.size(); ++itr)
		{
			std::string_view token = Tokens.at(itr);

	#define DOSTRCMP(A) (T.size && !strncmp(T.SubStr, A, max(strlen(A), T.size)))
	#define CHECKTAG(A, TYPE) ((A != nullptr) && (A->Type == TYPE))

			// TODO: Reform this into a table
			if(token == "AnimationClip")
			{
#if USING(RESCOMPILERVERBOSE)
				std::cout << "Animation Clip Meta Data Found\n";
#endif
				auto [Values, innerEnd] = ProcessDeclaration(Tokens, itr + 3, end);
				auto ID					= FindValue(Values, "ID");			
				auto begin				= FindValue(Values, "Begin");		
				auto end				= FindValue(Values, "End");			
				auto GUID				= FindValue(Values, "AssetGUID");	

				// Check for ill formed data
	#if _DEBUG
				FK_ASSERT((ID != nullptr),		"MISSING ID		TAG!");
				FK_ASSERT((begin != nullptr),	"MISSING Begin	Value!");
				FK_ASSERT((end != nullptr),		"MISSING End	Value!");
				FK_ASSERT((GUID != nullptr),	"MISSING GUID!");
				FK_ASSERT((ID->Type    == Value::STRING));
				FK_ASSERT((begin->Type == Value::FLOAT));
				FK_ASSERT((end->Type   == Value::FLOAT));
				FK_ASSERT((GUID->Type  == Value::INT));
	#else	
				if ((!ID	|| ID->Type		!= Value::STRING) ||
					(!begin || begin->Type	!= Value::FLOAT)  ||
					(!end	|| end->Type	!= Value::FLOAT)  ||
					(!GUID	|| GUID->Type	!= Value::INT))
					return false;
	#endif

				AnimationClip_MetaData* NewAnimationClip = new AnimationClip_MetaData;

				auto Target = Tokens[itr - 2];

				NewAnimationClip->ClipID	= ID->Data.S;
				NewAnimationClip->ID		= Target;
				NewAnimationClip->T_Start	= begin->Data.F;
				NewAnimationClip->T_End		= end->Data.F;
				NewAnimationClip->guid		= GUID->Data.I;

				MD_Out.push_back(NewAnimationClip);

				itr = innerEnd;
			}
			else if (token == "Skeleton")
			{
#if USING(RESCOMPILERVERBOSE)
				std::cout << "Skeleton Meta Data Found\n";
#endif
				auto [Values, innerEnd] = ProcessDeclaration(Tokens, itr + 3, end);
				auto AssetID			= FindValue(Values, "AssetID");		
				auto AssetGUID			= FindValue(Values, "AssetGUID");	

	#if _DEBUG
				FK_ASSERT((AssetID			!= nullptr), "MISSING ID!");
				FK_ASSERT((AssetID->Type	== Value::STRING));

				FK_ASSERT((AssetGUID		!= nullptr), "MISSING GUID!");
				FK_ASSERT((AssetGUID->Type	== Value::INT));
	#else
				if ((!AssetID || AssetID->Type != Value::STRING) || (!AssetGUID || AssetGUID->Type != Value::INT))
					return false;
	#endif

				Skeleton_MetaData* Skeleton = new Skeleton_MetaData;

				auto Target = Tokens[itr - 2];

				Skeleton->SkeletonID	= AssetID->Data.S;
				Skeleton->SkeletonGUID	= AssetGUID->Data.I;
				Skeleton->ID			= Target;

				MD_Out.push_back(Skeleton);

				itr = innerEnd;
			}
			else if (token == "Model")
			{
#if USING(RESCOMPILERVERBOSE)
				std::cout << "Model Meta Data Found\n";
#endif

				auto [Values, innerEnd]	= ProcessDeclaration(Tokens, itr + 3, end);
				auto AssetID			= FindValue(Values, "AssetID");
				auto AssetGUID			= FindValue(Values, "AssetGUID");
				auto ColliderGUID		= FindValue(Values, "ColliderGUID");
				auto Target				= Tokens[itr - 2];

	#if _DEBUG
				FK_ASSERT((AssetID != nullptr), "MISSING ID!");
				FK_ASSERT((AssetID->Type == Value::STRING));

				FK_ASSERT((AssetGUID != nullptr), "MISSING GUID!");
				FK_ASSERT((AssetGUID->Type == Value::INT));
	#else
				if ((!AssetID || AssetID->Type != Value::STRING) || (!AssetGUID || AssetGUID->Type != Value::INT))
					return false;
	#endif

				Mesh_MetaData* Model = new Mesh_MetaData;

				if (ColliderGUID != nullptr && ColliderGUID->Type == Value::INT)
					Model->ColliderGUID = ColliderGUID->Data.I;
				else
					Model->ColliderGUID = INVALIDHANDLE;

				Model->MeshID	= AssetID->Data.S;
				Model->guid		= AssetGUID->Data.I;
				Model->ID		= Target;

				MD_Out.push_back(Model);

				itr = innerEnd;
			}
			else if (token == "TextureSet")
			{
#if USING(RESCOMPILERVERBOSE)
				std::cout << "TextureSet Meta Data Found\n";
#endif
				auto [Values, innerEnd] = ProcessDeclaration(Tokens, itr + 3, end);
				auto AssetID	  = Tokens[itr - 2];
				auto AssetGUID	  = FindValue(Values, "AssetGUID");
				auto Albedo		  = FindValue(Values, "Albedo");
				auto AlbedoID	  = FindValue(Values, "AlbedoGUID");
				auto RoughMetal	  = FindValue(Values, "RoughMetal");
				auto RoughMetalID = FindValue(Values, "RoughMetalGUID");

				TextureSet_MetaData* TextureSet_Meta = new TextureSet_MetaData;

				if (AssetGUID && AssetGUID->Type == Value::INT) 
					TextureSet_Meta->Guid = AssetGUID->Data.I;

				if (Albedo && Albedo->Type == Value::STRING){
					//auto dest = TextureSet_Meta->Textures.TextureLocation[ETT_ALBEDO].Directory;
					//strncpy(dest, Albedo->Data.S.S, Albedo->Data.S.size);
				}

				if (AlbedoID && AlbedoID->Type == Value::INT) {
					//TextureSet_Meta->Textures.TextureID[ETT_ALBEDO] = AlbedoID->Data.I;
				} else {
					//TextureSet_Meta->Textures.TextureID[ETT_ALBEDO] = INVALIDHANDLE;
				}

				if (RoughMetal && RoughMetal->Type == Value::STRING){
					//auto dest = TextureSet_Meta->Textures.TextureLocation[ETT_ROUGHSMOOTH].Directory;
					//strncpy(dest, RoughMetal->Data.S.S, RoughMetal->Data.S.size);
				}

				if (RoughMetalID && RoughMetalID->Type == Value::INT) {
					//TextureSet_Meta->Textures.TextureID[ETT_ROUGHSMOOTH] = RoughMetalID->Data.I;
				} else {
					//TextureSet_Meta->Textures.TextureID[ETT_ROUGHSMOOTH] = INVALIDHANDLE;
				}

				MD_Out.push_back(TextureSet_Meta);
				itr = innerEnd;
			}
			else if (token == "Scene")
			{
#if USING(RESCOMPILERVERBOSE)
				std::cout << "Scene Meta Data Found\n";
#endif
				auto [Values, innerEnd] = ProcessDeclaration(Tokens, itr + 3, end);
				auto Target				= Tokens[itr - 2];
				auto AssetGUID			= FindValue(Values, "AssetGUID");
				auto AssetID			= FindValue(Values, "AssetID");

				Scene_MetaData* Scene	= new Scene_MetaData;
				Scene->ID				= Target;

				size_t itr2 = itr;
				while(true)
				{
					auto [res, begin] = DetectSubContainer(Tokens, itr2, innerEnd);

					if (Tokens[begin + 1] != "{")
						break;

					if (res) 
					{
						size_t end = SkipBrackets(Tokens, begin + 1);
						ParseTokens(Tokens, Scene->sceneMetaData, begin + 2, innerEnd - 1);
						itr = end;
					}
					else
						break;
				}

				if(AssetGUID != nullptr && AssetGUID->Type == Value::INT)
					Scene->Guid = AssetGUID->Data.I;

				if (AssetID != nullptr && AssetID->Type == Value::STRING)
					Scene->SceneID = AssetID->Data.S;

				MD_Out.push_back(Scene);
				itr = innerEnd;
			}
			else if (token == "Collider")
			{
#if USING(RESCOMPILERVERBOSE)
				std::cout << "Collider Meta Data Found\n";
#endif
				auto [Values, innerEnd] = ProcessDeclaration(Tokens, itr + 3, end);
				auto Target				= Tokens[itr - 2];
				auto AssetGUID			= FindValue(Values,	"AssetGUID");
				auto AssetID			= FindValue(Values,	"AssetID");

				Collider_MetaData& Collider = *new Collider_MetaData;
				Collider.ID = Target;

				if (AssetGUID != nullptr && AssetGUID->Type == Value::INT)
					Collider.Guid = AssetGUID->Data.I;

				if (AssetID != nullptr && AssetID->Type == Value::STRING) 
					Collider.ColliderID = AssetID->Data.S;

				MD_Out.push_back(&Collider);
				itr = innerEnd;
			}
			else if (token == "TerrainCollider")
			{
				auto [Values, innerEnd] = ProcessDeclaration(Tokens, itr + 3, end);
				auto Target				= Tokens[itr - 2];
				auto AssetGUID			= FindValue(Values, "AssetGUID");
				auto AssetID			= FindValue(Values, "AssetID");
				auto BitMapLoc			= FindValue(Values, "BitMapLocation");

				TerrainCollider_MetaData* TerrainCollider = new TerrainCollider_MetaData;

				if (AssetGUID != nullptr && AssetGUID->Type == Value::INT)
					TerrainCollider->Guid = AssetGUID->Data.I;

				if (AssetID != nullptr && AssetID->Type == Value::STRING) 
					TerrainCollider->ColliderID	= AssetID->Data.S;

				if (BitMapLoc != nullptr && BitMapLoc->Type == Value::STRING) 					
					TerrainCollider->BitmapFileLoc = BitMapLoc->Data.S;

				MD_Out.push_back(TerrainCollider);
				itr = innerEnd;
			}
			else if (token == "Font")
			{
#if USING(RESCOMPILERVERBOSE)
				std::cout << "Font Meta Data Found\n";
#endif
				auto [Values, innerEnd] = ProcessDeclaration(Tokens, itr + 3, end);
				auto AssetID			= Tokens[itr - 2];
				auto AssetGUID			= FindValue(Values,	"AssetGUID");
				auto FontFile			= FindValue(Values, "File");

				// Check for ill formed data
	#if _DEBUG
				FK_ASSERT(CHECKTAG(FontFile,	Value::STRING));
				FK_ASSERT(CHECKTAG(AssetGUID,	Value::INT));
	#else	
				if	  (	CHECKTAG(FontFile, Value::STRING) &&
						CHECKTAG(AssetGUID, Value::INT))
					return false;
	#endif

				Font_MetaData* FontData		= new Font_MetaData;
				FontData->UserType			= MetaData::EMETA_RECIPIENT_TYPE::EMR_NONE;
				FontData->type				= MetaData::EMETAINFOTYPE::EMI_FONT;
				FontData->ID				= AssetID;

				if (CHECKTAG(FontFile, Value::STRING))
					FontData->FontFile		= FontFile->Data.S;

				if (CHECKTAG(AssetGUID, Value::INT))
					FontData->Guid			= AssetGUID->Data.I;

				MD_Out.push_back(FontData);
				itr = innerEnd;
			}
			else if (token =="{")
				itr = SkipBrackets(Tokens, itr);

	#undef CHECKTAG
	#undef DOSTRCMP
		}

		return true;
	}


	/************************************************************************************************/


	bool ReadMetaData(const char* Location, iAllocator* Memory, iAllocator* TempMemory, MetaDataList& MD_Out)
	{
		size_t BufferSize = FlexKit::GetFileSize(Location);

#if USING(RESCOMPILERVERBOSE)
		if(!BufferSize)
			std::cout << "Empty File Found!\nThis is Probably an Error.\n";
		else
			std::cout << "Loaded a MetaData File of size: " << BufferSize << "\n";

#endif

		char* Buffer = (char*)TempMemory->malloc(BufferSize);
		LoadFileIntoBuffer(Location, (byte*)Buffer, BufferSize);

		auto Tokens = GetMetaDataTokens(Buffer, BufferSize, TempMemory);

#if USING(RESCOMPILERVERBOSE)
		std::cout << "Found " << Tokens.size() << " TOkens\n";
#endif

		auto res = ParseTokens(Tokens, MD_Out, 0, Tokens.size());

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

		for (auto* meta : MetaData)
		{
			if (meta->UserType == Type && ID == meta->ID)
					RelatedData.push_back(meta);
		}

		return RelatedData;
	}


	/************************************************************************************************/


	Mesh_MetaData* GetMeshMetaData(MetaDataList& MetaData)
	{
		for (auto meta : MetaData)
			if (meta->type == MetaData::EMETAINFOTYPE::EMI_MESH)
				return static_cast<Mesh_MetaData*>(meta);

		return nullptr;
	}

	/************************************************************************************************/




	MetaDataList ScanForRelated(const MetaDataList& metaData, const MetaData::EMETAINFOTYPE type)
	{
		return FilterList(
			[&](auto* metaData)
			{
				return (metaData->type == type);
			},
			metaData);
	}

	/************************************************************************************************/
}// Namespace Flexkit