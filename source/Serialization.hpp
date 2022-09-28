#pragma once

#include <any>
#include <assert.h>
#include <cstdio>
#include <functional>
#include <map>
#include <type_traits>
#include <typeinfo>
#include <vector>
#include <any>
#include <variant>
#include <optional>
#include <limits>
#include "static_vector.h"

namespace FlexKit
{   /************************************************************************************************/


	class Blob
	{
	public:

		Blob() = default;


		template<typename TY>
		Blob(const TY& IN_struct)
		{
			//static_assert(std::is_pod_v<TY>, "POD types only!");

			buffer.resize(sizeof(IN_struct));
			memcpy(data(), &IN_struct, sizeof(TY));
		}


		Blob(const char* IN_buffer, const size_t size)
		{
			buffer.resize(size);
			memcpy(data(), IN_buffer, size);
		}


		Blob operator + (const Blob& rhs_blob)
		{
			Blob out;

			out += *this;
			out += rhs_blob;

			return out;
		}


		Blob& operator += (const Blob& rhs_blob)
		{
			const size_t offset = buffer.size();

			buffer.resize(buffer.size() + rhs_blob.size());
			memcpy(buffer.data() + offset, rhs_blob, rhs_blob.size());

			return *this;
		}


		size_t size() const
		{
			return buffer.size();
		}


		void resize(size_t newSize)
		{
			buffer.resize(newSize);
		}


		std::byte* data()
		{
			return buffer.data();
		}


		operator const std::byte* () const
		{
			return buffer.data();
		}


		std::pair<std::byte*, const size_t> Release()
		{
			std::byte* _ptr = new std::byte[buffer.size()];

			memcpy(_ptr, buffer.data(), buffer.size());
			const size_t size = buffer.size();
			buffer.clear();

			return { _ptr, size };
		}


		void Serialize(auto& ar)
		{
			ar& buffer;
		}


		std::vector<std::byte> buffer;
	};


	inline void WriteBlob(const Blob& b, FILE* f)
	{
		size_t bytesWritten = 0;

		do
		{
			bytesWritten += fwrite(b.buffer.data() + bytesWritten, 1, b.buffer.size() - bytesWritten, f);
		}while(bytesWritten < b.size());
	}

	/************************************************************************************************/


	class Archive;

	using TypeID_t = uint64_t;


	class SaveArchiveContext;
	class LoadFileArchiveContext;
	class LoadBlobArchiveContext;

	class SerializableBase
	{
	public:

		virtual void _Serialize(SaveArchiveContext& archive) {}
		virtual void _Serialize(LoadFileArchiveContext& archive) {}
		virtual void _Serialize(LoadBlobArchiveContext& archive) {}

		virtual TypeID_t GetTypeID() const noexcept { return -1; }

		static SerializableBase* Construct(size_t ID)
		{
			if (auto res = factories.find(ID); res != factories.end())
				return res->second();
			else
				return nullptr;
		}

		inline static std::map<size_t, std::function<SerializableBase* ()>> factories;
	};


	/************************************************************************************************/


	class SerializableInterfaceBase : 
		public SerializableBase
	{
	public:
		virtual TypeID_t GetInterfaceTypeID() const noexcept { return -1; }
	};


	/************************************************************************************************/


	template<typename TY, typename TY_Interface, typename TypeID_t typeID>
	class Serializable : 
		public TY_Interface
	{
	public:
		Serializable() = default;

		template<typename ... TY_ARGS>
		Serializable(TY_ARGS ... args) : TY_Interface{ std::forward<TY_ARGS>(args)... } {}

		void _Serialize(SaveArchiveContext& archive) final
		{
			static_cast<TY*>(this)->Serialize(archive);
		}

		void _Serialize(LoadFileArchiveContext& archive) final
		{
			static_cast<TY*>(this)->Serialize(archive);
		}

		void _Serialize(LoadBlobArchiveContext& archive) final
		{
			static_cast<TY*>(this)->Serialize(archive);
		}

		size_t GetID() { return typeID; }

		virtual TypeID_t GetTypeID() const noexcept final { return typeID; }

		static TY* construct()
		{
			return new TY{};
		}


	private:

		static SerializableBase* Create()
		{
			return new TY;
		}

		static bool Register()
		{
			SerializableBase::factories[typeID] =
				[]() -> SerializableBase*
			{
				return Create();// new TY{};
			};

			return true;
		}

		inline const static bool _registered = Register();
	};


	/************************************************************************************************/


