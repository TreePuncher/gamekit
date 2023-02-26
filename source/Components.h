#pragma once

#include "buildsettings.h"
#include "containers.h"
#include "Handle.h"
#include "logging.h"
#include "MathUtils.h"
#include "MemoryUtilities.h"
#include "RuntimeComponentIDs.h"
#include "ThreadUtilities.h"
#include "type.h"

#include <iostream>
#include <type_traits>
#include <tuple>
#include <span>
#include <regex>

namespace FlexKit
{	/************************************************************************************************/


	constexpr ComponentID InvalidComponentID = -1;

	class GameObject;

	using KeyValuePair	= std::pair<uint32_t, void*>;
	using ValueMap		= std::span<KeyValuePair>;

	template<typename TY>
	std::optional<TY> FindValue(ValueMap& map, uint32_t valueId)
	{
		auto res = std::find_if(
					map.begin(),
					map.end(),
					[&](auto& pair) {
						return std::get<0>(pair) == valueId; }
					);

		if (res != map.end())
			return { (TY)res->second };
		else
			return {};
	}

	class ComponentBase
	{
	protected:

		struct ComponentEntry
		{
			ComponentBase*	component;
			ComponentID		ID;
		};

		virtual ComponentID	GetID() { return -1; }

		ComponentBase() {}
		// no moving
		ComponentBase& operator =	(ComponentBase&& rhs)		= delete; 
		ComponentBase				(const ComponentBase&&)		= delete;

		// no copying
		ComponentBase& operator =	(ComponentBase&	rhs)		= delete; 
		ComponentBase				(const ComponentBase&)		= delete;


		static void AddComponent(ComponentBase& component)
		{
			const auto ID = component.GetID();
			auto res = find(Components, [&](auto value)
				{
					return value.ID == ID;
				});

			if (res != Components.end()) {
				FK_LOG_WARNING("Component System Adding already added System!");
				FK_ASSERT(0, "Critical Error!");
			}

			Components.push_back({ &component, component.GetID() });
		}


		static void RemoveComponent(ComponentBase& system) noexcept
		{
			const auto ID = system.GetID();
			auto res = find(Components, [&](auto value)
				{
					return value.ID == ID;
				});

			if (res != Components.end())
			{
				*res = Components.back();
				Components.pop_back();
			}
			else
				FK_LOG_WARNING("Component System Releasing Already Released System!");
		}

	public:
		static ComponentBase& GetComponent(ComponentID ID)
		{
			auto res = find(
				Components,
				[&](auto e)
				{
					return e.ID == ID;
				});

			if (res == Components.end())
			{
				FK_LOG_ERROR("Component System Not Initialized!");
				throw(std::runtime_error("Component System Not Initialized!"));
			}

			return *(*res).component;
		}


		static bool isComponentAvailable(ComponentID ID)
		{
			auto res = find(
				Components,
				[&](auto e)
				{
					return e.ID == ID;
				});


			return res != Components.end();
		}


		static static_vector<ComponentEntry, 128> GetComponentList()
		{
			return Components;
		}


		virtual void AddComponentView(GameObject& GO, ValueMap user_ptr, const std::byte* buffer, const size_t bufferSize, iAllocator* allocator) {};


		inline static static_vector<ComponentEntry, 128> Components = static_vector<ComponentBase::ComponentEntry, 128>();
	};


	inline bool ComponentAvailability(ComponentID ID)
	{
		return ComponentBase::isComponentAvailable(ID);
	}


	inline ComponentBase& GetComponent(ComponentID ID)
	{
		return ComponentBase::GetComponent(ID);
	}


	/************************************************************************************************/


	template<typename TY, ComponentID ID = -1>
	class Component : public ComponentBase
	{
	public:
	protected:
		inline static TY* component	= nullptr;


		Component() 
		{
			FK_ASSERT(component != nullptr, "Component System error: double creation detected!");

			component = static_cast<TY*>(this);
			ComponentBase::AddComponent(*static_cast<ComponentBase*>(this));
		}


		~Component()
		{
			ComponentBase::RemoveComponent(*static_cast<ComponentBase*>(this));
			component = nullptr;
		}

	public:

		ComponentID GetID() override					{ return ID; }

		constexpr static ComponentID GetComponentID()	{ return ID; }

		static TY& GetComponent()
		{
#if _DEBUG
			if (!component)
				throw(std::runtime_error("Component::GetSystem() : ERROR : Component not initialized!"));
#endif

			return *static_cast<TY*>(component);
		}

		static bool isAvailable()
		{
			return component != nullptr;
		}

		virtual void AddComponentView(GameObject& GO, ValueMap user_ptr, const std::byte* buffer, const size_t bufferSize, iAllocator* allocator) override {}
	};


	/************************************************************************************************/
	

