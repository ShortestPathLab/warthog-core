#ifndef WARTHOG_MEMORY_ALLOC_SOURCE_FACTORY_H
#define WARTHOG_MEMORY_ALLOC_SOURCE_FACTORY_H

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
#include "object.h"

#include <cstddef>
#include <cstdlib>
#include <memory_resource>
#include <utility>

namespace warthog::memory::alloc
{

/**
 * Source factory uses malloc to source generate ByteFactory allocations.
 */
class malloc_factory
{
public:
	using value_type = std::byte;
	using pointer    = value_type*;
	using size_type  = size_t;

	static consteval uint32_t
	traits() noexcept
	{ return FactorySource; }

	constexpr malloc_factory() noexcept   = default;
	malloc_factory(const malloc_factory&) = delete;

	constexpr bool
	setup() noexcept
	{ return true; }

	static consteval size_type
	alignment() noexcept
	{ return alignof(max_align_t); }
	static consteval size_type
	element_size() noexcept
	{ return 1; }

	[[nodiscard]] pointer
	allocate(size_type elems)
	{ return static_cast<pointer>(std::malloc(elems)); }
	void
	deallocate(pointer ptr)
	{ std::free(ptr); }
	void
	deallocate(pointer ptr, size_type)
	{ deallocate(ptr); }
};

static_assert(
    ByteFactory<malloc_factory>, "malloc_factory must be a valid Factory");

/**
 * Source factory uses std::pmr::memory_resource* to source generate
 * ByteFactory allocations.
 */
class memory_resource_factory
{
public:
	using value_type = std::byte;
	using pointer    = value_type*;
	using size_type  = size_t;

	/// @brief memory_resource_factory may not align properly with these
	/// underlying traits
	static consteval uint32_t
	traits() noexcept
	{ return FactorySource; }

	constexpr memory_resource_factory() noexcept            = default;
	memory_resource_factory(const memory_resource_factory&) = delete;

	constexpr bool
	setup(std::pmr::memory_resource* res) noexcept
	{
		if(res == nullptr) return false;
		m_memory_resouce = res;
		return true;
	}

	static consteval size_type
	alignment() noexcept
	{ return alignof(max_align_t); }
	static consteval size_type
	element_size() noexcept
	{ return 1; }

	[[nodiscard]] pointer
	allocate(size_type elems)
	{
		return static_cast<pointer>(m_memory_resouce->allocate(
		    elems)); // alignment() should be the same as default
	}
	[[nodiscard]] pointer
	allocate(size_type elems, size_type align)
	{
		return static_cast<pointer>(m_memory_resouce->allocate(
		    elems, align)); // alignment() should be the same as default
	}
	void
	deallocate(pointer ptr, size_type elems)
	{ m_memory_resouce->deallocate(ptr, elems); }
	void
	deallocate(pointer ptr, size_type elems, size_type align)
	{ m_memory_resouce->deallocate(ptr, elems, align); }

	std::pmr::memory_resource*
	get_memory_resource() const noexcept
	{ return m_memory_resouce; }

private:
	std::pmr::memory_resource* m_memory_resouce = nullptr;
};

static_assert(
    ByteFactory<memory_resource_factory>,
    "memory_resource_factory must be a valid Factory");

namespace details
{

struct buffer_overflow_slab
{
	buffer_overflow_slab* next;
	size_t elems;
	alignas(max_align_t) std::byte buffer[sizeof(max_align_t)];
};

template<typename Upstream>
struct BufferOverflowData
{
	buffer_overflow_slab* slab  = nullptr;
	buffer_overflow_slab* reuse = nullptr;
};

template<>
struct BufferOverflowData<void_factory>
{ };

} // namespace details

/**
 * Source factory uses a stack-sized buffer to source ByteFactory allocations.
 * Once buffer is full, will source from Upstream in slabs of size
 * UpstreamSize. If no upstream is set, allocation will fail and return
 * nullptr.
 *
 * @tparam Buffer the size (in bytes) of the buffer to store within class.
 * @tparam Upstream upstream factory once buffer is full, if not set to void.
 * @tparam UpstreamSize the slab size from Upstream to allocate at a time.
 */
template<
    size_t Buffer, ByteFactory Upstream = void_factory,
    size_t UpstreamSize = 0>
class buffer_factory : private Upstream
{
public:
	using upstream_factory = Upstream;
	using value_type       = std::byte;
	using pointer          = value_type*;
	using size_type        = size_t;

	static_assert(
	    Buffer >= alignof(max_align_t), "Buffer must fit max alignment.");
	static_assert(
	    UpstreamSize == 0 || UpstreamSize >= alignof(max_align_t),
	    "Upstream size must fit max alignment (or 0).");

	static consteval uint32_t
	traits() noexcept
	{ return FactoryOwn | FactoryNoFree; }

private:
	static constexpr bool overflow_buffer = !VoidFactory<Upstream>;
	static constexpr bool overflow_size
	    = UpstreamSize != 0 ? UpstreamSize : Buffer;

public:
	constexpr buffer_factory() noexcept   = default;
	buffer_factory(const buffer_factory&) = delete;
	~buffer_factory() { release(factory_chain_free_release<Upstream>); }

	template<typename... T>
	constexpr bool
	setup(T&&... args)
	{
		release(factory_chain_free_release<Upstream>);
		m_bumpPtr  = m_buffer.data();
		m_bumpSize = Buffer;
		return Upstream::setup(std::forward<T>(args)...);
	}