	template<TypeID_t typeID>
	class SerializableInterface :
		public SerializableInterfaceBase
	{
	public:
		TypeID_t GetInterfaceTypeID() const noexcept { return typeID; }
	};


	/************************************************************************************************/


	using DeferredSerializer = std::function<void(SaveArchiveContext& ctx)>;


	/************************************************************************************************/


	struct StreamedValue
	{
	};


	/************************************************************************************************/


	namespace SerializeableUtilities
	{
		template<typename TY>
		struct is_pointer
		{
			constexpr operator bool() { return std::is_pointer_v<TY>; }
		};

		template<typename TY>
		struct is_pointer<std::shared_ptr<TY>>
		{
			constexpr operator bool() { return true; }
			static constexpr bool value = true;
		};

		template<typename TY>
		struct is_pointer<std::weak_ptr<TY>>
		{
			constexpr operator bool() { return true; }
			static constexpr bool value = true;
		};

		template<typename TY>
		struct is_serializable_ptr
		{
			constexpr operator bool() { return false; }
			static constexpr bool value = false;

			using type = void;
		};

		template<typename TY>
		struct is_serializable_ptr<std::shared_ptr<TY>>
		{
			constexpr operator bool() { return true; }
			static constexpr bool value = true;

			using type = TY;
		};

		template<typename TY>
		struct is_serializable_ptr<std::weak_ptr<TY>>
		{
			constexpr operator bool() { return true; }
			static constexpr bool value = true;

			using type = TY;
		};

		template<typename TY, typename ... EXTRA>
		struct is_serializable_ptr<std::unique_ptr<TY, EXTRA...>>
		{
			constexpr operator bool() { return false; }
			static constexpr bool value = false;

			using type = TY;
		};
	}


	template<typename TY>
	constexpr bool is_serializable_ptr = SerializeableUtilities::is_serializable_ptr<TY>::value;


	template<typename TY>
	TypeID_t GetTypeID(TY& value)
	{
		return typeid(value).hash_code();
	}

	template<typename TY>
	requires std::is_base_of_v<SerializableBase, TY>
	TypeID_t GetTypeID(TY& value)
	{
		return value.GetTypeID();
	}

	template<typename TY>
	requires is_serializable_ptr<TY> && std::is_base_of_v<SerializableBase, typename SerializeableUtilities::is_serializable_ptr<TY>::type>
	TypeID_t GetTypeID(TY& value)
	{
		return value->GetTypeID();
	}

	template<typename TY, typename TY_archive>
	concept SerializableValue = requires(TY t, TY_archive& ar) 
	{
		Serialize(ar, t);
	};

	template<typename TY, typename TY_archive>
	concept SerializableStruct = requires(TY t, TY_archive & ar)
	{
		t.Serialize(ar);
	} && !SerializableValue<TY, TY_archive>;

	template<typename TY, typename TY_archive>
	concept TrivialCopy =
		std::is_trivially_copyable_v<TY> &&
		!SerializableValue<TY, TY_archive> &&
		!SerializableStruct<TY, TY_archive>;

	template<typename TY> concept SerializableInterfacePointer = std::is_base_of_v<SerializableBase, TY>;//&& std::is_abstract_v<TY>;

	struct PointerValue
	{
		uint64_t ID;
		uint64_t offset;
	};

	struct PointerTableHeader
	{
		uint64_t size;
		uint64_t hash;
	};

	struct VectorHeader
	{
		TypeID_t type;
		uint64_t size;
	};

	struct StringHeader
	{
		uint64_t stringSize;
	};

	struct MapHeader
	{
		uint64_t size;
	};

	struct PointerHeader
	{
		uint64_t typeID;
		uint64_t pointerID;
	};

	struct RawBuffer
	{
		void*&  _ptr;
		size_t& size;
	};

	std::optional<Blob>     CompressBuffer      (const RawBuffer& buffer);
	std::optional<void*>    DecompressBuffer    (const RawBuffer& buffer, const size_t decompressedBufferSize);

	constexpr const char* stringTest() { return "Testing"; }

	class SaveArchiveContext
	{
	public:
		SaveArchiveContext() = default;

		SaveArchiveContext(const SaveArchiveContext&)               = delete;
		SaveArchiveContext(SaveArchiveContext&&)                    = delete;

		SaveArchiveContext& operator = (const SaveArchiveContext&)  = delete;
		SaveArchiveContext& operator = (SaveArchiveContext&&)       = delete;