	class ComponentViewBase
	{
	public:
		ComponentViewBase(ComponentID IN_ID) :
			ID{ IN_ID } {}

		virtual ~ComponentViewBase() {}

		ComponentBase& GetComponentRef()	{ return ComponentBase::GetComponent(ID); }

		ComponentID ID;
	};


	template<typename ComponentTY>
	class ComponentView_t : public ComponentViewBase
	{
	public:
		ComponentView_t() : ComponentViewBase{ ComponentTY::GetComponentID() } {}
		virtual ~ComponentView_t() override {}

		static ComponentID		GetComponentID()	{ return ComponentTY::GetComponentID(); }
		static decltype(auto)	GetComponent()		{ return ComponentTY::GetComponent(); }
	};


	/************************************************************************************************/


	struct ComponentViewContainer
	{
		ComponentViewContainer() = default;

		template<typename TY, typename... TY_ARGS>
		void Create(iAllocator& allocator, GameObject& gameObject, TY_ARGS&& ... args)
		{
			componentSize = (uint8_t)sizeof(TY);

			if (componentSize > sizeof(buffer))
				_ptr = &allocator.allocate<TY>(gameObject, std::forward<TY_ARGS>(args)...);
			else
				new(buffer) TY(gameObject, std::forward<TY_ARGS>(args)...);
		}

		ComponentViewBase* operator -> () { return Get(); }
		operator ComponentViewBase* () { return Get(); }

		ComponentViewBase* Get() const
		{
			if (componentSize > sizeof(buffer))
				return _ptr;
			else
				return (ComponentViewBase*)(&buffer);
		}

		ComponentID GetID() const
		{
			return Get()->ID;
		}


		ComponentViewBase& Get_ref() const
		{
			return *Get();
		}

		void Release(iAllocator* allocator)
		{
			if (componentSize == 0)
				return;

			Get()->~ComponentViewBase();

			if (componentSize > componentSize)
				allocator->release(_ptr);
		}

		uint8_t		componentSize = 0;
		union
		{
			ComponentViewBase*	_ptr;
			char				buffer[24];
		};
	};


	class GameObject
	{
	public:
		GameObject(iAllocator* IN_allocator = SystemAllocator) :
			allocator	{ IN_allocator },
			ids			{ IN_allocator },
			views		{ IN_allocator } {}


		~GameObject()
		{
			Release();
		}


		GameObject				(const GameObject& rhs) = delete;
		GameObject& operator =	(const GameObject& rhs) = delete;


		GameObject				(GameObject&& rhs) = delete;
		GameObject& operator =	(GameObject&& rhs) = delete;

		template<typename TY_View, typename ... TY_args>
		auto& AddView(TY_args&& ... args)
		{
			static_assert(std::is_base_of<ComponentViewBase, TY_View>(), "You can only add view types!");

			if (auto res = GetView(TY_View::GetComponentID()); res)
				return *static_cast<TY_View*>(res);
			else
			{
				views.emplace_back();
				ids.push_back(TY_View::GetComponentID());
				auto& container = views.back();
				container.Create<TY_View>(*allocator, *this, std::forward<TY_args>(args)...);

				return *static_cast<TY_View*>(container.Get());
			}
		}


		void RemoveView(ComponentViewBase& view)
		{
			auto const id = view.ID;
			for (size_t I = 0; I < ids.size(); I++)
			{
				if (ids[I] == id) {
					views[I].Release(allocator);
					views.remove_unstable(views.begin() + I);
					ids.remove_unstable(ids.begin() + I);
				}
			}
		}


		void RemoveView(ComponentViewBase* view)
		{
			if (view)
				RemoveView(*view);
		}


		void Release()
		{
			for (auto& view : views) {
				view.Release(allocator);
			}

			views.clear();
			ids.clear();
		}


		void _Clear()
		{
			views.clear();
			ids.clear();
		}


		ComponentViewBase* GetView(ComponentID id) const
		{
			const size_t end = ids.size();
			for (size_t I = 0; I < end; I++)
				if (ids[I] == id)
					return views[I].Get();

			return nullptr;
		}

		template<typename TY_View>
		TY_View* GetView()
		{
			if (auto res = GetView(TY_View::GetComponentID()); res)
				return static_cast<TY_View*>(res);
			else
				return nullptr;
		}

		bool hasView(ComponentID id) const
		{
			for (auto& componentId : ids)
				if (componentId == id)
					return true;

			return false;
		}


		auto begin()	{ return views.begin(); }
		auto end()		{ return views.end(); }

		auto begin()	const { return views.begin(); }
		auto end()		const { return views.end(); }

	private:
		Vector<uint32_t, 8, uint8_t>					ids;	// component + Code
		Vector<ComponentViewContainer, 8, uint8_t>		views;	// component + Code
		iAllocator*										allocator;
};