	static consteval size_type
	alignment() noexcept
	{ return alignof(max_align_t); }
	static consteval size_type
	element_size() noexcept
	{ return 1; }

	[[nodiscard]] pointer
	allocate(size_type elems)
	{ return allocate(elems, alignof(max_align_t)); }
	[[nodiscard]] pointer
	allocate(size_type elems, size_type align)
	{
		assert(m_bumpPtr != nullptr);
		if constexpr(overflow_buffer)
		{
			if(auto* p = align_adjust(align, elems, m_bumpPtr, m_bumpSize);
			   p != nullptr) [[likely]]
				return static_cast<pointer>(p);
			if(elems > overflow_size / 2)
			{
				size_t osize = WARTHOG_VAR_STRUCT_DYNAMIC_SIZE(
				    details::buffer_overflow_slab, buffer, elems);
				auto* p = reinterpret_cast<details::buffer_overflow_slab*>(
				    Upstream::allocate(osize));
				// only track if free is required
				if constexpr(FactoryTraitNone<Upstream, FactoryNoFree>)
				{
					p->elems = elems;
					p->next  = std::exchange(m_overflow.slab, p);
				}
				return static_cast<pointer>(static_cast<void*>(p->buffer));
			}
			else
			{
				new_slab_buffer();
				return static_cast<pointer>(
				    align_adjust(align, elems, m_bumpPtr, m_bumpSize));
			}
		}
		else
		{
			return static_cast<pointer>(
			    align_adjust(align, elems, m_bumpPtr, m_bumpSize));
		}
	}
	void
	deallocate(pointer ptr [[maybe_unused]]) // FreeArrayFactory
	{ }
	void
	deallocate(pointer ptr [[maybe_unused]], size_type elems [[maybe_unused]])
	{ }
	void
	deallocate(
	    pointer ptr [[maybe_unused]], size_type elems [[maybe_unused]],
	    size_type align [[maybe_unused]]) // AlignByteFactory
	{ }

	/// @brief only releases memory calimed for reuse
	/// @param free_upstream destorys memory upstream
	void
	release(bool free_upstream [[maybe_unused]] = true)
	{
		if constexpr(FactoryTraitNone<Upstream, FactoryNoFree>)
		{
			// must free to upstream
			if(free_upstream)
			{
				for(details::buffer_overflow_slab* p :
				    {m_overflow.slab, m_overflow.reuse})
				{
					while(p != nullptr)
					{
						auto* pnext = p->next;
						Upstream::deallocate(
						    reinterpret_cast<std::byte*>(p),
						    WARTHOG_VAR_STRUCT_SIZE(
						        details::buffer_overflow_slab, buffer,
						        p->elems));
						p = pnext;
					}
				}
			}
			m_overflow.slab  = nullptr;
			m_overflow.reuse = nullptr;
		}
		m_bumpPtr  = m_buffer.data();
		m_bumpSize = Buffer;
	}
	void
	reclaim()
	{
		if constexpr(overflow_buffer)
		{
			if(m_overflow.slab != nullptr)
			{
				// move slab to reuse
				if(m_overflow.reuse != nullptr)
				{
					// push slab to reuse
					details::buffer_overflow_slab* end_slab = m_overflow.slab;
					while(end_slab->next != nullptr)
					{
						end_slab = end_slab->next;
					}
					// move reuse to end of slab, move slab to reuse
					end_slab->next
					    = std::exchange(m_overflow.reuse, m_overflow.slab);
				}
			}
			m_overflow.slab = nullptr;
		}
		m_bumpPtr  = m_buffer.data();
		m_bumpSize = Buffer;
	}

	upstream_factory&
	upstream() noexcept
	{ return static_cast<upstream_factory&>(*this); }
	const upstream_factory&
	upstream() const noexcept
	{ return static_cast<const upstream_factory&>(*this); }

private:
	details::buffer_overflow_slab*
	new_slab_buffer()
	    requires(overflow_buffer)
	{
		if(m_overflow.reuse != nullptr)
		{
			details::buffer_overflow_slab* p
			    = std::exchange(m_overflow.reuse, m_overflow.reuse->next);
			p->next    = std::exchange(m_overflow.slab, p);
			m_bumpPtr  = p->buffer;
			m_bumpSize = p->elems;
			return p;
		}
		else
		{
			size_t osize = WARTHOG_VAR_STRUCT_SIZE(
			    details::buffer_overflow_slab, buffer, overflow_size);
			auto* p = reinterpret_cast<details::buffer_overflow_slab*>(
			    Upstream::allocate(osize));
			// only track if free is required
			if constexpr(FactoryTraitNone<Upstream, FactoryNoFree>)
			{
				p->elems = overflow_size;
				p->next  = std::exchange(m_overflow.slab, p);
			}
			m_bumpPtr  = p->buffer;
			m_bumpSize = overflow_size;
			return p;
		}
	}

private:
	void* m_bumpPtr   = nullptr;
	size_t m_bumpSize = 0;
	[[no_unique_address]] details::BufferOverflowData<Upstream> m_overflow;
	object_bytes_size<
	    pad_alignment(Buffer, alignof(max_align_t)), alignof(max_align_t)>
	    m_buffer;
};

static_assert(
    ByteFactory<buffer_factory<1024>>,
    "buffer_factory<1024> must be a valid Factory");

} // namespace warthog::memory::alloc

#endif // WARTHOG_MEMORY_ALLOC_SOURCE_FACTORY_H
