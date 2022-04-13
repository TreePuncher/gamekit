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

namespace FlexKit
{	/************************************************************************************************/


	constexpr ComponentID InvalidComponentID = -1;

    class GameObject;

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


        virtual void AddComponentView(GameObject& GO, void* user_ptr, const std::byte* buffer, const size_t bufferSize, iAllocator* allocator) {};


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
			if (!component)
				throw(std::runtime_error("Component::GetSystem() : ERROR : Component not initialized!"));

			return *static_cast<TY*>(component);
		}

		static bool isAvailable()
		{
			return component != nullptr;
		}

        virtual void AddComponentView(GameObject& GO, void* user_ptr, const std::byte* buffer, const size_t bufferSize, iAllocator* allocator) override {}
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

        template<typename TY, typename... TY_ARGS> [[nodiscard]]
        static ComponentViewContainer Create(iAllocator& allocator, GameObject& gameObject, TY_ARGS&& ... args)
        {
            ComponentViewContainer component;
            component.componentSize = sizeof(TY);

            if (component.componentSize > sizeof(component.buffer))
                component._ptr = &allocator.allocate<TY>(gameObject, std::forward<TY_ARGS>(args)...);
            else
                new(component.buffer) TY(gameObject, std::forward<TY_ARGS>(args)...);

            component.ID = component.Get()->ID;

            return component;
        }

        ComponentViewBase* operator -> () { return Get(); }
        operator ComponentViewBase* () { return Get(); }

        ComponentViewBase* Get() const
        {
            if (componentSize > sizeof(buffer))
                return _ptr;
            else
                return (ComponentViewBase*)&(buffer[0]);
        }

        ComponentViewBase& Get_ref() const
        {
            return *Get();
        }

        void Release(iAllocator* allocator)
        {
            if (ID == -1)
                return;

            Get()->~ComponentViewBase();

            if (componentSize > componentSize)
                allocator->release(_ptr);

            ID = -1;
        }

        ComponentID ID;
        uint16_t    componentSize;
        union
        {
            ComponentViewBase* _ptr;
            char                buffer[24];
        };
    };


	class GameObject
	{
    public:
		GameObject(iAllocator* IN_allocator = SystemAllocator) :
			allocator{ IN_allocator }
			//behaviors{ allocator } 
		{}


		~GameObject()
		{
			Release();
		}


        GameObject              (const GameObject& rhs) = delete;
        GameObject& operator =  (const GameObject& rhs) = delete;


        GameObject              (GameObject&& rhs) = delete;
        GameObject& operator =  (GameObject&& rhs) = delete;

		template<typename TY_View, typename ... TY_args>
		auto& AddView(TY_args&& ... args)
		{
			static_assert(std::is_base_of<ComponentViewBase, TY_View>(), "You can only add view types!");

            if (!hasView(TY_View::GetComponentID()))
            {
                views.emplace_back(ComponentViewContainer::Create<TY_View>(*allocator, *this, std::forward<TY_args>(args)...));
                return *static_cast<TY_View*>(views.back().Get());
            }
            else
                return *static_cast<TY_View*>(GetView(TY_View::GetComponentID()));
		}


		void RemoveView(ComponentViewBase& view)
		{
			auto const id = view.ID;
			for (auto itr = views.begin(); itr < views.end(); itr++)
			{
				if ((*itr).ID == id) {
					itr->Release(allocator);
					views.remove_unstable(itr);
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
		}


        void _Clear()
        {
            views.clear();
        }


		ComponentViewBase* GetView(ComponentID id) const
		{
			for (const auto& view : views)
				if (view.ID == id)
					return view.Get();

			return nullptr;
		}


		bool hasView(ComponentID id) const
		{
			for (auto& view : views)
				if (view.ID == id)
					return true;

			return false;
		}


        auto begin()    { return views.begin(); }
        auto end()      { return views.end(); }

        auto begin()    const { return views.begin(); }
        auto end()      const { return views.end(); }

	private:


        static_vector<ComponentViewContainer, 16>	views;	        // component + Code
		iAllocator*						        	allocator;
    };


    /************************************************************************************************/


    bool LoadPrefab(GameObject&, const char* assetID,   iAllocator& allocator, void* user = nullptr);
    bool LoadPrefab(GameObject&, uint64_t assetID,      iAllocator& allocator, void* user = nullptr);


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
        if (go.hasView(TY::ValueType::GetComponentID()))
        {
            auto& value = *(typename TY::ValueType*)go.GetView(TY::ValueType::GetComponentID());

            return ty.IsValid(value);
        }
        else
            return false;
    }

    template<typename TY_GO, typename ... TY>
    auto Query(TY_GO& go, TY ... requests) requires( std::is_same_v<std::decay_t<TY_GO>, GameObject> )
    {
        if constexpr (std::is_const_v<TY_GO>)
            static_assert((TY::IsConst() && ...), "All queries must be read only!");

        bool available = (Filter(go, requests) && ...);
        using Tuple_TY = std::tuple<TY::Type...>;

        if (available)
            return std::optional<Tuple_TY>(Tuple_TY{ GetView<TY::ValueType>(go)... });
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
        static void OnCreateView(GameObject& gameObject, void* user_ptr, const std::byte* buffer, const size_t bufferSize, iAllocator* allocator)
        {
        }
    };

    template<typename TY, typename TY_Handle, ComponentID ID, typename TY_EventHandler = BasicComponentEventHandler>
	class BasicComponent_t : public Component<BasicComponent_t<TY, TY_Handle, ID, TY_EventHandler>, ID>
	{
	public:
        using ThisType      = BasicComponent_t<TY, TY_Handle, ID, TY_EventHandler>;
        using EventHandler  = TY_EventHandler;

        template<typename ... TY_args>
        BasicComponent_t(iAllocator* allocator, TY_args&&... args) :
            eventHandler    { std::forward<TY_args>(args)... },
			elements	    { allocator },
			handles		    { allocator } {}

        BasicComponent_t(iAllocator* allocator) :
            elements        { allocator },
            handles         { allocator } {}

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
			handles[handle] = (index_t)elements.push_back({ handle, initial });

			return handle;
		}


        TY_Handle Create(TY&& initial)
        {
            auto handle = handles.GetNewHandle();
            handles[handle] = (index_t)elements.emplace_back(handle, std::move(initial));

            return handle;
        }


        TY_Handle Create()
        {
            auto handle = handles.GetNewHandle();
            handles[handle] = (index_t)elements.emplace_back(handle, TY{});

            return handle;
        }


        void AddComponentView(GameObject& GO, void* user_ptr, const std::byte* buffer, const size_t bufferSize, iAllocator* allocator) override
        {
            eventHandler.OnCreateView(GO, user_ptr, buffer, bufferSize, allocator);
        }


		void Remove(TY_Handle handle)
		{
			auto lastElement			= elements.back();
			elements[handles[handle]]	= lastElement;
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
        TY_EventHandler                         eventHandler;
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

        void AddComponentView(GameObject& GO, void* user_ptr, const std::byte* buffer, const size_t bufferSize, iAllocator* allocator) override;

		HandleUtilities::HandleTable<StringIDHandle>	handles;
		Vector<StringID>								IDs;
	};


	/************************************************************************************************/


	class StringIDView : public ComponentView_t<StringIDComponent>
	{
	public:
        StringIDView(GameObject& gameObject, const char* id, size_t idLen) : ID{ GetComponent().Create(id, idLen) } {}

		char* GetString() const
		{
			return GetComponent()[ID].ID;
		}

        void SetString(const char* string)
        {
            strncpy_s(GetComponent()[ID].ID, 64, string, 64);
        }

                StringIDComponent::StringID* operator -> ()          { return &GetComponent()[ID]; }
        const   StringIDComponent::StringID* operator -> () const    { return &GetComponent()[ID]; }

		StringIDHandle ID;
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
                strncpy_s(ID.GetString(), 64, str, 64);
            });
    }


    template<typename TY>
    concept IsConstCharStar = ( std::is_same_v<const char*, TY>);

    template<IsConstCharStar ... TY>
    struct StringQuery
    {
        using Type          = StringIDView&;
        using ValueType     = StringIDView;
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
    };


    template<IsConstCharStar ... TY>
    struct ROStringQuery
    {
        using Type          = const StringIDView&;
        using ValueType     = StringIDView;
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


	class UpdateDispatcher
	{
	public:
		typedef uint32_t UpdateID_t;

		class UpdateTaskBase
		{
		public:
			struct iUpdateFN
			{
				virtual void operator () (UpdateTaskBase& task, iAllocator& threadAllocator) = 0;
			};


			UpdateTaskBase(ThreadManager* IN_manager, iUpdateFN& IN_updateFn, iAllocator* IN_allocator) :
				Update		{ IN_updateFn						},
				threadTask	{ this, IN_manager, IN_allocator	} {}

			// No Copy
			UpdateTaskBase				(const UpdateTaskBase&)	= delete;
			UpdateTaskBase& operator =	(const UpdateTaskBase&)	= delete;


			class UpdateThreadTask : public iWork
			{
			public:
				UpdateThreadTask(UpdateTaskBase* IN_task, ThreadManager* threads, iAllocator* memory) :
					iWork	{ memory    },
					task	{ IN_task	}{}

				// No Copy
				UpdateThreadTask				(const UpdateThreadTask&) = delete;
				UpdateThreadTask& operator =	(const UpdateThreadTask&) = delete;

				UpdateTaskBase*	task;

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


            void AddInput(UpdateTaskBase& input) 
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


            void AddOutput(UpdateTaskBase& output)
            {
                output.AddInput(*this);
            }


            bool _DecrementCounter()
            {
                const auto count = counter.fetch_sub(1);
                return count == 1;
            }


            void AddContinuation(UpdateTaskBase& task)
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

            std::atomic_int     counter     = 0;
            bool                leaf        = true;

			UpdateID_t			ID          = (uint32_t)-1;
			iUpdateFN&			Update;
			char*			    Data;
		};

		template<typename TY>
		class UpdateTask : 
			public UpdateTaskBase
		{
		public:
			UpdateTask(ThreadManager* IN_manager, UpdateTaskBase::iUpdateFN& IN_updateFn, iAllocator* IN_allocator) :
				UpdateTaskBase  { IN_manager, IN_updateFn, IN_allocator	} {}

			const TY& GetData() const
			{
				return *(reinterpret_cast<TY*>(Data));
			}
		};


		UpdateDispatcher(ThreadManager* IN_threads, iAllocator* IN_allocator) :
			nodes		{ IN_allocator	},
			allocator	{ IN_allocator	},
			threads		{ IN_threads	},
            taskMap     { IN_allocator  } {}


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
			UpdateBuilder(UpdateTaskBase& IN_node, UpdateDispatcher& IN_dispatcher) :
				newNode     { IN_node	    },
                dispatcher  { IN_dispatcher } {}


			void SetDebugString(const char* str) noexcept
			{
				newNode.threadTask._debugID = str;
			}


			void AddOutput(UpdateTaskBase& node)
			{
				node.AddInput(newNode);
			}


			void AddInput(UpdateTaskBase& node)
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

            UpdateDispatcher&   dispatcher;
			UpdateTaskBase&     newNode;
		};


        template<
            typename TY_NODEDATA,
            typename FN_LINKAGE,
            typename FN_UPDATE>
        UpdateTask<TY_NODEDATA>& Add(uint32_t TaskID, FN_LINKAGE LinkageSetup, FN_UPDATE UpdateFN)
        {
            auto& task = Add<TY_NODEDATA>(LinkageSetup, std::move(UpdateFN));

            if (find(taskMap, [&](auto& task) { return std::get<0>(task) == TaskID; } ) == taskMap.end())
                taskMap.push_back({ TaskID, &task });
            else
            {
                std::cout << "ERROR!";
                std::exit(-1);
            }

            return task;
        }

		template<
			typename TY_NODEDATA,
			typename FN_LINKAGE,
			typename FN_UPDATE>
		UpdateTask<TY_NODEDATA>&	Add(FN_LINKAGE LinkageSetup, FN_UPDATE&& UpdateFN)
		{
			struct data_BoilderPlate : UpdateTaskBase::iUpdateFN
			{
                data_BoilderPlate(FN_UPDATE&& IN_fn) : 
                    function{ std::move(IN_fn) } {}

				virtual void operator() (UpdateTaskBase& task, iAllocator& threadAllocator) override
				{
					function(locals, threadAllocator);
				}

				TY_NODEDATA locals;
                FN_UPDATE	function;
			};

			auto& functor		= allocator->allocate_aligned<data_BoilderPlate>(std::move(UpdateFN));
			auto& newNode		= allocator->allocate_aligned<UpdateTask<TY_NODEDATA>>(threads, functor, allocator);
			newNode.Data		= reinterpret_cast<char*>(&functor.locals);

			UpdateBuilder Builder{ newNode, *this };
			LinkageSetup(Builder, functor.locals);

			nodes.push_back(&newNode);

			return newNode;
		}

		ThreadManager*				                    threads;
        Vector<UpdateTaskBase*>		                    nodes;
        Vector<std::pair<uint32_t, UpdateTaskBase*>>	taskMap;
		iAllocator*					                    allocator;
	};


	using DependencyBuilder = UpdateDispatcher::UpdateBuilder;
	using UpdateTask		= UpdateDispatcher::UpdateTaskBase;

	template<typename TY>
	using UpdateTaskTyped	= UpdateDispatcher::UpdateTask<TY>;


}	/************************************************************************************************/


/**********************************************************************

Copyright (c) 2015 - 2021 Robert May

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