	/************************************************************************************************/


	bool LoadPrefab(GameObject&, const char* assetID,   iAllocator& allocator, ValueMap user = {});
	bool LoadPrefab(GameObject&, uint64_t assetID,      iAllocator& allocator, ValueMap user = {});


	/************************************************************************************************/


	template<typename TY_COMPONENT>
	constexpr bool ValidType()
	{
		return std::is_base_of_v<ComponentViewBase, std::remove_pointer_t<std::decay_t<TY_COMPONENT>>>;
	}


	template<typename ... ARGS>
	constexpr bool ValidTypes()
	{
		return (ValidType<ARGS>() || ...);
	}


	template<typename ... TY_PACKED_ARGS>
	bool hasViews(const GameObject& go)
	{
		static_assert(ValidTypes<TY_PACKED_ARGS...>(), "Invalid Type Detected, Use only ComponentView types!");

		return (go.hasView(std::decay_t<TY_PACKED_ARGS>::GetComponentID()) & ...);
	}


	template<typename TY_COMPONENT>
	auto& GetView(const GameObject& go)
	{
		static_assert(std::is_base_of<ComponentViewBase, TY_COMPONENT>::value, "Parameter that is not a behavior type detected, behavior types only!");

		return *static_cast<TY_COMPONENT*>(go.GetView(TY_COMPONENT::GetComponentID()));
	}


	template<typename TY_COMPONENT>
	auto GetViewTuple(const GameObject& go)
	{
		using TY_COMPONENT_DECAYED  = typename std::remove_pointer_t<std::decay_t<TY_COMPONENT>>;
		constexpr bool pointer      = std::is_pointer_v<TY_COMPONENT>;

		static_assert(std::is_base_of<ComponentViewBase, TY_COMPONENT_DECAYED>::value, "Parameter that is not a behavior type detected, behavior types only!");

		if constexpr (pointer)
			return  std::tuple<TY_COMPONENT_DECAYED*>{ &GetView<TY_COMPONENT_DECAYED>(go) };
		else
			return  std::tuple<TY_COMPONENT_DECAYED&>{  GetView<TY_COMPONENT_DECAYED>(go) };
	}


	template<typename TY_COMPONENT_ARG, typename ... TY_PACKED_ARGS>
	auto GetViewsTuple(const GameObject& go)
	{
		if constexpr (sizeof...(TY_PACKED_ARGS) == 0)
			return  GetViewTuple<TY_COMPONENT_ARG>(go);
		else
			return  std::tuple_cat(
				GetViewTuple<TY_COMPONENT_ARG>(go),
				GetViewsTuple<TY_PACKED_ARGS...>(go));
	}


	template<typename ... TY_PACKED_ARGS, typename FN, typename ErrorFN>
	auto Apply_t(const GameObject& go, const FN& fn, const ErrorFN& errorFn)
	{
		if (!hasViews<TY_PACKED_ARGS...>(go))
			return errorFn();
		else 
			return std::apply(fn, GetViewsTuple<TY_PACKED_ARGS...>(go));
	}

	// Thank Columbo for this idea, https://stackoverflow.com/a/28213747

	template <typename T>
	struct ApplyProxy : ApplyProxy<decltype(&T::operator())> {};

	#define REM_CTOR(...) __VA_ARGS__
	#define SPEC(cv, var, is_var)														\
	template <typename C, typename R, typename... Args>									\
	struct ApplyProxy<R (C::*) (Args... REM_CTOR var) cv>								\
	{																					\
		template<typename FN, typename ErrorFN>	static auto run(GameObject& go, const FN& fn, ErrorFN errorFn)						\
		{																				\
			return Apply_t<Args...>(go, fn, errorFn);									\
		}																				\
		template<typename FN, typename ErrorFN>	static auto run(const GameObject& go, const FN& fn, ErrorFN errorFn)						\
		{																				\
			return Apply_t<Args...>(go, fn, errorFn);									\
		}																				\
	};																					\

	SPEC(const, (,...), 1)
	SPEC(const, (), 0)
	SPEC(, (,...), 1)
	SPEC(, (), 0)


	/************************************************************************************************/


	template<typename FN, typename ErrorFN>
	auto Apply(GameObject& go, FN fn, ErrorFN errorFn)
	{
		return ApplyProxy<FN>::run(go, fn, errorFn);
	}


	template<typename FN>
	auto Apply(GameObject& go, FN fn)
	{
		return ApplyProxy<FN>::run(go, fn, [&] {});
	}

	template<typename FN>
	void Apply(GameObject** itr, GameObject** end, FN fn)
	{
		while (itr != end)
		{
			ApplyProxy<FN>::run(**itr, fn, [&] {});
			itr++;
		}
	}


	/************************************************************************************************/