		static consteval bool Loading() noexcept { return false; }

		std::map<std::string, SerializableBase*>    serializers;
		std::map<void*, DeferredSerializer>         deferred;

		std::vector<Blob>           dataBuffer = { Blob{} };
		std::vector<PointerValue>   pointerTable;


		//template<typename TY>
		//void _Serialize(TY& value)
	   // {
		//    static_assert(false, "Unable to serialize a value. Please add a void ::Serialize(type&) overload, or inherit from Serializable, or SerializableInterface");
	   // }


		void _Serialize(SerializableInterfacePointer auto& value)
		{
			const TypeID_t typeID = GetTypeID(value);
			dataBuffer.back() += { typeID };

			value._Serialize(*this);
		}


		template<typename TY>
		requires TrivialCopy<TY, SaveArchiveContext>
		void _Serialize(TY& value)
		{
			dataBuffer.back() += Blob{ value };
		}


		template<typename TY>
		requires(SerializableValue<TY, SaveArchiveContext>)
		void _Serialize(TY& value)
		{
			Serialize(*this, value);
		}


		template<typename TY>
		requires(   SerializableStruct<TY, SaveArchiveContext> &&
					!SerializableInterfacePointer<TY>)

		void _Serialize(TY& value)
		{
			value.Serialize(*this);
		}


		template<typename TY>
		requires(SerializableStruct<TY, SaveArchiveContext>)
		void _Serialize(TY* value)
		{
			dataBuffer.emplace_back();

			value->Serialize(*this);

			*(++dataBuffer.rbegin()) += dataBuffer.back();
			dataBuffer.pop_back();
		}


		void _Serialize(const std::string& string)
		{
			dataBuffer.back() += StringHeader{ string.size() };
			dataBuffer.back() += Blob{ string.data(), string.size() };
		}


		template<typename TY>
		void SerializePointer(TY& value)
		{
			if constexpr (is_serializable_ptr<TY>)
			{
				if(value != nullptr)
				{
					auto _ptr       = value.get();
					auto typeID     = GetTypeID(value);

					if(_ptr != nullptr)
						deferred[(void*)_ptr] =
							[&, _ptr = _ptr](SaveArchiveContext& ar) 
							{ 
								_Serialize(*_ptr); 
							};

					dataBuffer.back() += Blob{ PointerHeader{ PointerHeader{ typeID, (uint64_t)_ptr}}};
				}
				else
				{
					dataBuffer.back() += PointerHeader{ 0, (uint64_t)nullptr };
				}
			}
			else
			{
				auto typeID = typeid(TY).hash_code();

				if (value != nullptr)
					deferred[value] =
						[&](SaveArchiveContext& ar) 
						{ 
							_Serialize(value); 
						};

				dataBuffer.back() += Blob{ PointerHeader{ typeID, (uint64_t)value } };
			}
		}


		template<typename TY>
		void SerializeValue(TY& value)
		{
			_Serialize(value);
		}


		template<typename ... TY_Types>
		void SerializeValue(std::variant<TY_Types...>& variant)
		{
			size_t i = variant.index();
			dataBuffer.back() += i;

			if (i != std::variant_npos)
				std::visit(
					[&](auto& value) 
					{
						SerializeValue(value);
					}, variant);
		}


		template<typename TY_Key, typename TY_Value>
		void SerializeValue(std::map<TY_Key, TY_Value>& value_map)
		{
			using namespace SerializeableUtilities;

			const auto& typeID = typeid(std::map<TY_Key, TY_Value>);

			dataBuffer.back() += MapHeader{ value_map.size() };

			for (auto& element : value_map)
			{
				if constexpr (is_pointer<TY_Key>())
					SerializeValue(*element.first);
				else
					SerializeValue(element.first);

				if constexpr (is_pointer<TY_Value>())
					SerializeValue(*element.second);
				else
					SerializeValue(element.second);
			}
		}


		template<typename TY>
		void SerializeValue(std::vector<TY>& vector)
		{
			using namespace SerializeableUtilities;

			const auto& typeID = typeid(TY).hash_code();
			dataBuffer.back() += VectorHeader{ typeID, vector.size() };

			for (auto& element : vector)
			{
				if constexpr (is_pointer<TY>())
					SerializePointer(element);
				else
					SerializeValue(element);
			}
		}


		template<typename TY>
		void operator & (TY& rhs)
		{
			using namespace SerializeableUtilities;

			if constexpr (is_pointer<TY>())
				SerializePointer(rhs);
			else
				SerializeValue(rhs);
		}


