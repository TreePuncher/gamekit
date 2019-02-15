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
	struct MD_Token
	{
		char*	SubStr;
		size_t	size;
	};


	typedef Vector<MD_Token> MeshTokenList;


	/************************************************************************************************/


	MeshTokenList* GetMetaDataTokens(char* Buffer, size_t BufferSize, iAllocator* Memory)
	{
		MeshTokenList* Tokens = &Memory->allocate_aligned<MeshTokenList>(Memory);

		size_t StartPos = 0;
		size_t CurrentPos = 0;

		auto RemoveWhiteSpaces = [&]()
		{
			bool WhitespaceSkipped = false;
			while (CurrentPos < BufferSize && !WhitespaceSkipped)
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
					WhitespaceSkipped = true;
					break;
				}
			}
		};

		while (CurrentPos < BufferSize)
		{
			auto C = Buffer[CurrentPos];
			if (Buffer[CurrentPos] == ' ' || Buffer[CurrentPos] == '\n' || Buffer[CurrentPos] == '\t')
			{
				RemoveWhiteSpaces();

				char CurrentChar = Buffer[CurrentPos];

				MD_Token NewToken = { Buffer + StartPos, CurrentPos - StartPos - 1 }; // -1 to remove Whitespace at end

				if (NewToken.size)
					Tokens->push_back(NewToken);

				StartPos = CurrentPos;

				RemoveWhiteSpaces();
			}
			else if (Buffer[CurrentPos] == '\0')
				break;
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
		enum TYPE
		{
			INT,
			STRING,
			FLOAT,
		}Type;

		union
		{
			float	F;
			int		I;
			struct
			{
				char*	S;
				size_t	size;
			}S;
		}Data;

		char*	ID;
		size_t	ID_Size;
	};

	typedef static_vector<Value> ValueList;


	/************************************************************************************************/


	void MoveTokenStr(MD_Token T, char* out)
	{
		memset(out, 0x00, T.size + 1);
		strncpy(out, T.SubStr, T.size);

		for (size_t I = T.size; I > 0; --I)
		{
			switch (out[I])
			{
			case '\n':
			case ' ':
				out[I] = '\0';
				T.size--;
			}
		}
	}


	/************************************************************************************************/


	FlexKit::Pair<ValueList, size_t> ProcessDeclaration(iAllocator* Memory, iAllocator* TempMemory, MeshTokenList* Tokens, size_t StartingPosition)
	{
		ValueList Values;
		size_t itr2 = StartingPosition;

		for (; itr2 < Tokens->size(); ++itr2)
		{
			auto T = Tokens->at(itr2);

			if (T.size)
				if (!strncmp(T.SubStr, "int", 3))
				{
					Value NewValue;
					NewValue.Type = Value::INT;

					auto IDToken    = Tokens->at(itr2 - 2);
					auto ValueToken = Tokens->at(itr2 + 2);

					char ValueBuffer[16];
					MoveTokenStr(ValueToken, ValueBuffer);
					int V				= atoi(ValueBuffer);
					NewValue.Data.I		= V;
					NewValue.ID			= (char*)TempMemory->malloc(IDToken.size + 1); // 1 Extra for the Null Terminator
					NewValue.ID_Size	= ValueToken.size;
					MoveTokenStr(IDToken, NewValue.ID);

					Values.push_back(NewValue);
				}
				else if (!strncmp(T.SubStr, "string", 6))
				{
					Value NewValue;
					NewValue.Type = Value::STRING;

					auto IDToken		= Tokens->at(itr2 - 2);
					auto ValueToken		= Tokens->at(itr2 + 2);

					size_t IDSize		= IDToken.size;
					NewValue.ID			= (char*)TempMemory->malloc(IDSize + 1); // 
					NewValue.ID_Size	= IDSize;
					MoveTokenStr(IDToken, NewValue.ID);

					size_t StrSize       = ValueToken.size;
					NewValue.Data.S.S    = (char*)TempMemory->malloc(StrSize + 1); // 
					NewValue.Data.S.size = StrSize;
					MoveTokenStr(ValueToken, NewValue.Data.S.S);

					Values.push_back(NewValue);
				}
				else if (!strncmp(T.SubStr, "float", 5))
				{
					Value NewValue;
					NewValue.Type = Value::FLOAT;

					auto IDToken = Tokens->at(itr2 - 2);
					auto ValueToken = Tokens->at(itr2 + 2);

					char ValueBuffer[16];
					MoveTokenStr(ValueToken, ValueBuffer);
					float V         = atof(ValueBuffer);
					NewValue.Data.F = V;

					NewValue.ID = (char*)TempMemory->malloc(IDToken.size + 1); // 
					MoveTokenStr(IDToken, NewValue.ID);

					Values.push_back(NewValue);
				}
				else if (!strncmp(T.SubStr, "};", min(T.size, 2)))
					return{ Values, itr2 };
		}

		// Should Be Un-reachable
		return{ Values, itr2 };
	}


	/************************************************************************************************/


	size_t SkipBrackets(MeshTokenList* Tokens, size_t StartingPosition)
	{
		size_t itr2 = StartingPosition;

		for (; itr2 < Tokens->size(); ++itr2)
		{
			auto T = Tokens->at(itr2);

			if (T.size && (!strncmp(T.SubStr, "};", min(T.size, 2))))
				return itr2;
		}

		// Malformed Document if code reaches here
		FK_ASSERT(0);
		return -1;
	}


	/************************************************************************************************/


	Value* FindValue(static_vector<Value>& Values, char* ValueID)
	{
		auto res = FlexKit::find(Values, [&](Value& V) {
			return (!strncmp(V.ID, ValueID, strlen(ValueID))); });

		return (res == Values.end()) ? nullptr : res;
	}


	/************************************************************************************************/


	bool ProcessTokens(iAllocator* Memory, iAllocator* TempMemory, MeshTokenList* Tokens, MD_Vector& MD_Out)
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
		// {Identifier} : {type} = {Value(s)};

		ValueList Values;
		for (size_t itr = 0; itr < Tokens->size(); ++itr)
		{
			auto T = Tokens->at(itr);

	#define DOSTRCMP(A) (T.size && !strncmp(T.SubStr, A, max(strlen(A), T.size)))
	#define CHECKTAG(A, TYPE) ((A != nullptr) && (A->Type == TYPE))

			// TODO: Reform this into a table
			if(DOSTRCMP("AnimationClip"))
			{
#if USING(RESCOMPILERVERBOSE)
				std::cout << "Animation Clip Meta Data Found\n";
#endif
				auto res    = ProcessDeclaration(Memory, TempMemory, Tokens, itr);
				auto Values = res.V1;
				auto ID     = FindValue(Values, "ID");			
				auto begin  = FindValue(Values, "Begin");		
				auto end    = FindValue(Values, "End");			
				auto GUID   = FindValue(Values, "AssetGUID");	

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

				AnimationClip_MetaData* NewAnimationClip = &Memory->allocate_aligned<AnimationClip_MetaData>();

				auto Target = Tokens->at(itr - 2);

				strncpy(NewAnimationClip->ClipID, ID->Data.S.S, ID->Data.S.size);

				NewAnimationClip->SetID(Target.SubStr, Target.size);
				NewAnimationClip->T_Start	= begin->Data.F;
				NewAnimationClip->T_End		= end->Data.F;
				NewAnimationClip->guid		= GUID->Data.I;

				MD_Out.push_back(NewAnimationClip);

				itr = res;
			}
			else if (DOSTRCMP("Skeleton"))
			{
#if USING(RESCOMPILERVERBOSE)
				std::cout << "Skeleton Meta Data Found\n";
#endif
				auto res		= ProcessDeclaration(Memory, TempMemory, Tokens, itr);
				auto Values		= res.V1;
				auto AssetID	= FindValue(Values, "AssetID");		
				auto AssetGUID	= FindValue(Values, "AssetGUID");	

	#if _DEBUG
				FK_ASSERT((AssetID			!= nullptr), "MISSING ID!");
				FK_ASSERT((AssetID->Type	== Value::STRING));

				FK_ASSERT((AssetGUID		!= nullptr), "MISSING GUID!");
				FK_ASSERT((AssetGUID->Type	== Value::INT));
	#else
				if ((!AssetID || AssetID->Type != Value::STRING) || (!AssetGUID || AssetGUID->Type != Value::INT))
					return false;
	#endif

				Skeleton_MetaData* Skeleton = &Memory->allocate_aligned<Skeleton_MetaData>();

				auto Target = Tokens->at(itr - 2);

				strncpy(Skeleton->SkeletonID, AssetID->Data.S.S, AssetID->Data.S.size);


				Skeleton->SetID(Target.SubStr, Target.size);
				Skeleton->SkeletonGUID	= AssetGUID->Data.I;

				MD_Out.push_back(Skeleton);

				itr = res;
			}
			else if (DOSTRCMP("Model"))
			{
#if USING(RESCOMPILERVERBOSE)
				std::cout << "Model Meta Data Found\n";
#endif

				auto res			= ProcessDeclaration(Memory, TempMemory, Tokens, itr);
				auto Values			= res.V1;
				auto AssetID		= FindValue(Values, "AssetID");
				auto AssetGUID		= FindValue(Values, "AssetGUID");
				auto ColliderGUID	= FindValue(Values, "ColliderGUID");
				auto Target			= Tokens->at(itr - 2);

	#if _DEBUG
				FK_ASSERT((AssetID != nullptr), "MISSING ID!");
				FK_ASSERT((AssetID->Type == Value::STRING));

				FK_ASSERT((AssetGUID != nullptr), "MISSING GUID!");
				FK_ASSERT((AssetGUID->Type == Value::INT));
	#else
				if ((!AssetID || AssetID->Type != Value::STRING) || (!AssetGUID || AssetGUID->Type != Value::INT))
					return false;
	#endif

				Mesh_MetaData* Model = &Memory->allocate_aligned<Mesh_MetaData>();

				if (ColliderGUID != nullptr && ColliderGUID->Type == Value::INT)
					Model->ColliderGUID = ColliderGUID->Data.I;
				else
					Model->ColliderGUID = INVALIDHANDLE;

				strncpy(Model->MeshID, AssetID->Data.S.S, AssetID->Data.S.size);

				for (size_t I = AssetID->Data.S.size; I > 0; --I)
				{
					switch (Model->MeshID[I])
					{
						case '\n':
						case ' ':
							Model->MeshID[I] = '\0';
							AssetID->Data.S.size--;
					}
				}

				Model->SetID(Target.SubStr, Target.size + 1);
				Model->guid = AssetGUID->Data.I;

				MD_Out.push_back(Model);

				itr = res;
			}
			else if (DOSTRCMP("TextureSet"))
			{
#if USING(RESCOMPILERVERBOSE)
				std::cout << "TextureSet Meta Data Found\n";
#endif
				auto res		  = ProcessDeclaration(Memory, TempMemory, Tokens, itr);
				auto Values		  = res.V1;
				auto AssetID	  = Tokens->at(itr - 2);
				auto AssetGUID	  = FindValue(Values, "AssetGUID");
				auto Albedo		  = FindValue(Values, "Albedo");
				auto AlbedoID	  = FindValue(Values, "AlbedoGUID");
				auto RoughMetal	  = FindValue(Values, "RoughMetal");
				auto RoughMetalID = FindValue(Values, "RoughMetalGUID");

				TextureSet_MetaData* TextureSet_Meta = &Memory->allocate_aligned<TextureSet_MetaData>();

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
				itr = res;
			}
			else if (DOSTRCMP("Scene"))
			{
#if USING(RESCOMPILERVERBOSE)
				std::cout << "Scene Meta Data Found\n";
#endif
				auto res        = ProcessDeclaration(Memory, TempMemory, Tokens, itr);
				auto Values     = res.V1;
				auto Target		= Tokens->at(itr - 2);
				auto AssetGUID  = FindValue(Values, "AssetGUID");
				auto AssetID	= FindValue(Values, "AssetID");

				Scene_MetaData& Scene		= Memory->allocate<Scene_MetaData>();
				Scene.SetID(Target.SubStr, Target.size);

				if(AssetGUID != nullptr && AssetGUID->Type == Value::INT)
					Scene.Guid = AssetGUID->Data.I;

				if (AssetID != nullptr && AssetID->Type == Value::STRING) {
					strncpy(Scene.SceneID, AssetID->Data.S.S, min(AssetID->Data.S.size, 64));
					Scene.SceneIDSize = AssetID->Data.S.size;
				}

				MD_Out.push_back(&Scene);
				itr = res;
			}
			else if (DOSTRCMP("Collider"))
			{
#if USING(RESCOMPILERVERBOSE)
				std::cout << "Collider Meta Data Found\n";
#endif
				auto res		= ProcessDeclaration(Memory, TempMemory, Tokens, itr);
				auto Values		= res.V1;
				auto Target		= Tokens->at(itr - 2);
				auto AssetGUID	= FindValue(Values,	"AssetGUID");
				auto AssetID	= FindValue(Values,	"AssetID");

				Collider_MetaData& Collider = Memory->allocate<Collider_MetaData>();
				Collider.SetID(Target.SubStr, Target.size);

				if (AssetGUID != nullptr && AssetGUID->Type == Value::INT)
					Collider.Guid = AssetGUID->Data.I;

				if (AssetID != nullptr && AssetID->Type == Value::STRING) {
					strncpy(Collider.ColliderID, AssetID->Data.S.S, min(AssetID->Data.S.size, 64));
					Collider.ColliderIDSize = AssetID->Data.S.size;
				}

				MD_Out.push_back(&Collider);
				itr = res;
			}
			else if (DOSTRCMP("TerrainCollider"))
			{
				auto res       = ProcessDeclaration(Memory, TempMemory, Tokens, itr);
				auto Values    = res.V1;
				auto Target    = Tokens->at(itr - 2);
				auto AssetGUID = FindValue(Values, "AssetGUID");
				auto AssetID   = FindValue(Values, "AssetID");
				auto BitMapLoc = FindValue(Values, "BitMapLocation");

				TerrainCollider_MetaData TerrainCollider = Memory->allocate<TerrainCollider_MetaData>();

				if (AssetGUID != nullptr && AssetGUID->Type == Value::INT)
					TerrainCollider.Guid = AssetGUID->Data.I;

				if (AssetID != nullptr && AssetID->Type == Value::STRING) {
					strncpy(TerrainCollider.ColliderID, AssetID->Data.S.S, min(AssetID->Data.S.size, 64));
					TerrainCollider.ColliderIDSize = AssetID->Data.S.size;
				}

				if (BitMapLoc != nullptr && BitMapLoc->Type == Value::STRING) {
					strncpy(TerrainCollider.BitmapFileLoc, BitMapLoc->Data.S.S, min(BitMapLoc->Data.S.size, 256));
					TerrainCollider.BitmapFileLocSize = BitMapLoc->Data.S.size;
				}
				MD_Out.push_back(&TerrainCollider);
				itr = res;
			}
			else if (DOSTRCMP("Font")) 
			{
#if USING(RESCOMPILERVERBOSE)
				std::cout << "Font Meta Data Found\n";
#endif
				auto res		= ProcessDeclaration(Memory, TempMemory, Tokens, itr);
				auto Values		= res.V1;
				auto AssetID	= Tokens->at(itr - 2);
				auto AssetGUID	= FindValue(Values,	"AssetGUID");
				auto FontFile	= FindValue(Values, "File");

				// Check for ill formed data
	#if _DEBUG
				FK_ASSERT(CHECKTAG(FontFile,	Value::STRING));
				FK_ASSERT(CHECKTAG(AssetGUID,	Value::INT));
	#else	
				if	  (	CHECKTAG(FontFile, Value::STRING) &&
						CHECKTAG(AssetGUID, Value::INT))
					return false;
	#endif

				Font_MetaData& FontData = Memory->allocate<Font_MetaData>();
				FontData.UserType       = MetaData::EMETA_RECIPIENT_TYPE::EMR_NONE;
				FontData.type           = MetaData::EMETAINFOTYPE::EMI_FONT;
				FontData.SetID(AssetID.SubStr, AssetID.size);

				if (CHECKTAG(FontFile, Value::STRING))
					strncpy(FontData.FontFile, FontFile->Data.S.S, min(FontFile->Data.S.size, sizeof(FontData.FontFile)));

				if (CHECKTAG(AssetGUID, Value::INT))
					FontData.Guid = AssetGUID->Data.I;

				MD_Out.push_back(&FontData);
				itr = res;
			}
			else if (DOSTRCMP("{"))
				itr = SkipBrackets(Tokens, itr);

	#undef CHECKTAG
	#undef DOSTRCMP
		}

		return true;
	}


	/************************************************************************************************/


	bool ReadMetaData(const char* Location, iAllocator* Memory, iAllocator* TempMemory, MD_Vector& MD_Out)
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
		std::cout << "Found " << Tokens->size() << " TOkens\n";