	template<typename TY>
	struct ReadOnly
	{
		using Type          = const TY&;
		using ValueType     = TY;

		static constexpr bool IsConst() { return true; }
		bool IsValid(auto&) { return true; }
	};

	template<typename TY>
	struct Mut
	{
		using Type      = TY&;
		using ValueType = TY;

		static constexpr bool IsConst() { return false; }
		bool IsValid(auto&) { return true; }
	};

	template<typename TY>
	bool Filter(const GameObject& go, TY& ty)
	{
		return (ty.Available(go));
	}


	template<typename TY_GO, typename ... TY_ARGS>
	auto Query(TY_GO& go, TY_ARGS ... requests) //requires( std::is_same_v<std::decay_t<TY_GO>, GameObject> )
	{
		if constexpr (std::is_const_v<TY_GO>)
			static_assert((TY_ARGS::IsConst() && ...), "All queries must be read only!");

		bool available  = (Filter(go, requests) && ...);
		using Tuple_TY = std::tuple<typename TY_ARGS::Type...>;

		auto _getValue = [&](auto& value) -> decltype(auto) { return value.GetValue(go); };

		if (available)
			return std::optional{ Tuple_TY{ _getValue(requests) ... } };
		else
			return std::optional<Tuple_TY>{};
	}


	/************************************************************************************************/


	template<typename TY_Component>
	class BasicComponentView_t : public ComponentView_t<TY_Component>
	{
	public:
		using ComponentHandle_t = typename Handle_t<32, TY_Component::GetComponentID()>;

		BasicComponentView_t(GameObject& gameObject, ComponentHandle_t IN_handle) : handle{ IN_handle } {}


		template<typename ... TY_Args>
		BasicComponentView_t(GameObject& gameObject, TY_Args ... args) : handle{ ComponentView_t<TY_Component>::GetComponent().Create(std::forward<TY_Args>(args)...) } {}

		virtual ~BasicComponentView_t() final
		{
			TY_Component::GetComponent().Remove(handle);
		}


		// No moving
		BasicComponentView_t(const BasicComponentView_t&)               = delete;
		BasicComponentView_t& operator = (const BasicComponentView_t&)  = delete;

		BasicComponentView_t(BasicComponentView_t&&)                = delete;
		BasicComponentView_t& operator = (BasicComponentView_t&&)   = delete;


		decltype(auto) operator -> ()
		{
			return &GetData();
		}

		decltype(auto) GetData()
		{
			return ComponentView_t<TY_Component>::GetComponent()[handle];
		}

		ComponentHandle_t handle;
	};


	/************************************************************************************************/


	struct BasicComponentEventHandler
	{
		static decltype(auto) OnCreate(auto&& args)
		{
			return args;
		}

		static void OnCreateView(GameObject& gameObject, ValueMap user_ptr, const std::byte* buffer, const size_t bufferSize, iAllocator* allocator)
		{
		}
	};


	template<typename TY>
	concept VoidCreator = requires(TY creationHandler)
	{
		creationHandler.OnCreate();
	};


	template<typename TY, typename TY_Handle, ComponentID ID, typename TY_EventHandler = BasicComponentEventHandler>
	class BasicComponent_t : public Component<BasicComponent_t<TY, TY_Handle, ID, TY_EventHandler>, ID>
	{
	public:
		using ThisType		= BasicComponent_t<TY, TY_Handle, ID, TY_EventHandler>;
		using EventHandler	= TY_EventHandler;

		template<typename ... TY_args>
		BasicComponent_t(iAllocator* allocator, TY_args&&... args) :
			eventHandler	{ std::forward<TY_args>(args)... },
			elements		{  allocator },
			handles			{  allocator } {}

		BasicComponent_t(iAllocator* allocator) :
			elements		{  allocator },
			handles			{  allocator } {}

		~BasicComponent_t()
		{
			elements.Release();
		}

		struct elementData
		{
			TY_Handle		handle;
			TY				componentData;
		};

		using View = BasicComponentView_t<BasicComponent_t<TY, TY_Handle, ID, TY_EventHandler>>;


		TY_Handle Create(const TY& initial)
		{
			auto handle		= handles.GetNewHandle();
			handles[handle] = (index_t)elements.push_back({ handle, eventHandler.OnCreate(initial) });

			return handle;
		}


		TY_Handle Create(TY&& initial)
		{
			auto handle = handles.GetNewHandle();
			handles[handle] = (index_t)elements.emplace_back(handle, std::move(eventHandler.OnCreate(initial)));

			return handle;
		}


		TY_Handle Create() requires VoidCreator<TY_EventHandler>
		{
			auto handle = handles.GetNewHandle();
			handles[handle] = (index_t)elements.emplace_back(handle, std::move(eventHandler.OnCreate()));

			return handle;
		}