		void operator & (RawBuffer&& rhs)
		{
			if (rhs.size < 4096)
			{
				dataBuffer.back() += rhs.size;
				dataBuffer.back() += Blob{ (const char*)rhs._ptr, rhs.size };
			}
			else
			{
				if (auto compressed = CompressBuffer(rhs); compressed)
				{
					dataBuffer.back() += rhs.size;
					dataBuffer.back() += compressed.value().buffer.size();
					dataBuffer.back() += compressed.value();
				}
				else
				{
					dataBuffer.back() += 0;
					dataBuffer.back() += 0;
				}
			}
		}


		void SerializeDeferred()
		{
			assert(dataBuffer.size() == 1);

			do
			{
				auto temp = std::move(deferred);
				deferred.clear();

				for (auto& d : temp)
				{
					pointerTable.emplace_back(
						PointerValue{
							reinterpret_cast<uint64_t>(d.first),
							static_cast<uint64_t>(dataBuffer.back().size()) });

					d.second(*this);
				}
			} while (deferred.size());

			PointerTableHeader header{ pointerTable.size(), sizeof(PointerValue) * pointerTable.size() };
			size_t pointerTableSize = sizeof(PointerTableHeader) + sizeof(PointerValue) * pointerTable.size();

			Blob pointerTableBlob;
			for (auto& pointerKey : pointerTable)
			{
				pointerKey.offset   += pointerTableSize;
				pointerTableBlob    += pointerKey;
			}

			/*
			* { Header }
			*   [PointerTable]
			* { DataBlocks   }
			*   [bytes] * n
			*   [void*, size_t] * n
			* { DataBlocks   }
			*/

			dataBuffer.back() = Blob{ header } + pointerTableBlob + dataBuffer.back();
		}


		void MappedValue(uint64_t ID, auto& value)
		{
			deferred[(void*)ID] =
				[&](SaveArchiveContext& ar)
				{
					if constexpr (is_serializable_ptr<decltype(value)>)
						(*this)& *value;
					else
						(*this)& value;
				};
		}

		Blob GetBlob()
		{
			SerializeDeferred();

			return dataBuffer.front();
		}
	};


	struct ArchiveFieldHeader
	{
		uint64_t    fieldSize;
		uint64_t    vectorSize;

		char        ID[64];
		char        type[64];
	};


	/************************************************************************************************/


	template<typename ... TY_Types>
	struct VariantDeserializationHelper
	{
		static constexpr size_t TypeListLength = sizeof ... (TY_Types);

		template<size_t idx = 0, typename TY_Head, typename ... TY_Tail>
		static void _Construct(auto& archive, auto& variant_out, size_t I)
		{
			if (I == idx)
			{
				variant_out = TY_Head{};
				archive& std::get<idx>(variant_out);
			}
			else
			{
				if constexpr (idx + 1 < TypeListLength)
					_Construct<idx + 1, TY_Tail ...>(archive, variant_out, I);
			}
		}


		static void Construct(auto& archive, auto& variant_out, size_t idx)
		{
			if(idx < sizeof ... (TY_Types))
				_Construct<0, TY_Types ...>(archive, variant_out, idx);
		}
	};


	class LoadFileArchiveContext
	{
	public:

		LoadFileArchiveContext(FILE* file) : f { file } 
		{
			PointerTableHeader pointerHeader;

			fread(&pointerHeader, 1, sizeof(pointerHeader), file);

			for (size_t I = 0; I < pointerHeader.size; I++)
			{
				PointerValue e;
				auto res = fread(&e, 1, sizeof(e), file);

				if (res != sizeof(e) && ferror(f))
					assert(false);

				pointerTable[e.ID] = { e.offset };
			}
		}


		~LoadFileArchiveContext()
		{
			if(f)
				fclose(f);

			f = nullptr;
		}


		LoadFileArchiveContext              (const LoadFileArchiveContext&) = delete;
		LoadFileArchiveContext& operator =  (const LoadFileArchiveContext&) = delete;

		LoadFileArchiveContext              (LoadFileArchiveContext&&) = delete;
		LoadFileArchiveContext& operator =  (LoadFileArchiveContext&&) = delete;

		void Read(void* dest, size_t size) noexcept
		{
			fread(dest, 1, size, f);

			offset += size;
		}

		void Seek(size_t pos) noexcept
		{
			assert(pos < std::numeric_limits<long>::max());

			fseek(f, (long)pos, 0);
		}