#endif

		auto res = ProcessTokens(Memory, TempMemory, Tokens, MD_Out);

		return res;
	}


	/************************************************************************************************/


	void PrintMetaDataList(MD_Vector& MD)
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


	Vector<size_t> FindRelatedMetaData(MD_Vector* MetaData, MetaData::EMETA_RECIPIENT_TYPE Type, const char* ID, iAllocator* TempMem)
	{
		Vector<size_t> RelatedData(TempMem);
		size_t IDLength = strlen(ID);

		for (size_t I = 0; I < MetaData->size(); ++I)
		{
			auto& MD = (*MetaData)[I];
			if (MD->UserType == Type)
				if (!strncmp(MD->ID, ID, IDLength))
					RelatedData.push_back(I);
		}

		return RelatedData;
	}


	/************************************************************************************************/


	Mesh_MetaData* GetMeshMetaData(MD_Vector* MetaData, Vector<size_t>& related)
	{
		for (auto I : related)
			if (MetaData->at(I)->type == MetaData::EMETAINFOTYPE::EMI_MESH)
				return (Mesh_MetaData*)MetaData->at(I);

		return nullptr;
	}

	/************************************************************************************************/


	MetaData* ScanForRelated(MD_Vector* MetaData, RelatedMetaData& Related, MetaData::EMETAINFOTYPE Type)
	{
		for (auto I : Related)
			if (MetaData->at(I)->type == Type)
				return (Mesh_MetaData*)MetaData->at(I);

		return nullptr;
	}

	/************************************************************************************************/
}// Namespace Flexkit