		TY_Handle Create() requires !VoidCreator<TY_EventHandler>
		{
			auto handle = handles.GetNewHandle();
			handles[handle] = (index_t)elements.emplace_back(handle);

			return handle;
		}


		void AddComponentView(GameObject& GO, ValueMap values, const std::byte* buffer, const size_t bufferSize, iAllocator* allocator) override
		{
			eventHandler.OnCreateView(GO, values, buffer, bufferSize, allocator);
		}


		void Remove(TY_Handle handle)
		{
			auto lastElement			= std::move(elements.back());
			elements[handles[handle]]	= std::move(lastElement);
			elements.pop_back();

			handles[lastElement.handle] = handles[handle];
			handles.RemoveHandle(handle);
		}


		Vector<TY> GetElements_copy(iAllocator* tempMemory) const
		{
			Vector<TY>	out{ tempMemory };

			for (auto& I : elements)
				out.push_back({ I.componentData });

			return out;
		}


		TY& operator[] (TY_Handle handle)
		{
			return elements[handles[handle]].componentData;
		}

		TY operator[] (TY_Handle handle) const
		{
			return elements[handles[handle]].componentData;
		}

		auto begin()
		{
			return elements.begin();
		}

		auto end()
		{
			return elements.end();
		}

		HandleUtilities::HandleTable<TY_Handle>	handles;
		Vector<elementData>						elements;
		TY_EventHandler							eventHandler;
	};


	/************************************************************************************************/


	using StringIDHandle = Handle_t <32, GetTypeGUID(StringID)>;

	class StringIDComponent : public Component<StringIDComponent, StringComponentID>
	{
	public:
		StringIDComponent(iAllocator* allocator) : 
			IDs			{ allocator },
			handles		{ allocator } {}


		struct StringID
		{
			StringIDHandle	handle;
			char			ID[64];
		};


		StringIDHandle Create(const char* initial = nullptr, size_t length = 0);

		void Remove(StringIDHandle handle);

		StringID& operator[] (StringIDHandle handle) { return IDs[handles[handle]]; }

		void AddComponentView(GameObject& GO, ValueMap user_ptr, const std::byte* buffer, const size_t bufferSize, iAllocator* allocator) override;

		HandleUtilities::HandleTable<StringIDHandle>	handles;
		Vector<StringID>								IDs;
	};


	/************************************************************************************************/


	class StringIDView : public ComponentView_t<StringIDComponent>
	{
	public:
		StringIDView(GameObject& gameObject, const char* id, size_t idLen) : ID{ GetComponent().Create(id, idLen) }
		{
			IDHash = std::hash<std::string_view>{}(std::string_view{ id, idLen });
		}

		char* GetString() const
		{
			return GetComponent()[ID].ID;
		}

		void SetString(const char* string)
		{
			IDHash = std::hash<std::string_view>{}(string);
			strncpy_s(GetComponent()[ID].ID, 64, string, 64);
		}

				StringIDComponent::StringID* operator -> ()			{ return &GetComponent()[ID]; }
		const	StringIDComponent::StringID* operator -> () const	{ return &GetComponent()[ID]; }

		StringIDHandle	ID;
		uint64_t		IDHash;
	};


	inline const char* GetStringID(GameObject& gameObject)
	{
		return Apply(gameObject,
			[](StringIDView& ID) -> const char*
			{
				return ID.GetString();
			},
			[]() -> const char*
			{
				return nullptr;
			});
	}


	inline void SetStringID(GameObject& gameObject, const char* str)
	{
		Apply(gameObject,
			[&](StringIDView& ID)
			{
				ID.SetString(str);
			});
	}


	template<typename TY>
	concept IsConstCharStar = ( std::is_same_v<const char*, TY>);

	template<IsConstCharStar ... TY>
	struct StringQuery
	{
		using Type			= StringIDView&;
		using ValueType		= StringIDView;
		static constexpr bool IsConst() { return false; }

		const std::tuple<TY...> IDs;

		StringQuery() = default;

		StringQuery(TY ... args) :
			IDs{ std::make_tuple(args...) } {}

		bool Compare(const StringIDView& stringID, const char* str)
		{
			return strncmp(stringID->ID, str, 64) == 0;
		}

		template<int ... N>
		bool STRCMP_Helper(const StringIDView& stringID, std::integer_sequence<int, N...> integers)
		{
			return (Compare(stringID, std::get<N>(IDs)) | ...);
		}

		bool IsValid(const StringIDView& stringID)
		{
			return STRCMP_Helper(stringID, std::make_integer_sequence<int, sizeof ... (TY)>());
		}

		bool Available(const GameObject& gameObject)
		{
			if (gameObject.hasView(StringIDView::GetComponentID()))
				return IsValid(GetView<StringIDView>(gameObject));
			else return false;
		}