		size_t Tell() const noexcept
		{
			return ftell(f);
		}

		static consteval bool Loading() noexcept { return true; }

		template<typename TY>
		requires(TrivialCopy<TY, LoadFileArchiveContext>)
		void _Deserialize(TY& value)
		{
			fread(&value, 1, sizeof(TY), f);
		}

		template<typename ... TY_Types>
		void _Deserialize(std::variant<TY_Types...>& variant)
		{
			size_t i = -1;
			fread(&i, 1, sizeof(i), f);

			if (i != std::variant_npos)
				VariantDeserializationHelper<TY_Types...>::Construct(
					*this, variant, i);
		}

		template<typename TY>
		requires(SerializableValue<TY, LoadFileArchiveContext>)
		void _Deserialize(TY& value)
		{
			Serialize(*this, value);
		}


		void _Deserialize(std::string& value)
		{
			StringHeader header;
			fread(&header, 1, sizeof(header), f);

			value.resize(header.stringSize);

			fread(value.data(), 1, header.stringSize, f);
		}


		void _Deserialize(SerializableInterfacePointer auto& value)
		{
			uint64_t interfaceID;
			fread(&interfaceID, sizeof(interfaceID), 1, f);

			const TypeID_t expectedID = GetTypeID(value);
			FK_ASSERT(interfaceID == expectedID);

			value._Serialize(*this);
		}


		template<typename TY>
		requires(SerializableStruct<TY, LoadFileArchiveContext> && !SerializableInterfacePointer<TY>)
		void _Deserialize(TY& value)
		{
			value.Serialize(*this);
		}


		template<typename TY_Key, typename TY_Value>
		void _Deserialize(std::map<TY_Key, TY_Value>& value_map)
		{
			using namespace SerializeableUtilities;

			MapHeader header{};
			fread(&header, 1, sizeof(header), f);

			for (size_t I = 0; I < header.size; I++)
			{
				TY_Key      key;
				TY_Value    value;

				_Deserialize(key);
				_Deserialize(value);

				value_map[key] = std::move(value);
			}
		}


		template<typename TY>
		void DeserializeValue(TY& value)
		{
			using namespace SerializeableUtilities;

			_Deserialize(value);
		}


		template<typename TY>
		void DeserializeValue(std::vector<TY>& vector)
		{
			const auto& typeID = typeid(TY).hash_code();

			auto temp = ftell(f);

			VectorHeader header = { 0, 0 };
			if (const auto res = fread(&header, 1, sizeof(header), f); res != sizeof(header))
			{
				if (ferror(f))
				{
					if(feof(f))
						assert(false);// "End of file reached!");
				}
				assert(false);// "Unknown IO error!");
			}
			else
				assert(typeID == header.type);

			vector.clear();
			vector.reserve(header.size);

			for (size_t I = 0; I < header.size; I++)
			{
				TY value;

				if constexpr (is_serializable_ptr<TY>)
				{
					if (value == nullptr)
					{
						if constexpr (is_serializable_ptr<TY>)
							value = std::make_shared<typename SerializeableUtilities::is_serializable_ptr<TY>::type>();
					}
					DeserializePointer(value);
				}
				else
					DeserializeValue(value);

				vector.emplace_back(std::move(value));
			}
		}


		template<template<typename ... > typename Ptr_Ty, typename TY>
		void DeserializePointer(Ptr_Ty<TY>& value) requires (is_serializable_ptr<Ptr_Ty<TY>> && std::is_base_of_v<FlexKit::SerializableInterfaceBase, TY>)
		{
			using namespace SerializeableUtilities;

			PointerHeader header;
			fread(&header, 1, sizeof(header), f);

			if (header.pointerID == 0)
				return;

			if (pointerTable[header.pointerID]._ptr.has_value())
			{
				value = std::any_cast<Ptr_Ty<TY>>(pointerTable[header.pointerID]._ptr);
			}
			else
			{
				auto temp   = ftell(f);
				auto offset = static_cast<long>(pointerTable[header.pointerID].offset);

				fseek(f, offset, 0);

				TypeID_t typeID;
				fread(&typeID, 1, sizeof(typeID), f);

				assert(typeID == header.typeID);

				auto newValue = FlexKit::SerializableBase::Construct(typeID);
				newValue->_Serialize(*this);

				value = Ptr_Ty<TY>(dynamic_cast<TY*>(newValue));

				pointerTable[header.pointerID]._ptr = value;

				fseek(f, temp, 0);
			}
		}


