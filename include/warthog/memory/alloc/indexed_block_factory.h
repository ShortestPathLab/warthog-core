#ifndef WARTHOG_MEMORY_ALLOC_INDEXED_BLOCK_FACTORY_H
#define WARTHOG_MEMORY_ALLOC_INDEXED_BLOCK_FACTORY_H

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
#include "factory_adaptor.h"
#include "slab_factory.h"

#include <algorithm>

namespace warthog::memory::alloc
{

/// @brief parameters for indexed_block_factory setup, when using
struct indexed_block_factory_params
{
	constexpr indexed_block_factory_params(
	    size_t l_size, size_t l_align) noexcept
	    : size(l_size), align(l_align)
	{ }
	size_t size;  ///< size of each single item
	size_t align; ///< alignment of each single item
};
template<typename T>
constexpr indexed_block_factory_params indexed_block_factory_params_type
    = indexed_block_factory_params(sizeof(T), alignof(T));

/**
 * SingleFactory that sections off area blocks into set sized allocations.
 * Similar to block_factory, except this one is indexable by pointer to area
 * buffers. Use resize() to change number of elements, or use create() to
 * append one additional element. Use item(i) to select the ith element, only
 * assert checks for range thus undefined if out-of-range.
 *
 * If Size == 0: size is dynamically determined through setup.
 * Otherwise: set size and align at compile time.
 * Elements == 0 makes elements defined, setup() returns false if elements do
 * not fit on area.
 *
 * @tparam Overflow the memory source of pointer to areas once greater than
 * SlabCount
 * @tparam SlabCount the number of areas to store in-class, takes 8*SlabCount
 * of class size, supports max of SlabCount slabs before invoking overflow
 * allocations.
 */
template<
    SlabFactory Upstream, ByteFactory Overflow = void_factory,
    size_t SlabCount = 0, size_t Size = 0, size_t Align = 0>
class indexed_block_factory
    : private slab_link_pattern<overflow_pattern<Upstream, Overflow>>
{
	using pattern  = slab_link_pattern<overflow_pattern<Upstream, Overflow>>;
	using size_set = slab_reshape<Upstream, Size, Align>;

public:
	using upstream_factory = Upstream;
	using value_type       = std::byte;
	using pointer          = value_type*;
	using size_type        = size_t;
	using area             = upstream_factory::value_type;

public:
	~indexed_block_factory() { release(); }

	static consteval uint32_t
	traits() noexcept
	{ return FactoryOwn | FactoryNoFree; }

	constexpr size_type
	alignment() const noexcept
	{ return m_slabSet.align(); }
	constexpr size_type
	element_size() const noexcept
	{ return m_slabSet.size(); }

	/// @brief setup(std::tuple<OverflowParams>, ...)
	template<typename... T>
	constexpr bool
	setup(T&&... args)
	    requires(!size_set::dynamic)
	{
		if(!pattern::setup(std::forward<T>(args)...)) return false;
		m_slabCount.set(m_slabSet, upstream());
		m_slabs[0]     = reinterpret_cast<pointer>(m_slabs.data() + 1);
		m_size         = 0;
		m_slabSize     = 0;
		m_slabCapacity = SlabCount;
		return true;
	}
	/// @brief setup(block_factory_params, std::tuple<OverflowParams>, ...)
	template<typename... T>
	constexpr bool
	setup(indexed_block_factory_params param, T&&... args)
	    requires(size_set::dynamic)
	{
		if(!pattern::setup(std::forward<T>(args)...)) return false;
		if(!m_slabSet.set(param.size, param.align)) return false;
		m_slabCount.set(m_slabSet, upstream());
		m_slabs[0]     = reinterpret_cast<pointer>(m_slabs.data() + 1);
		m_size         = 0;
		m_slabSize     = 0;
		m_slabCapacity = SlabCount;
		return true;
	}

	[[nodiscard]] pointer
	create()
	{
		if(m_currentLeft == 0) [[unlikely]] { new_root(); }
		assert(m_currentLeft > 0);
		pointer ptr
		    = reinterpret_cast<pointer>(pattern::root()) + m_currentPos;
		m_currentPos  += m_slabSet.size();
		m_currentLeft -= 1;
		m_size        += 1;
		return ptr;
	}
	void
	destroy(pointer ptr)
	{ }

	/// @brief only releases memory calimed for reuse
	/// @param free_upstream destorys memory upstream
	void
	release(bool free_upstream = true)
	{
		if(m_slabSize > SlabCount)
		{
			pattern::overflow_deallocate(
			    m_slabs[0], sizeof(pointer) * m_slabCapacity);
		}
		m_currentPos  = 0;
		m_currentLeft = 0;
		m_size        = 0;
		m_slabSize    = 0;
		m_slabs       = {};
		m_slabs[0]    = reinterpret_cast<pointer>(m_slabs.data() + 1);
		pattern::release(free_upstream);
	}
	void
	reclaim()
	{
		m_currentPos  = 0;
		m_currentLeft = 0;
		m_size        = 0;
		m_slabSize    = 0;
		pattern::reclaim();
	}

	size_type
	size() const noexcept
	{ return m_size; }

	void
	resize(size_type items) noexcept
	{
		const uint32_t sc   = *m_slabCount;
		uint32_t slab_id    = (items + (sc - 1)) / sc;
		uint32_t slab_index = items % sc;
		slab_resize(slab_id);
		m_size        = items;
		m_currentPos  = size_set::header() + slab_index * m_slabSet.size();
		m_currentLeft = *m_slabCount - slab_index;
	}

	/// @brief get element by index without checking, undefined if index >
	/// size()
	pointer
	get(size_type index) const noexcept
	{
		uint32_t slab_id    = index / *m_slabCount;
		uint32_t slab_index = index % *m_slabCount;
		assert(slab_id < m_slabCapacity);
		return get_slabs()[slab_id] + slab_index * m_slabSet.size();
	}
	pointer
	get_if(size_type index) const noexcept
	{
		uint32_t slab_id    = index / *m_slabCount;
		uint32_t slab_index = index % *m_slabCount;
		if(slab_id < m_slabCapacity)
		{
			return get_slabs()[slab_id] + slab_index * m_slabSet.size();
		}
		else
		{
			return nullptr;
		}
	}
	pointer
	at(size_type index) const
	{
		uint32_t slab_id    = index / *m_slabCount;
		uint32_t slab_index = index % *m_slabCount;
		if(slab_id < m_slabCapacity)
		{
			return get_slabs()[slab_id] + slab_index * m_slabSet.size();
		}
		else
		{
			throw std::out_of_range("index");
		}
	}

	upstream_factory&
	upstream() noexcept
	{ return static_cast<upstream_factory&>(*this); }
	const upstream_factory&
	upstream() const noexcept
	{ return static_cast<const upstream_factory&>(*this); }

protected:
	pointer*
	get_slabs() const noexcept
	{ return reinterpret_cast<pointer*>(m_slabs[0]); }

	/// @brief allocate slabs
	/// @param amount
	void
	slab_resize(uint32_t amount)
	{
		if(amount >= m_slabCapacity)
		{
			// update m_slabs to new size
			uint32_t new_cap = std::max(amount, uint32_t(m_slabCapacity * 2));
			assert(new_cap > SlabCount);
			pointer* new_ptr = reinterpret_cast<pointer*>(
			    pattern::overflow_allocate(sizeof(pointer) * new_cap));
			std::copy_n(get_slabs(), m_slabSize, new_ptr);
			// fill zero
			std::fill(new_ptr + m_slabSize, new_ptr + new_cap, nullptr);
			// delete old allocation
			if(m_slabCapacity > SlabCount) // if not stored in heap, deallocate
			{
				assert(get_slabs() != nullptr);
				pattern::overflow_deallocate(
				    m_slabs[0], sizeof(pointer) * m_slabCapacity);
			}
			// create new slabs
			m_slabCapacity = new_cap;
			m_slabs[0]     = reinterpret_cast<pointer>(new_ptr);
		}
		pointer* slabs = get_slabs();
		if(amount > m_slabSize)
		{
			// create new slabs
			for(uint32_t i = m_slabSize; i < amount; ++i)
			{
				slabs[i] = reinterpret_cast<pointer>(pattern::create())
				    + size_set::header();
			}
		}
		else if(amount < m_slabSize)
		{
			// destroy old slabs
			for(uint32_t i = m_slabSize; i > amount; --i)
			{
				pattern::destroy(
				    reinterpret_cast<typename pattern::pointer>(
				        std::exchange(slabs[--i], nullptr)));
			}
		}
		m_slabSize = amount;
	}

	void
	new_root() noexcept
	{
		slab_resize(m_slabSize + 1);
		m_currentPos  = size_set::header();
		m_currentLeft = *m_slabCount;
	}

protected:
	uint32_t m_size         = 0;
	uint32_t m_slabSize     = 0;
	uint32_t m_slabCapacity = 0;
	uint32_t m_currentPos   = 0;
	uint32_t m_currentLeft  = 0;
	[[no_unique_address]] ReshapeCountCache<size_set, upstream_factory>
	    m_slabCount = {};
	[[no_unique_address]] size_set m_slabSet;
	std::array<pointer, 1 + SlabCount> m_slabs = {};
};

template<
    typename T, SlabFactory Upstream, ByteFactory Overflow = void_factory,
    size_t SlabCount = 0>
using indexed_block_factory_type = indexed_block_factory<
    Upstream, Overflow, SlabCount, sizeof(T), alignof(T)>;

} // namespace warthog::memory::alloc

#endif // WARTHOG_MEMORY_ALLOC_INDEXED_BLOCK_FACTORY_H