		decltype(auto) GetValue(GameObject& gameObject)
		{
			
			return GetView<StringIDView>(gameObject);// .GetView(StringIDView::GetComponentID());
		};
	};

	template<IsConstCharStar ... TY>
	struct ROStringQuery
	{
		using Type			= const StringIDView&;
		using ValueType		= StringIDView;
		static constexpr bool IsConst() { return true; }

		const std::tuple<TY...> IDs;

		ROStringQuery() = default;

		ROStringQuery(TY ... args) :
			IDs{ std::make_tuple(args...) } {}

		bool Compare(const StringIDView& stringID, const char* str)
		{
			return strncmp(stringID->ID, str, 64) == 0;
		}

		template<int ... N>
		bool STRCMP_Helper(const StringIDView& stringID, std::integer_sequence<int, N...> integers)
		{
			return (Compare(stringID, std::get<N>(IDs)) | ...);
		}

		bool IsValid(const StringIDView& stringID)
		{
			return STRCMP_Helper(stringID, std::make_integer_sequence<int, sizeof ... (TY)>());
		}

		bool Available(const GameObject& gameObject)
		{
			if (gameObject.hasView(StringIDView::GetComponentID()))
				return IsValid(GetView<StringIDView>(gameObject));
			else return false;
		}

		decltype(auto) GetValue(GameObject& gameObject)
		{
			return GetView<StringIDView>(gameObject);
		};
	};

	
	struct StringPatternQuery
	{
		using Type			= StringIDView&;
		using ValueType		= StringIDView;
		static constexpr bool IsConst() { return false; }
		const std::regex& pattern;

		StringPatternQuery() = default;

		StringPatternQuery(const std::regex& IN_pattern) :
			pattern{ IN_pattern } {}

		bool IsValid(const StringIDView& stringID) const noexcept;

		bool Available(const GameObject& gameObject)
		{
			if (gameObject.hasView(StringIDView::GetComponentID()))
				return IsValid(GetView<StringIDView>(gameObject));
			else return false;
		}

		decltype(auto) GetValue(GameObject& gameObject)
		{
			return GetView<StringIDView>(gameObject);
		};
	};


	struct StringHashQuery
	{
		using Type		= StringIDView&;
		using ValueType = StringIDView;
		static constexpr bool IsConst() { return false; }

		uint64_t hash;

		StringHashQuery() = default;

		StringHashQuery(uint64_t IN_hash) :
			hash{ IN_hash } {}

		bool IsValid(const StringIDView& stringID)
		{
			return stringID.IDHash == hash;
		}

		bool Available(const GameObject& gameObject)
		{
			if (gameObject.hasView(StringIDView::GetComponentID()))
				return IsValid(GetView<StringIDView>(gameObject));
			else return false;
		}

		decltype(auto) GetValue(GameObject& gameObject)
		{
			return GetView<StringIDView>(gameObject);// .GetView(StringIDView::GetComponentID());
		};
	};

	/************************************************************************************************/


	struct GameObjectReq
	{
		using Type		= GameObject&;
		using ValueType = GameObject&;
		static constexpr bool IsConst() { return false; }

		bool Available(const GameObject& gameObject)
		{
			return true;
		}

		decltype(auto) GetValue(GameObject& gameObject)
		{
			return gameObject;
		}
	};


	/************************************************************************************************/


	constexpr ComponentID SampleComponentID = GetTypeGUID(SampleComponentID);
	using SampleHandle = Handle_t <32, GetTypeGUID(SampleHandle)>;


	class SampleComponent : public Component<SampleComponent, SampleComponentID>
	{
	public:
		SampleComponent(iAllocator* allocator = SystemAllocator) : entities{ allocator } { }

		SampleHandle CreateComponent()
		{
			return SampleHandle{ static_cast<unsigned int>(entities.push_back({})) };
		}

		void ReleaseEntity(SampleHandle handle)
		{
			// do release
		}

		struct Entity
		{
			int x;
		};

		Vector<Entity> entities;
	};


	constexpr ComponentID SampleComponent2ID = GetTypeGUID(SampleComponent2ID);
	using Sample2Handle = Handle_t <32, GetTypeGUID(Sample2Handle)>;


	class SampleComponent2 : public Component<SampleComponent2, SampleComponent2ID>
	{
	public:
		SampleComponent2(iAllocator* allocator = SystemAllocator) : entities{ allocator } { }

		Sample2Handle CreateComponent()
		{
			return Sample2Handle{ static_cast<unsigned int>(entities.push_back({})) };
		}

		void ReleaseEntity(Sample2Handle handle)
		{
			// do release
		}

		struct Entity
		{
			int x;
		};

		Vector<Entity> entities;
	};