		template<template<typename ... > typename Ptr_Ty, typename TY>
		void DeserializePointer(Ptr_Ty<TY>& value) requires (is_serializable_ptr<Ptr_Ty<TY>> && !std::is_base_of_v<FlexKit::SerializableInterfaceBase, TY>)
		{
			using namespace SerializeableUtilities;

			uint64_t pointerValue[2];
			fread(pointerValue, 1, 16, f);

			if (pointerTable[pointerValue[1]]._ptr.has_value())
			{
				auto& any_ptr = pointerTable[pointerValue[1]]._ptr;
				value = std::any_cast<std::shared_ptr<TY>>(any_ptr);
			}
			else if(pointerValue[1] != 0)
			{
				assert(pointerTable.find(pointerValue[1]) != pointerTable.end());

				auto temp   = ftell(f);
				auto offset = static_cast<long>(pointerTable[pointerValue[1]].offset);

				fseek(f, offset, 0);

				if (value == nullptr)
					value = std::make_shared<TY>();

				pointerTable[pointerValue[1]]._ptr = value;

				_Deserialize(*value);

				fseek(f, temp, 0);
			}
		}

		void MappedValue(uint64_t ID, auto& value)
		{
			if (auto res = pointerTable.find(ID); res != pointerTable.end())
			{
				const auto offset			= res->second.offset;
				const auto currentOffset	= ftell(f);

				fseek(f, (long)offset, 0);

				(*this)& value;

				fseek(f, currentOffset, 0);
			}
		}

		template<typename TY>
		void operator & (TY& rhs)
		{
			if constexpr (is_serializable_ptr<TY>)
			{
				DeserializePointer(rhs);
			}
			else
				DeserializeValue(rhs);
		}


		void operator & (RawBuffer&& rhs)
		{
			Read(&rhs.size, sizeof(rhs.size));

			if (!rhs.size)
				return;

			if(rhs.size >= 4096)
			{
				size_t tempBufferSize;
				Read(&tempBufferSize, sizeof(tempBufferSize));

				rhs._ptr = malloc(rhs.size);
				void* temp = malloc(tempBufferSize);

				Read((void*)temp, tempBufferSize);
				auto decompressed = DecompressBuffer(RawBuffer{ ._ptr = temp, .size = tempBufferSize }, rhs.size);

				rhs._ptr = decompressed.value_or(nullptr);
				rhs.size = decompressed ? rhs.size : 0;
			}
			else
			{
				rhs._ptr = malloc(rhs.size);

				Read(rhs._ptr, rhs.size);
			}
		}

	private:
		struct PointerMapping
		{
			uint64_t    offset  = 0;
			std::any    _ptr;
		};

		std::map<uint64_t, PointerMapping> pointerTable;

		size_t offset   = 16;
		FILE* f         = nullptr;
	};


	/************************************************************************************************/


	class LoadBlobArchiveContext
	{
		void Read(void* dest, size_t size) noexcept
		{
			memcpy(dest, blob.data() + offset, size);
			offset += size;
		}

		void Seek(size_t pos) noexcept
		{
			offset = pos;
		}

		size_t Tell() const noexcept
		{
			return offset;
		}
	public:


		LoadBlobArchiveContext(Blob& IN_blob) : blob { IN_blob } 
		{
			PointerTableHeader pointerHeader;

			Read(&pointerHeader, sizeof(pointerHeader));

			for (size_t I = 0; I < pointerHeader.size; I++)
			{
				PointerValue e;
				Read(&e, sizeof(e));

				pointerTable[e.ID] = { e.offset };
			}
		}


		~LoadBlobArchiveContext() = default;


		LoadBlobArchiveContext              (const LoadBlobArchiveContext&) = delete;
		LoadBlobArchiveContext& operator =  (const LoadFileArchiveContext&) = delete;

		LoadBlobArchiveContext              (LoadBlobArchiveContext&&) = delete;
		LoadBlobArchiveContext& operator =  (LoadBlobArchiveContext&&) = delete;

		static consteval bool Loading() noexcept { return true; }

		template<typename TY>
		requires TrivialCopy<TY, LoadBlobArchiveContext>
		void _Deserialize(TY& value)
		{
			Read(&value, sizeof(TY));
		}

		template<typename ... TY_Types>
		void _Deserialize(std::variant<TY_Types...>& variant)
		{
			size_t i = -1;
			Read(&i, sizeof(i));

			if (i != std::variant_npos)
				VariantDeserializationHelper<TY_Types...>::Construct(
					*this, variant, i);
		}

