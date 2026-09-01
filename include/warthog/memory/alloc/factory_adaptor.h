#ifndef WARTHOG_MEMORY_ALLOC_FACTORY_ADAPTOR_H
#define WARTHOG_MEMORY_ALLOC_FACTORY_ADAPTOR_H

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

#include <warthog/util/template.h>

#include <cstring>
#include <memory>
#include <tuple>

namespace warthog::memory::alloc
{

constexpr size_t
reuse_size(size_t size) noexcept
{ return std::max(size, sizeof(void*)); }
template<typename T>
constexpr size_t
reuse_size() noexcept
{ return reuse_size(sizeof(T)); }

template<SingleFactory Upstream>
    requires(!FactoryTraitAny<Upstream, FactoryOwn | FactoryPointer>)
class reuse_adaptor : public Upstream
{
public:
	using typename Upstream::pointer;
	using typename Upstream::size_type;
	using typename Upstream::value_type;

	static consteval uint32_t
	traits() noexcept
	{ return (Upstream::traits() & ~FactoryNoFree) | FactoryReuse; }

	using Upstream::alignment;
	using Upstream::element_size;

	~reuse_adaptor() { release(factory_chain_free_release<Upstream>); }

	template<typename... T>
	constexpr bool
	setup(T&&... args)
	{
		release(factory_chain_free_release<Upstream>);
		if(!Upstream::setup(std::forward<T>(args)...)) return false;
		if(Upstream::element_size() < sizeof(pointer)) return false;
		return true;
	}

	[[nodiscard]] pointer
	create()
	{
		pointer res;
		if(m_reuse)
		{
			// reuse
			res     = m_reuse;
			m_reuse = reuse_get(res);
		}
		else
		{
			// allocate new
			res = Upstream::create();
		}
		return res;
	}
	void
	destroy(pointer ptr)
	{
		// keep for reuse
		reuse_set(ptr, m_reuse);
		m_reuse = ptr;
	}

	/// @brief only releases memory calimed for reuse
	/// @param free_upstream destorys memory upstream
	void
	release(bool free_upstream = true)
	{
		if constexpr(ReleaseFactory<Upstream>)
		{
			// Upstream has release, just call it
			Upstream::release(free_upstream);
		}
		else if constexpr(!FactoryTraitAll<Upstream, FactoryNoFree>)
		{
			// only call Upstream::destory if upsteam uses one has one
			if(free_upstream)
			{
				pointer p = m_reuse;
				while(p)
				{
					pointer pnext = reuse_get(p);
					Upstream::destroy(p);
					p = pnext;
				}
			}
		}
		m_reuse = nullptr;
	}
	void
	reclaim()
	    requires ReclaimFactory<Upstream>
	{
		Upstream::reclaim();
		m_reuse = nullptr;
	}

protected:
	static pointer
	reuse_get(pointer p) noexcept
	{
		// handle unaligned access
		pointer value;
		std::memcpy(&value, p, sizeof(pointer));
		return value;
	}
	static void
	reuse_set(pointer p, pointer value) noexcept
	{
		// handle unaligned access
		std::memcpy(p, &value, sizeof(pointer));
	}

protected:
	pointer m_reuse = nullptr;
};

template<ByteFactory Upstream>
class release_adaptor : private Upstream
{
public:
	using upstream_factory = Upstream;
	using typename Upstream::pointer;
	using typename Upstream::size_type;
	using typename Upstream::value_type;

	static consteval uint32_t
	traits() noexcept
	{ return FactoryOwn; }

	using Upstream::alignment;
	using Upstream::element_size;
	using Upstream::setup;
	using Upstream::Upstream;

private:
	struct alignas(max_align_t) pointer_meta_size
	{
		pointer prev;
		pointer next;
		uintptr_t size;
	};
	struct alignas(max_align_t) pointer_meta
	{
		pointer prev;
		pointer next;
		// pointer_meta& operator=(const pointer_meta_size& m)
		// {
		// 	prev = m.prev;
		// 	next = m.next;
		// }
	};
	static constexpr bool auto_free
	    = !FactoryTraitAll<upstream_factory, FactoryNoFree>;
	static constexpr bool store_size
	    = !auto_free && !FreeArrayFactory<upstream_factory>;
	using store_meta
	    = std::conditional_t<store_size, pointer_meta_size, pointer_meta>;

public:
	~release_adaptor() { release(true); }
	template<typename... T>
	constexpr bool
	setup(T&&... args)
	{ return Upstream::setup(std::forward<T>(args)...); }