	constexpr ComponentID SampleComponent3ID = GetTypeGUID(SampleComponent3ID);
	using Sample3Handle = Handle_t <32, GetTypeGUID(Sample3Handle)>;


	class SampleComponent3 : public Component<SampleComponent3, SampleComponent3ID>
	{
	public:
		SampleComponent3(iAllocator* allocator = SystemAllocator) : entities{ allocator } { }

		Sample3Handle CreateComponent()
		{
			return Sample3Handle{ static_cast<unsigned int>(entities.push_back({})) };
		}

		void ReleaseEntity(Sample3Handle handle)
		{
			// do release
		}

		struct Entity
		{
			int x;
		};

		Vector<Entity> entities;
	};


	class SampleView : public ComponentView_t<SampleComponent>
	{
	public:
		SampleView(GameObject& gameObject) :
			handle{ GetComponent().CreateComponent() } {}


		~SampleView()
		{
			GetComponent().ReleaseEntity(handle);
		}


		void DoSomething()
		{
			std::cout << "SampleBehavior1::DoSomething()\n";
			GetComponent().entities[handle].x++;
		}

		SampleHandle handle;
	};


	class SampleView2: public ComponentView_t<SampleComponent2>
	{
	public:
		SampleView2(GameObject& gameObject) :
			handle{ GetComponent().CreateComponent() } {}


		~SampleView2()
		{
			GetComponent().ReleaseEntity(handle);
		}


		void DoSomething()
		{
			std::cout << "SampleBehavior2::DoSomething()\n";
			GetComponent().entities[handle].x++;
		}

		Sample2Handle handle;
	};


	class SampleView3 : public ComponentView_t<SampleComponent3>
	{
	public:
		SampleView3(GameObject& gameObject) :
			handle{ GetComponent().CreateComponent() } {}


		~SampleView3()
		{
			GetComponent().ReleaseEntity(handle);
		}


		void DoSomething()
		{
			std::cout << "SampleBehavior3::DoSomething()\n";
			GetComponent().entities[handle].x++;
		}

		Sample3Handle handle;
	};
	

	/************************************************************************************************/


	using TestView = BasicComponentView_t<StringIDComponent>;
	inline void GameOjectTest(iAllocator* allocator);


	/************************************************************************************************/

	typedef uint32_t UpdateID_t;

	class UpdateTask
	{
	public:
		struct iUpdateFN
		{
			virtual void operator () (UpdateTask& task, iAllocator& threadAllocator) = 0;
		};


		UpdateTask(ThreadManager* IN_manager, iUpdateFN& IN_updateFn, iAllocator* IN_allocator) :
			Update		{ IN_updateFn						},
			threadTask	{ this, IN_manager, IN_allocator	} {}

		// No Copy
		UpdateTask				(const UpdateTask&)	= delete;
		UpdateTask& operator =	(const UpdateTask&)	= delete;


		class UpdateThreadTask : public iWork
		{
		public:
			UpdateThreadTask(UpdateTask* IN_task, ThreadManager* threads, iAllocator* memory) :
				iWork	{ memory    },
				task	{ IN_task	}{}

			// No Copy
			UpdateThreadTask				(const UpdateThreadTask&) = delete;
			UpdateThreadTask& operator =	(const UpdateThreadTask&) = delete;

			UpdateTask*	task;

			void Run(iAllocator& threadAllocator) override
			{
				ProfileFunction();

				task->Run(threadAllocator);
			}

			void Release() override {}
		}threadTask;


		void Run(iAllocator& threadAllocator)
		{
			Update(*this, threadAllocator);
		}


		bool isLeaf() const noexcept
		{
			return leaf;
		}


		void AddInput(UpdateTask& input) 
		{
			counter++;
			leaf = false;

			input.AddContinuationTask(
				[&]
				{
					if (_DecrementCounter())
						PushToLocalQueue(threadTask);
				});
		}


		void AddOutput(UpdateTask& output)
		{
			output.AddInput(*this);
		}


		bool _DecrementCounter()
		{
			const auto count = counter.fetch_sub(1);
			return count == 1;
		}


		void AddContinuation(UpdateTask& task)
		{
			threadTask.Subscribe(
				[&]
				{
					PushToLocalQueue(task);
				});
		}


		template<typename TY_FN>
		void AddContinuationTask(TY_FN task)
		{
			threadTask.Subscribe(task);
		}


		operator iWork& () { return threadTask; }
		operator iWork* () { return &threadTask; }

		std::atomic_int		counter		= 0;
		bool				leaf		= true;

		UpdateID_t			ID			= (uint32_t)-1;
		iUpdateFN&			Update;
		char*				Data;
	};


	class UpdateDispatcher
	{
	public:
		template<typename TY>
		class Task : public UpdateTask
		{
		public:
			Task(ThreadManager* IN_manager, Task::iUpdateFN& IN_updateFn, iAllocator* IN_allocator) :
				UpdateTask{ IN_manager, IN_updateFn, IN_allocator	} {}

