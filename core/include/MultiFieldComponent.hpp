#include <Components.hpp>
#include <MultiField.hpp>

namespace FlexKit
{
	template<typename TY_Handle, ComponentID ID, typename TY_EventHandler, typename ... TY_Fields>
	class MultiFieldComponent_t : public Component<MultiFieldComponent_t<TY_Handle, ID, TY_EventHandler, TY_Fields...>, ID>
	{
	public:
		using ThisType		= MultiFieldComponent_t<TY_Handle, ID, TY_EventHandler, TY_Fields...>;
		using EventHandler	= TY_EventHandler;

		struct MultiFieldComponentView : public ComponentView_t<MultiFieldComponent_t>
		{
			MultiFieldComponentView(GameObject& gameObject, TY_Handle IN_handle) : handle{ IN_handle } {}
			MultiFieldComponentView(GameObject& gameObject, TY_Fields&& ... args) : handle{ MultiFieldComponentView::GetComponent().Create(gameObject, std::forward<TY_Fields>(args)...) } {}


			MultiFieldComponentView(const MultiFieldComponentView&)					= delete;
			MultiFieldComponentView& operator = (const MultiFieldComponentView&)	= delete;

			MultiFieldComponentView(MultiFieldComponentView&&)						= delete;
			MultiFieldComponentView& operator = (MultiFieldComponentView&&)			= delete;

			void Release()
			{
				MultiFieldComponent_t::GetComponent().Remove(handle);
				handle = InvalidHandle;
			}

			decltype(auto) operator -> ()
			{
				return &GetData();
			}

			decltype(auto) GetData()
			{
				return MultiFieldComponentView::GetComponent()[handle];
			}

			template<size_t ... FieldIDs>
			decltype(auto) Slice()
			{
				return MultiFieldComponentView::GetComponent()[handle];
			}

			TY_Handle handle = InvalidHandle;
		};

		template<typename ... TY_args>
		MultiFieldComponent_t(iAllocator* allocator, TY_args&&... args) :
			eventHandler	{ std::forward<TY_args>(args)... },
			fields			{ allocator },
			handles			{ allocator } 
		{
			fields.push_back(1);
		}

		MultiFieldComponent_t(iAllocator* allocator) :
			fields	{ *allocator },
			handles	{  allocator } 
		{
		}

		struct elementData
		{
			TY_Handle		handle;
		};

		using View = MultiFieldComponentView;


		TY_Handle Create(GameObject& gameObject, const TY_Fields& ... initial)
		{
			auto handle = handles.GetNewHandle();
			handles[handle] = fields.push_back(handle, initial...);

			return handle;
		}


		TY_Handle Create(GameObject& gameObject, TY_Fields&& ... initial)
		{
			auto handle = handles.GetNewHandle();
			handles[handle] = fields.push_back(handle, initial...);

			return handle;
		}


		TY_Handle Create(GameObject& gameObject) requires ComponentVoidCreator<TY_EventHandler>
		{
			auto handle = handles.GetNewHandle();
			std::apply(
				[&](auto ...args) 
				{
					handles[handle] = fields.push_back(handle, args...);
				}, eventHandler(gameObject));

			return handle;
		}

		auto operator [](TY_Handle handle) -> decltype(auto)
		{
			return fields.Slice<1, sizeof ... (TY_Fields)>()[handles[handle]];
		}


		auto operator [](TY_Handle handle) const -> decltype(auto)
		{
			return fields.Slice<1, sizeof ... (TY_Fields)>()[handles[handle]];
		}

		void AddComponentView(GameObject& gameObject, ValueMap values, const std::byte* buffer, const size_t bufferSize, iAllocator* allocator) override
		{
			eventHandler.OnCreateView(gameObject, values, buffer, bufferSize, allocator);
		}


		void FreeComponentView(void* _ptr) override
		{
			reinterpret_cast<View*>(_ptr)->Release();
		}

		void Remove(TY_Handle handle)
		{
			auto lastElement = std::move(fields.back());
			fields[handles[handle]] = std::move(lastElement);
			fields.pop_back();

			handles[std::get<0>(lastElement)] = handles[handle];
			handles.RemoveHandle(handle);
		}

		auto& GetFactory() noexcept { return eventHandler; }

		auto begin()	{ return fields.begin(); }
		auto end()		{ return fields.end(); }


		template<size_t ... FieldIDs> decltype(auto) Slice() noexcept { return fields.Slice<(FieldIDs + 1)...>(); }
		template<size_t ... FieldIDs> decltype(auto) Slice(size_t begin, size_t end) noexcept { return fields.Slice<(FieldIDs + 1)...>(begin, end); }


		size_t size() const noexcept { return fields.size(); }

		HandleUtilities::HandleTable<TY_Handle>	handles;
		MultiField<TY_Handle, TY_Fields...>		fields;
		NO_UNIQUE_ADDRESS TY_EventHandler		eventHandler;
	};

	struct MultiFieldComponentEventHandler
	{
		static decltype(auto) OnCreate(GameObject&, auto&& args)
		{
			return args;
		}

		static void OnCreateView(GameObject& gameObject, ValueMap user_ptr, const std::byte* buffer, const size_t bufferSize, iAllocator* allocator)
		{
		}
	};
}


/**********************************************************************

Copyright (c) 2024 Robert May

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