	[[nodiscard]] pointer
	allocate(size_type elems)
	{
		auto* ptr = reinterpret_cast<pointer>(
		    reinterpret_cast<pointer*>(
		        upstream_factory::allocate(elems + 2 * sizeof(pointer*)))
		    + 2);
		pointer next   = std::exchange(m_linkStart, ptr);
		link_next(ptr) = next;
		if(next != nullptr) [[likely]] { link_prev(next) = ptr; }
	}
	void
	deallocate(pointer ptr)
	{
		auto meta = link_meta(ptr);
		if(meta.prev != nullptr) [[likely]]
		{
			link_next(meta.prev) = meta.next;
		}
		else
		{
			m_linkStart = meta.next;
		}
		if(meta.next != nullptr) [[likely]]
		{
			link_prev(meta.next) = meta.prev;
		}
		if constexpr(!auto_free)
		{
			if constexpr(store_size) { free_(ptr, meta.size); }
			else
			{
				free_(ptr, 0);
			}
		}
	}
	void
	deallocate(pointer ptr, size_type elems)
	{ deallocate(ptr); }

	void
	release(bool free_upstream [[maybe_unused]] = true)
	{
		if constexpr(!auto_free)
		{
			if(free_upstream)
			{
				for(pointer p = m_linkStart; p != nullptr;)
				{
					auto meta   = link_meta(p);
					auto next_p = meta.next;
					if constexpr(store_size) { free_(p, meta.size); }
					else
					{
						free_(p, 0);
					}
					p = next_p;
				}
			}
		}
		m_linkStart = nullptr;
	}

	upstream_factory&
	upstream() noexcept
	{ return static_cast<upstream_factory&>(*this); }
	const upstream_factory&
	upstream() const noexcept
	{ return static_cast<const upstream_factory&>(*this); }

protected:
	static store_meta&
	link_meta(pointer mem) noexcept
	{ return *reinterpret_cast<store_meta*>(mem - sizeof(store_meta)); }
	pointer
	allocate_(size_type elems)
	{
		elems      += sizeof(store_meta);
		pointer ptr = upstream_factory::allocate(elems);
		reinterpret_cast<store_meta*>(ptr)->size = elems;
		return ptr + sizeof(store_meta);
	}
	void
	free_(pointer ptr, size_type elems [[maybe_unused]])
	{
		if constexpr(store_size)
		{
			upstream_factory::deallocate(ptr - sizeof(store_meta), elems);
		}
		else
		{
			upstream_factory::deallocate(ptr - sizeof(store_meta));
		}
	}

protected:
	pointer m_linkStart = nullptr;
};

template<Factory Upstream, ByteFactory Overflow = void_factory>
class overflow_pattern : public Upstream
{
public:
	using overflow_type = Overflow;
	static consteval bool
	overflow_is_pointer() noexcept
	{ return FactoryTraitAny<overflow_type, FactoryPointer>; }

	template<util::Tuple OverflowTuple, typename... T>
	constexpr bool
	setup(OverflowTuple&& setup_overflow, T&&... args)
	{
		if(!Upstream::setup(std::forward<T>(args)...)) return false;
		if(!std::apply(
		       [&of = m_overflow](auto&&... ts) {
			       return of.setup(std::forward<decltype(ts)>(ts)...);
		       },
		       setup_overflow))
		{
			return false;
		}
		return true;
	}

	overflow_type&
	overflow() noexcept
	{ return m_overflow; }
	const overflow_type&
	overflow() const noexcept
	{ return m_overflow; }

protected:
	constexpr typename Overflow::pointer
	overflow_allocate(typename Overflow::size_type elems)
	{ return m_overflow.allocate(elems); }
	constexpr void
	overflow_deallocate(
	    typename Overflow::pointer ptr, typename Overflow::size_type elems)
	{ return m_overflow.deallocate(ptr, elems); }

protected:
	[[no_unique_address]] Overflow m_overflow;
};

template<Factory Upstream, ByteFactory Overflow>
    requires VoidFactory<Overflow>
class overflow_pattern<Upstream, Overflow> : public Upstream
{
public:
	using Upstream::Upstream;

protected:
	template<Factory Fact>
	static ByteFactory auto&
	overflow_upstream(Fact& upsteam_fact) noexcept
	{
		if constexpr(ByteFactory<Fact>) { return upsteam_fact; }
		else
		{
			return overflow_upstream(upsteam_fact.upstream());
		}
	}

