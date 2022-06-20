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
#include <static_vector.h>

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


    inline void WriteBlob(Blob& b, FILE* f)
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
        struct is_smart_ptr
        {
            constexpr operator bool() { return false; }
            static constexpr bool value = false;

            using type = void;
        };

        template<typename TY>
        struct is_smart_ptr<std::shared_ptr<TY>>
        {
            constexpr operator bool() { return true; }
            static constexpr bool value = true;

            using type = TY;
        };

        template<typename TY>
        struct is_smart_ptr<std::weak_ptr<TY>>
        {
            constexpr operator bool() { return true; }
            static constexpr bool value = true;

            using type = TY;
        };
    }


    template<typename TY>
    constexpr bool is_Pointer = SerializeableUtilities::is_smart_ptr<TY>::value;


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
    requires is_Pointer<TY> && std::is_base_of_v<SerializableBase, typename SerializeableUtilities::is_smart_ptr<TY>::type>
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

        constexpr bool Loading() const noexcept { return false; }

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
            TypeID_t temp = GetTypeID(value);
            dataBuffer.back() += { temp };

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


        void _Serialize(std::string string)
        {
            dataBuffer.back() += StringHeader{ string.size() };
            dataBuffer.back() += Blob{ string.data(), string.size() };
        }


        template<typename TY>
        void SerializePointer(TY& value)
        {
            using namespace SerializeableUtilities;

            if constexpr (is_smart_ptr<TY>())
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
                    dataBuffer.back() += PointerHeader{ PointerHeader{ 0, (uint64_t)nullptr }};
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

                dataBuffer.back() += Blob{ PointerHeader{ PointerHeader{ typeID, (uint64_t)value } } };
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
            if(auto compressed = CompressBuffer(rhs); compressed)
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
            fseek(f, pos, 0);
        }

        size_t Tell() const noexcept
        {
            return ftell(f);
        }

        constexpr bool Loading() const noexcept { return true; }

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
            value._Serialize(*this);
        }


        template<typename TY>
        requires(SerializableStruct<TY, LoadFileArchiveContext>)
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
            using namespace SerializeableUtilities;

            const auto& typeID = typeid(TY).hash_code();

            auto temp = ftell(f);

            VectorHeader header = { 0, 0 };
            auto res = fread(&header, 1, sizeof(header), f);

            if (res != sizeof(header))
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

            vector.reserve(header.size);

            for (size_t I = 0; I < header.size; I++)
            {
                TY value;

                if constexpr (is_pointer<TY>())
                {
                    if (value == nullptr)
                    {
                        if constexpr (SerializeableUtilities::is_smart_ptr<TY>())
                            value = std::make_shared<typename SerializeableUtilities::is_smart_ptr<TY>::type>();
                    }
                    DeserializePointer(value);
                }
                else
                    DeserializeValue(value);

                vector.emplace_back(std::move(value));
            }
        }


        template<typename TY>
        requires(std::is_base_of_v<FlexKit::SerializableInterfaceBase, TY>)
        void DeserializePointer(std::shared_ptr<TY>& value)
        {
            using namespace SerializeableUtilities;

            PointerHeader header;
            fread(&header, 1, sizeof(header), f);

            if (header.pointerID == 0)
                return;

            if (pointerTable[header.pointerID]._ptr.has_value())
            {
                value = std::any_cast<std::shared_ptr<TY>>(pointerTable[header.pointerID]._ptr);
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

                value = std::shared_ptr<TY>(dynamic_cast<TY*>(newValue));

                pointerTable[header.pointerID]._ptr = value;

                fseek(f, temp, 0);
            }
        }


        template<typename TY>
        void DeserializePointer(std::shared_ptr<TY>& value)
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


        template<typename TY>
        void operator & (TY& rhs)
        {
            using namespace SerializeableUtilities;

            if constexpr (is_pointer<TY>())
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

            size_t tempBufferSize;
            Read(&tempBufferSize, sizeof(tempBufferSize));

            rhs._ptr = malloc(rhs.size);
            void* temp = malloc(tempBufferSize);

            Read((void*)temp, tempBufferSize);
            auto decompressed = DecompressBuffer(RawBuffer{ ._ptr = temp, .size = tempBufferSize }, rhs.size);

            rhs._ptr = decompressed.value_or(nullptr);
            rhs.size = decompressed ? rhs.size : 0;
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
    public:


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

        constexpr bool Loading() const noexcept { return true; }

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
            using namespace SerializeableUtilities;

            const auto& typeID = typeid(TY).hash_code();

            auto temp = Tell();

            VectorHeader header = { 0, 0 };
            Read(&header, sizeof(header));

            assert(typeID == header.type);

            vector.reserve(header.size);

            for (size_t I = 0; I < header.size; I++)
            {
                TY value;

                if constexpr (is_pointer<TY>())
                {
                    if (value == nullptr)
                    {
                        if constexpr (SerializeableUtilities::is_smart_ptr<TY>())
                            value = std::make_shared<typename SerializeableUtilities::is_smart_ptr<TY>::type>();
                    }
                    DeserializePointer(value);
                }
                else
                    DeserializeValue(value);

                vector.emplace_back(std::move(value));
            }
        }


        template<typename TY>
        requires std::is_base_of_v<FlexKit::SerializableInterfaceBase, TY>
        void DeserializePointer(std::shared_ptr<TY>& value)
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


        template<typename TY>
        void DeserializePointer(std::shared_ptr<TY>& value)
        {
            using namespace SerializeableUtilities;

            uint64_t pointerValue[2];
            Read(pointerValue, sizeof(pointerValue));

            if (pointerTable[pointerValue[1]]._ptr.has_value())
            {
                auto& any_ptr = pointerTable[pointerValue[1]]._ptr;
                value = std::any_cast<std::shared_ptr<TY>>(any_ptr);
            }
            else if(pointerValue[1] != 0)
            {
                assert(pointerTable.find(pointerValue[1]) != pointerTable.end());

                auto temp   = Tell();
                auto offset = static_cast<long>(pointerTable[pointerValue[1]].offset);

                Seek(offset);

                if (value == nullptr)
                    value = std::make_shared<TY>();

                pointerTable[pointerValue[1]]._ptr = value;

                _Deserialize(*value);

                Seek(temp);
            }
        }


        template<typename TY>
        void operator & (TY& rhs)
        {
            using namespace SerializeableUtilities;

            if constexpr (is_pointer<TY>())
            {
                DeserializePointer(rhs);
            }
            else
                DeserializeValue(rhs);
        }


        void operator & (RawBuffer&& rhs)
        {
            Read(&rhs.size,         sizeof(rhs.size));

            if (!rhs.size)
                return;

            size_t tempBufferSize;
            Read(&tempBufferSize, sizeof(tempBufferSize));

            rhs._ptr = malloc(rhs.size);
            void*    temp = malloc(tempBufferSize);

            Read((void*)temp, tempBufferSize);
            auto decompressed = DecompressBuffer(RawBuffer{ ._ptr = temp, .size = tempBufferSize }, rhs.size);

            rhs._ptr = decompressed.value_or(nullptr);
            rhs.size = decompressed ? rhs.size : 0;
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


}   /************************************************************************************************/