			const TY& GetData() const
			{
				return *(reinterpret_cast<TY*>(Data));
			}
		};


		UpdateDispatcher(ThreadManager* IN_threads, iAllocator* IN_allocator) :
			nodes		{ IN_allocator	},
			allocator	{ IN_allocator	},
			threads		{ IN_threads	},
			taskMap		{ IN_allocator  } {}


		// No Copy
		UpdateDispatcher					(const UpdateDispatcher&) = delete;
		const UpdateDispatcher& operator =	(const UpdateDispatcher&) = delete;


		void Execute()
		{
			ProfileFunction();
			WorkBarrier barrier{ *threads, allocator };

			for (auto& node : nodes)
				barrier.AddWork(node->threadTask);

			for (auto node : nodes)
				if (node->isLeaf())
					threads->AddWork(node->threadTask);

			barrier.JoinLocal();

			nodes.clear();
			taskMap.clear();
		}


		class UpdateBuilder
		{
		public:
			UpdateBuilder(UpdateTask& IN_node, UpdateDispatcher& IN_dispatcher) :
				newNode		{ IN_node		},
				dispatcher	{ IN_dispatcher	} {}


			void SetDebugString(const char* str) noexcept
			{
				newNode.threadTask._debugID = str;
			}


			void AddOutput(UpdateTask& node)
			{
				node.AddInput(newNode);
			}


			void AddInput(UpdateTask& node)
			{
				newNode.AddInput(node);
			}


			void AddOutput(uint32_t taskID)
			{
				auto res = find(
					dispatcher.taskMap,
					[&](auto task){ return  std::get<0>(task) == taskID; });

				if (res != dispatcher.taskMap.end())
				{
					auto node = std::get<1>(*res);
					AddOutput(*node);
				}
				else
					FK_ASSERT(false, "Failed to find output Task!");
			}


			void AddInput(uint32_t taskID)
			{
				auto res = find(
					dispatcher.taskMap,
					[&](auto task){ return  std::get<0>(task) == taskID; });

				if (res != dispatcher.taskMap.end())
				{
					auto node = std::get<1>(*res);
					AddInput(*node);
				}
				else
					FK_ASSERT(false, "Failed to find inputTask!");
			}

		private:

			UpdateDispatcher&	dispatcher;
			UpdateTask&		newNode;
		};


		template<
			typename TY_NODEDATA,
			typename FN_LINKAGE,
			typename FN_UPDATE>
		Task<TY_NODEDATA>& Add(uint32_t TaskID, FN_LINKAGE LinkageSetup, FN_UPDATE UpdateFN)
		{
			auto& task = Add<TY_NODEDATA>(LinkageSetup, std::move(UpdateFN));

			if (find(taskMap, [&](auto& task) { return std::get<0>(task) == TaskID; } ) == taskMap.end())
				taskMap.push_back({ TaskID, &task });
			else
			{
				FK_LOG_FATAL("Task double scheduled!");
				std::cout << "ERROR!";
				std::exit(-1);
			}

			return task;
		}

		template<
			typename TY_NODEDATA,
			typename FN_LINKAGE,
			typename FN_UPDATE>
		Task<TY_NODEDATA>&	Add(FN_LINKAGE LinkageSetup, FN_UPDATE&& UpdateFN)
		{
			struct data_BoilderPlate : UpdateTask::iUpdateFN
			{
				data_BoilderPlate(FN_UPDATE&& IN_fn) : 
					function{ std::move(IN_fn) } {}

				virtual void operator() (UpdateTask& task, iAllocator& threadAllocator) override
				{
					function(locals, threadAllocator);
				}

				TY_NODEDATA locals;
				FN_UPDATE	function;
			};

			auto& functor		= allocator->allocate_aligned<data_BoilderPlate>(std::move(UpdateFN));
			auto& newNode		= allocator->allocate_aligned<Task<TY_NODEDATA>>(threads, functor, allocator);
			newNode.Data		= reinterpret_cast<char*>(&functor.locals);

			UpdateBuilder Builder{ newNode, *this };
			LinkageSetup(Builder, functor.locals);

			nodes.push_back(&newNode);

			return newNode;
		}

		ThreadManager*								threads;
		Vector<UpdateTask*>							nodes;
		Vector<std::pair<uint32_t, UpdateTask*>>	taskMap;
		iAllocator*									allocator;
	};


	using DependencyBuilder = UpdateDispatcher::UpdateBuilder;

	template<typename TY>
	using UpdateTaskTyped	= UpdateDispatcher::Task<TY>;


}	/************************************************************************************************/


/**********************************************************************

Copyright (c) 2015 - 2023 Robert May

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