	template<Factory Fact>
	static consteval bool
	overflow_is_pointer_upstream() noexcept
	{
		if constexpr(FactoryTraitAny<Fact, FactoryPointer>) { return true; }
		else if constexpr(ByteFactory<Fact>) { return false; }
		else
		{
			return overflow_is_pointer_upstream<
			    typename Fact::upstream_factory>();
		}
	}

public:
	using overflow_type = std::remove_cvref_t<decltype(overflow_upstream(
	    std::declval<Upstream>().upstream()))>;
	static consteval bool
	overflow_is_pointer() noexcept
	{ return overflow_is_pointer_upstream<Upstream>; }

	overflow_type&
	overflow() noexcept
	{ return overflow_upstream(Upstream::upstream()); }
	const overflow_type&
	overflow() const noexcept
	{ return overflow_upstream(Upstream::upstream()); }

protected:
	constexpr typename Overflow::pointer
	overflow_allocate(typename Overflow::size_type elems)
	{ return overflow().allocate(elems); }
	constexpr void
	overflow_deallocate(
	    typename Overflow::pointer ptr, typename Overflow::size_type elems)
	{ return overflow().deallocate(ptr, elems); }

protected:
	[[no_unique_address]] Overflow m_overflow;
};

/// @brief params for setting optional dynamic size, use 0,0 for compile-time
/// @tparam Elems number of elements to allocate from upstream; > 0 means
/// compile-time
/// @tparam Align in bytes; > 0 means compile-time size
template<size_t Elems, size_t Align, size_t MinElems = 1>
struct dynamic_size
{
	static constexpr bool dynamic = false;
	static_assert(
	    std::popcount(Align) == 1 && Align <= alignof(max_align_t),
	    "Must be a valid alignment");
	static_assert(Elems > 0 && MinElems > 0, "Elems must be greater than 0");
	constexpr uint32_t
	elements() const noexcept
	{ return std::max(Elems, MinElems); }
	constexpr uint32_t
	align() const noexcept
	{ return Align; }

	/// @brief mainly a check that array can support size an align
	bool
	set(const ArrayFactory auto& upstream)
	{
		const uint32_t s = elements() * upstream.element_size();
		return Align <= upstream.alignment() && s > 0
		    && (s <= Align || s % Align == 0);
	}
};
template<size_t Align, size_t MinElems>
struct dynamic_size<0, Align, MinElems>
{
	static constexpr bool dynamic = true;
	constexpr uint32_t
	elements() const noexcept
	{ return m_size; }
	constexpr uint32_t
	align() const noexcept
	    requires(Align != 0)
	{ return Align; }
	constexpr uint32_t
	align() const noexcept
	    requires(Align == 0)
	{ return m_align; }

	bool
	set(const ArrayFactory auto& upstream, uint32_t l_size,
	    uint32_t l_align) noexcept
	{
		l_size           = std::max(l_size, static_cast<uint32_t>(MinElems));
		const uint32_t s = l_size * upstream.element_size();
		if constexpr(Align == 0)
		{
			if(l_align == 0) { l_align = upstream.alignment(); }
			else
			{
				if(std::popcount(l_align) != 1
				   || l_align > alignof(max_align_t))
					return false; // invalid align
				if(l_align > upstream.alignment())
					return false; // align must fit into upstream alignment
			}
			m_align = l_align;
		}
		else
		{
			if(l_align != 0) return false; // must not be set
		}
		if(s == 0 || !(s <= l_align || s % l_align == 0))
			return false; // invalid size
		m_size = l_size;
		return true;
	}

protected:
	uint32_t m_size  = 0;
	uint32_t m_align = 0; ///< only used if align==0
};

/// @brief parameters for block_factory setup, when using
struct dynamic_factory_params
{
	constexpr dynamic_factory_params(size_t l_elems, size_t l_align)
	    : elems(l_elems), align(l_align)
	{ }
	size_t elems;
	size_t align;
};

} // namespace warthog::memory::alloc

#endif // WARTHOG_MEMORY_ALLOC_SINGLE_FACTORY_H