		template<typename TY>
		requires(SerializableValue<TY, LoadBlobArchiveContext>)
		void _Deserialize(TY& value)
		{
			Serialize(*this, value);
		}


		void _Deserialize(std::string& value)
		{
			StringHeader header;
			Read(&header, sizeof(header));

			value.resize(header.stringSize);

			Read(value.data(), header.stringSize);
		}


		void _Deserialize(SerializableInterfacePointer auto& value)
		{
			value._Serialize(*this);
		}


		template<typename TY>
		requires(SerializableStruct<TY, LoadBlobArchiveContext> && !SerializableInterfacePointer<TY>)
		void _Deserialize(TY& value)
		{
			value.Serialize(*this);
		}


		template<typename TY_Key, typename TY_Value>
		void _Deserialize(std::map<TY_Key, TY_Value>& value_map)
		{
			using namespace SerializeableUtilities;

			MapHeader header{};
			Read(&header, sizeof(header));

			for (size_t I = 0; I < header.size; I++)
			{
				TY_Key      key;
				TY_Value    value;

				_Deserialize(key);
				_Deserialize(value);

				value_map[key] = std::move(value);
			}
		}


		template<typename TY>
		void DeserializeValue(TY& value)
		{
			using namespace SerializeableUtilities;

			_Deserialize(value);
		}


		template<typename TY>
		void DeserializeValue(std::vector<TY>& vector)
		{
			const auto& typeID = typeid(TY).hash_code();

			auto temp = Tell();

			VectorHeader header = { 0, 0 };
			Read(&header, sizeof(header));

			assert(typeID == header.type);

			vector.reserve(header.size);

			for (size_t I = 0; I < header.size; I++)
			{
				TY value;

				if constexpr (is_serializable_ptr<TY>)
				{
					if (value == nullptr)
					{
						if constexpr (is_serializable_ptr<TY>)
							value = std::make_shared<typename SerializeableUtilities::is_serializable_ptr<TY>::type>();
					}
					DeserializePointer(value);
				}
				else
					DeserializeValue(value);

				vector.emplace_back(std::move(value));
			}
		}


		template<template<typename ... > typename Ptr_Ty, typename TY>
		void DeserializePointer(std::shared_ptr<TY>& value) requires std::is_base_of_v<FlexKit::SerializableInterfaceBase, TY> && is_serializable_ptr<Ptr_Ty<TY>>
		{
			using namespace SerializeableUtilities;

			PointerHeader header;
			Read(&header, sizeof(header));

			if (header.pointerID == 0)
				return;

			if (pointerTable[header.pointerID]._ptr.has_value())
			{
				value = std::any_cast<std::shared_ptr<TY>>(pointerTable[header.pointerID]._ptr);
			}
			else
			{
				auto temp   = Tell();
				auto offset = static_cast<long>(pointerTable[header.pointerID].offset);

				Seek(offset);

				TypeID_t typeID;
				Read(&typeID, sizeof(typeID));

				assert(typeID == header.typeID);

				auto newValue = FlexKit::SerializableBase::Construct(typeID);
				newValue->_Serialize(*this);

				value = std::shared_ptr<TY>(dynamic_cast<TY*>(newValue));

				pointerTable[header.pointerID]._ptr = value;

				Seek(temp);
			}
		}


		template<template<typename ... > typename Ptr_Ty, typename TY>
		void DeserializePointer(Ptr_Ty<TY>& value) requires is_serializable_ptr<Ptr_Ty<TY>>
		{
			using namespace SerializeableUtilities;

			uint64_t pointerValue[2];
			Read(pointerValue, sizeof(pointerValue));

			if (pointerTable[pointerValue[1]]._ptr.has_value())
			{
				auto& any_ptr	= pointerTable[pointerValue[1]]._ptr;

				value = std::any_cast<Ptr_Ty<TY>>(any_ptr);
			}
			else if(pointerValue[1] != 0)
			{
				assert(pointerTable.find(pointerValue[1]) != pointerTable.end());

				auto temp   = Tell();
				auto offset = static_cast<long>(pointerTable[pointerValue[1]].offset);

				Seek(offset);

				if (value == nullptr)
					value = Ptr_Ty<TY>{ new TY };

				pointerTable[pointerValue[1]]._ptr = value;

				_Deserialize(*value);

				Seek(temp);
			}
		}

