#ifndef WARTHOG_MEMORY_ALLOC_FACTORY_POINTER_H
#define WARTHOG_MEMORY_ALLOC_FACTORY_POINTER_H

/*
MIT License

Copyright (c) 2026 Ryan Hechenberger

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#include "factory.h"

#include <memory_resource>

namespace warthog::memory::alloc
{

template<typename Fact>
struct factory_pointer;

template<Factory Fact>
struct factory_pointer<Fact>
{
	using upstream_factory          = Fact;
	upstream_factory* m_factoryBase = nullptr;
	using value_type                = typename upstream_factory::value_type;
	using pointer                   = typename upstream_factory::pointer;
	using size_type                 = typename upstream_factory::size_type;

	static consteval uint32_t
	traits() noexcept
	{ return Fact::traits() | FactoryPointer; }

	factory_pointer()                       = default;
	factory_pointer(const factory_pointer&) = delete;

	static constexpr size_type
	alignment() noexcept
	    requires requires { Fact::alignment(); }
	{ return Fact::alignment(); }
	constexpr size_type
	alignment() const noexcept
	    requires(!requires { Fact::alignment(); })
	{ return m_factoryBase->alignment(); }
	static constexpr size_type
	element_size() noexcept
	    requires requires { Fact::element_size(); }
	{ return Fact::element_size(); }
	constexpr size_type
	element_size() const noexcept
	    requires(!requires { Fact::element_size(); })
	{ return m_factoryBase->element_size(); }
	static constexpr size_type
	item_count() noexcept
	    requires requires { Fact::item_count(); }
	{ return Fact::item_count(); }
	constexpr size_type
	item_count() noexcept
	    requires(!requires { Fact::item_count(); })
	{ return m_factoryBase->item_count(); }
	static constexpr size_type
	item_size() noexcept
	    requires requires { Fact::item_size(); }
	{ return Fact::item_size(); }
	constexpr size_type
	item_size() noexcept
	    requires(!requires { Fact::item_size(); })
	{ return m_factoryBase->item_size(); }

	constexpr bool
	setup(upstream_factory& upstream)
	{
		m_factoryBase = &upstream;
		return true;
	}

	[[nodiscard]] pointer
	allocate(size_type elems)
	    requires ArrayFactory<Fact>
	{ return m_factoryBase->allocate(elems); }
	[[nodiscard]] pointer
	allocate(size_type elems, size_type align)
	    requires AlignByteFactory<Fact>
	{ return m_factoryBase->allocate(elems, align); }
	void
	deallocate(pointer ptr, size_type elems)
	    requires ArrayFactory<Fact>
	{ return m_factoryBase->deallocate(ptr, elems); }
	void
	deallocate(pointer ptr)
	    requires ArrayFactory<Fact> && requires {
		    { m_factoryBase->deallocate(ptr) };
	    }
	{ return m_factoryBase->deallocate(ptr); }

	[[nodiscard]] pointer
	create()
	    requires SingleFactory<Fact>
	{ return m_factoryBase->create(); }
	void
	destroy(pointer ptr)
	    requires SingleFactory<Fact>
	{ m_factoryBase->destroy(ptr); }

	void
	release(bool free_upstream = true)
	    requires requires { m_factoryBase->release(free_upstream); }
	{ m_factoryBase->release(free_upstream); }
	void
	reclaim()
	    requires requires { m_factoryBase->reclaim(); }
	{ m_factoryBase->reclaim(); }

	upstream_factory&
	upstream() noexcept
	{
		assert(m_factoryBase != nullptr);
		return *m_factoryBase;
	}
	const upstream_factory&
	upstream() const noexcept
	{
		assert(m_factoryBase != nullptr);
		return *m_factoryBase;
	}
};

} // namespace warthog::memory::alloc

#endif // WARTHOG_MEMORY_ALLOC_FACTORY_POINTER_H
