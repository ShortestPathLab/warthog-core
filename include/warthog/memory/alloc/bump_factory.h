#ifndef WARTHOG_MEMORY_ALLOC_BUMP_FACTORY_H
#define WARTHOG_MEMORY_ALLOC_BUMP_FACTORY_H

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

#include "factory_adaptor.h"
#include "slab_factory.h"

namespace warthog::memory::alloc
{

/**
 * Factory that allocates arrays that bump a single pointer forward.
 * Overflow handles allocations that are larger than a slab.
 */
template<SlabFactory Upstream, ByteFactory Overflow = void_factory>
class bump_factory : private overflow_pattern<Upstream, Overflow>
{
	using pattern  = overflow_pattern<Upstream, Overflow>;
	using size_set = slab_reshape<Upstream, 1, alignof(std::max_align_t)>;

public:
	using upstream_factory = Upstream;
	using value_type       = std::byte;
	using pointer          = value_type*;
	using size_type        = size_t;
	using area             = typename pattern::value_type;
	using typename pattern::overflow_type;

	~bump_factory() { release(); }

	static consteval uint32_t
	traits() noexcept
	{
		return FactoryOwn | FactoryNoFree;
	}

	static consteval size_type
	alignment() noexcept
	{
		return size_set::align();
	}
	static consteval size_type
	element_size() noexcept
	{
		return size_set::size();
	}

	using pattern::setup;

	[[nodiscard]] pointer
	allocate(size_type elems)
	{
		return allocate(elems, alignment());
	}
	[[nodiscard]] pointer
	allocate(size_type elems, size_type align)
	{
		if(elems <= Upstream::item_count() * Upstream::item_size() / 2)
		    [[likely]]
		{
			return allocate_bump(elems, align);
		}
		else { return allocate_overflow(elems); }
	}
	void
	deallocate(pointer ptr [[maybe_unused]], size_type elems [[maybe_unused]])
	{ }

	void
	release(bool free_upstream = true)
	{
		if(free_upstream)
		{
			area* a = m_currentSlab;
			while(a != nullptr)
			{
				area* anext = static_cast<area*>(a->h[1].p64);
				delete_area_overflow(a);
				a = anext;
			}
		}
		m_bumpPtr     = nullptr;
		m_bumpSize    = 0;
		m_currentSlab = nullptr;
	}
	void
	reclaim()
	{
		area* a    = m_currentSlab;
		area* keep = nullptr;
		while(a != nullptr)
		{
			area* anext = reinterpret_cast<area*>(a->h[1].p64);
			if(!keep && a->h[0].u64 == 0)
			{
				a->h[1].p64 = nullptr;
				keep        = a;
			}
			else { delete_area_overflow(a); }
			a = anext;
		}
		m_bumpPtr     = nullptr;
		m_bumpSize    = 0;
		m_currentSlab = keep;
	}

	upstream_factory&
	upstream() noexcept
	{
		return static_cast<upstream_factory&>(*this);
	}
	const upstream_factory&
	upstream() const noexcept
	{
		return static_cast<const upstream_factory&>(*this);
	}

	using pattern::overflow;

protected:
	void
	new_area()
	{
		area* a       = pattern::create();
		a->h[0].u64   = 0;
		a->h[1].p64   = static_cast<void*>(m_currentSlab);
		m_currentSlab = a;
		m_bumpPtr     = static_cast<void*>(&a->data[0]);
		m_bumpSize    = pattern::item_count() * pattern::item_size();
	}
	area*
	new_overflow(size_type elems)
	{
		assert(elems > (pattern::item_count() >> 1));
		area* a = reinterpret_cast<area*>(
		    pattern::overflow_allocate(area::size_n(elems)));
		a->h[0].u64   = elems;
		a->h[1].p64   = static_cast<void*>(m_currentSlab);
		m_currentSlab = a;
		return a;
	}
	void
	delete_area_overflow(area* a)
	{
		if(size_type s = static_cast<size_type>(a->h[0].u64); s == 0)
		    [[likely]]
		{
			// area
			pattern::destroy(a);
		}
		else
		{
			pattern::overflow_deallocate(
			    reinterpret_cast<std::byte*>(a), area::size_n(s));
		}
	}
	pointer
	allocate_bump(size_type elems, size_type align)
	{
		assert(
		    elems <= (pattern::item_count() >> 1) && std::popcount(align) == 1
		    && align <= area::align());
		void* p = align_adjust(align, elems, m_bumpPtr, m_bumpSize);
		if(p != nullptr) [[likely]]
			return static_cast<pointer>(p);
		new_area();
		p = align_adjust(align, elems, m_bumpPtr, m_bumpSize);
		assert(p != nullptr);
		return static_cast<pointer>(p);
	}
	pointer
	allocate_overflow(size_type elems)
	{
		// assume expected alignment is less or equal to area::align
		assert(elems > (pattern::item_count() >> 1));
		area* a = new_overflow(elems);
		return reinterpret_cast<pointer>(&a->data[0]);
	}

protected:
	void* m_bumpPtr     = nullptr;
	size_t m_bumpSize   = 0;
	area* m_currentSlab = nullptr;
	[[no_unique_address]] Overflow m_overflow;
};

} // namespace warthog::memory::alloc

#endif // WARTHOG_MEMORY_ALLOC_BUMP_FACTORY_H