		void MappedValue(uint64_t ID, auto& value)
		{
			if (auto res = pointerTable.find(ID); res != pointerTable.end())
			{
				const auto offset			= res->second.offset;
				const auto currentOffset	= Tell();

				Seek(offset);

				(*this)& value;

				Seek(currentOffset);
			}
		}

		template<typename TY>
		void operator & (TY& rhs)
		{
			if constexpr (is_serializable_ptr<TY>)
			{
				DeserializePointer(rhs);
			}
			else
				DeserializeValue(rhs);
		}


		void operator & (RawBuffer&& rhs)
		{
			Read(&rhs.size, sizeof(rhs.size));

			if (!rhs.size)
				return;

			if(rhs.size >= 4096)
			{
				size_t tempBufferSize;
				Read(&tempBufferSize, sizeof(tempBufferSize));

				rhs._ptr = malloc(rhs.size);
				void* temp = malloc(tempBufferSize);

				Read((void*)temp, tempBufferSize);
				auto decompressed = DecompressBuffer(RawBuffer{ ._ptr = temp, .size = tempBufferSize }, rhs.size);

				rhs._ptr = decompressed.value_or(nullptr);
				rhs.size = decompressed ? rhs.size : 0;
			}
			else
			{
				rhs._ptr = malloc(rhs.size);

				Read(rhs._ptr, rhs.size);
			}
		}

	private:
		struct PointerMapping
		{
			uint64_t    offset  = 0;
			std::any    _ptr;
		};

		std::map<uint64_t, PointerMapping> pointerTable;

		size_t offset = 0;
		Blob& blob;
	};


	template<class Archive, typename T, size_t Size>
	void Serialize(Archive& ar, FlexKit::static_vector<T, Size>& vector)
	{
		if (ar.Loading())
		{
			size_t S = 0;
			ar& S;
			vector.resize(S);
		}
		else
		{
			size_t S = vector.size();
			ar& S;
		}

		for (auto& element : vector)
			ar& element;
	}


	/************************************************************************************************/


	struct ValueStore
	{
		struct ValueMapping
		{
			using Deserializer	= std::function<void(ValueMapping&, LoadFileArchiveContext&)>;
			using Deserializer2	= std::function<void(ValueMapping&, LoadBlobArchiveContext&)>;
			using Serializer	= std::function<void(ValueMapping&, SaveArchiveContext&)>;

			uint64_t							id;
			uint64_t							blobId;

			Deserializer	deserializer;
			Deserializer2	deserializer2;
			Serializer		serializer;

			void Serialize(auto& ar)
			{
				ar& id;
				ar& blobId;
			}
		};

		void Bind(std::string_view id, auto& rhs)
		{
			const auto idHash		= std::hash<std::string_view>{}(id);
			const uint64_t blobId	= rand();

			auto deserializer = [&, idHash, id](ValueMapping& value, LoadFileArchiveContext& ctx)
			{
				ctx.MappedValue(value.blobId, rhs);
			};

			auto deserializer2 = [&, idHash, id](ValueMapping& value, LoadBlobArchiveContext& ctx)
			{
				ctx.MappedValue(value.blobId, rhs);
			};

			auto serializer = [&](ValueMapping& value, SaveArchiveContext& ctx)
			{
				ctx.MappedValue(value.blobId, rhs);
			};

			ValueMapping value;
			value.id			= idHash;
			value.blobId		= blobId;
			value.deserializer = deserializer;
			value.deserializer2	= deserializer2;
			value.serializer	= serializer;

			mappings.push_back(value);
		}

		void Serialize(auto& ar)
		{
			if constexpr(ar.Loading())
			{
				std::vector<ValueMapping> fileMappings;
				ar& fileMappings;

				for (auto& mapping : mappings)
				{
					if (auto res = std::ranges::find_if(fileMappings,
						[&](ValueMapping& rhs) { return rhs.id == mapping.id; }); res != fileMappings.end())
					{
						if constexpr( std::is_same_v<decltype(ar), LoadFileArchiveContext>)
							mapping.deserializer(*res, ar);
						if constexpr (std::is_same_v<decltype(ar), LoadBlobArchiveContext>)
							mapping.deserializer2(*res, ar);
					}
				}
			}
			else
			{
				ar& mappings;

				for (auto& mapping : mappings)
					mapping.serializer(mapping, ar);
			}
		}

		std::vector<ValueMapping> mappings;
	};


#define BindValue(valueStore, value) valueStore.Bind(#value, value)

}   /************************************************************************************************/
