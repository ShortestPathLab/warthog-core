#ifndef WARTHOG_MEMORY_ALLOC_SLAB_FACTORY_H
#define WARTHOG_MEMORY_ALLOC_SLAB_FACTORY_H

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
#include "object.h"

#include <concepts>
#include <memory>

namespace warthog::memory::alloc
{

struct slab_memory_header
{ };

template<size_t Size, size_t Align>
struct alignas(max_align_t) slab_memory : slab_memory_header
{
	static_assert(Size != 0, "Size must be greater than 0.");
	static_assert(
	    std::popcount(Align) == 1 && Align <= alignof(max_align_t),
	    "Align must be a valid alignment.");

	union Val
	{
		uint64_t u64;
		int64_t i64;
		void* p64;
	};
	std::array<Val, 2> h;
	alignas(max_align_t) std::array<std::byte, Size> data[1];

	consteval static size_t
	size() noexcept
	{ return Size; }
	consteval static size_t
	align() noexcept
	{ return Align; }

	consteval static size_t
	size_header() noexcept
	{ return offsetof(slab_memory, data); }
	constexpr static size_t
	size_n(size_t elems) noexcept
	{
		return pad_alignment(
		    size_header() + elems * Size, alignof(max_align_t));
	}

	template<std::derived_from<slab_memory_header> T>
	constexpr T*
	cast() noexcept
	{ return static_cast<T*>(static_cast<slab_memory_header*>(this)); }
};

template<typename T>
using slab_memory_type  = slab_memory<sizeof(T), alignof(T)>;
using slab_memory_bytes = slab_memory<1, alignof(max_align_t)>;

/// @brief Determine the size (in bytes) each slab slab requires for number of
/// Elements of Size in Slab (slab_memory_type,slab_memory_bytes)
/// @tparam Slab
template<size_t Size, typename Slab = slab_memory_bytes>
constexpr size_t
calc_slab_size(size_t Elements) noexcept
{ return Slab::size_n(Elements); }

/// @brief Determine the size (in bytes) of slab_memory_type<ElementType> with
/// number of Elements
template<typename ElementType>
constexpr size_t
calc_slab_size_type(size_t Elements) noexcept
{ return slab_memory_type<ElementType>::size_n(Elements); }

struct slab_factory_params
{
	slab_factory_params() = default;
	slab_factory_params(size_t l_count, bool l_use_size = false)
	    : count(l_count), use_size(l_use_size)
	{ }
	size_t count = 0; ///< amount to set element to, 0 = default
	bool use_size
	    = false; ///< if true: element_size(count), else: element_count(count)
};

/**
 * Factory that generates slab blocks for slab-based factories.
 * If SlabCount == 0, slab_factory holds a dynamic size.
 */
template<
    ByteFactory Upstream, size_t SlabCount, typename Slab = slab_memory_bytes>
class slab_factory : private Upstream
{
public:
	using upstream_factory = Upstream;
	using value_type       = Slab;
	using pointer          = value_type*;
	using size_type        = size_t;

	static consteval uint32_t
	traits() noexcept
	{ return FactoryDefault; }

	static consteval size_type
	alignment() noexcept
	{ return Slab::align(); }
	static consteval size_type
	element_size() noexcept
	{ return Slab::size_n(SlabCount); }
	static consteval size_type
	item_size() noexcept
	{ return Slab::size(); }
	static consteval size_type
	item_count() noexcept
	{ return SlabCount; }

	using Upstream::setup;
	// template <typename... T>
	// constexpr bool setup(T&&... args)
	// {
	// 	return Upstream::setup(std::forward<T>(args)...);
	// }

	pointer
	create()
	{ return reinterpret_cast<pointer>(Upstream::allocate(element_size())); }
	void
	destroy(pointer ptr)
	{
		Upstream::deallocate(
		    reinterpret_cast<typename Upstream::pointer>(ptr), element_size());
	}

	upstream_factory&
	upstream() noexcept
	{ return static_cast<upstream_factory&>(*this); }
	const upstream_factory&
	upstream() const noexcept
	{ return static_cast<const upstream_factory&>(*this); }
};
template<ByteFactory Upstream, typename Slab>
class slab_factory<Upstream, 0, Slab> : private Upstream
{
public:
	using upstream_factory = Upstream;
	using value_type       = Slab;
	using pointer          = value_type*;
	using size_type        = size_t;

	static consteval uint32_t
	traits() noexcept
	{ return FactoryDefault; }

	static consteval size_type
	alignment() noexcept
	{ return Slab::align(); }
	size_type
	element_size() const noexcept
	{ return m_elementSize; }
	static consteval size_type
	item_size() noexcept
	{ return Slab::size(); }
	size_type
	item_count() noexcept
	{ return m_elementCount; }

	constexpr slab_factory() noexcept : slab_factory(1024) { }
	constexpr slab_factory(size_type elem_size) noexcept
	{ element_size(elem_size); }

	template<typename... T>
	bool
	setup(slab_factory_params params, T&&... args)
	{
		if(!Upstream::setup(std::forward<T>(args)...)) return false;
		if(params.count != 0)
		{
			if(params.use_size)
				element_size(params.count);
			else
				element_count(params.count);
		}
		return true;
	}

	[[nodiscard]] pointer
	create()
	{ return reinterpret_cast<pointer>(Upstream::allocate(element_size())); }
	void
	destroy(pointer ptr)
	{
		Upstream::deallocate(
		    reinterpret_cast<typename Upstream::pointer>(ptr), element_size());
	}

	void
	element_size(size_type size)
	{
		size = std::max(size, static_cast<size_type>(Slab::size_n(2)));
		m_elementCount = (size - Slab::size_header()) / Slab::size();
		m_elementSize  = Slab::size_n(m_elementCount);
	}
	void
	element_count(size_type count)
	{
		count          = std::max(count, static_cast<size_type>(0));
		m_elementCount = count;
		m_elementSize  = Slab::size_n(count);
		if(m_elementSize < 64)
		{
			// set to 64
			element_size(64);
		}
	}

	upstream_factory&
	upstream() noexcept
	{ return static_cast<upstream_factory&>(*this); }
	const upstream_factory&
	upstream() const noexcept
	{ return static_cast<const upstream_factory&>(*this); }

protected:
	uint32_t m_elementCount;
	uint32_t m_elementSize;
};

namespace details
{
template<typename T>
struct is_SlabFactory : std::bool_constant<false>
{ };
template<typename Upstream, size_t SlabCount, typename Slab>
struct is_SlabFactory<slab_factory<Upstream, SlabCount, Slab>>
    : std::bool_constant<true>
{ };
template<typename Upstream>
    requires FactoryTraitAll<Upstream, FactoryPointer>
    && FactoryTraitNone<Upstream, FactorySource>
struct is_SlabFactory<Upstream>
    : is_SlabFactory<typename Upstream::upstream_factory>
{ };
}; // namespace details

template<typename T>
concept SlabFactory = details::is_SlabFactory<T>::value;

/// @tparam Slab type of slab to link
///
/// Utility functions to link slab, uses inbuilt h[0] & h[1].
/// Supports three types of link strategy:
/// link: forward link using h[0] only, FIFO.
/// dlink: double link list using h[0] & h[1], supports removal from middle.
/// chain: link lists h[0] and other chains h[1] together.
///
/// Detached slabs have an invalid h[*] set, will be set by push_detached.
template<typename Slab = slab_memory_bytes>
struct slab_link_fn
{
	using pointer = Slab*;

	/// @return the next slab as stored in slab (h[0].p64)
	/// @pre slab != nullptr
	static pointer
	get_next(pointer slab) noexcept
	{ return reinterpret_cast<pointer>(slab->h[0].p64); }
	/// @brief sets the next slab in slab to next_slab
	/// @pre slab != nullptr
	static void
	set_next(pointer slab, pointer next_slab) noexcept
	{ slab->h[0].p64 = next_slab; }

	/// @return the next slab as stored in slab (h[1].p64)
	/// @pre slab != nullptr
	static pointer
	get_prev(pointer slab) noexcept
	{ return reinterpret_cast<pointer>(slab->h[1].p64); }
	/// @brief sets the next slab in slab to next_slab
	/// @pre slab != nullptr
	static void
	set_prev(pointer slab, pointer next_slab) noexcept
	{ slab->h[1].p64 = next_slab; }

	/// @return list attached to current chain link
	/// @pre link != nullptr
	static pointer
	get_chain_list(pointer link) noexcept
	{ return reinterpret_cast<pointer>(link->h[0].p64); }

	/// @return list attached to current chain link
	/// @pre link != nullptr
	static pointer
	get_chain_link(pointer link) noexcept
	{ return reinterpret_cast<pointer>(link->h[1].p64); }

	/// @brief add a detached slab to a forward list of slab
	/// @param head current head(root) of list, can be null
	/// @param detached_slab slab to add to list
	/// @return new head (always detached_slab)
	/// @pre detached_slab != nullptr && detached_slab != head
	static pointer
	list_push_detached(pointer head, pointer detached_slab) noexcept
	{
		assert(detached_slab != nullptr && detached_slab != head);
		set_next(detached_slab, head);
		return detached_slab;
	}

	/// @brief detached (pop) a single slab from list head
	/// @param head head of list to detach (fifo)
	/// @return pair, first => new head, second => detached slab
	static std::pair<pointer, pointer>
	list_pop_detach(pointer head) noexcept
	{
		assert(head != nullptr);
		pointer anext = get_next(head);
		return {anext, head};
	}

	/// @brief add a detached slab to a double list of slab
	/// @param head current head(root) of list, can be null
	/// @param detached_slab slab to add to list
	/// @return new head (always detached_slab)
	/// @pre detached_slab != nullptr && detached_slab != head
	static pointer
	dlist_push_detached(pointer head, pointer detached_slab) noexcept
	{
		assert(detached_slab != nullptr && detached_slab != head);
		set_prev(detached_slab, nullptr);
		set_next(detached_slab, head);
		if(head) [[likely]]
			set_prev(head, detached_slab);
		return detached_slab;
	}

	/// @brief detach a slab from a double list of slab
	/// @param slab the slab to detach (does not update pointers in slab)
	/// @return the next slab (or null) of slab
	/// @pre slab != nullptr
	static pointer
	dlist_detach(pointer slab) noexcept
	{
		assert(slab != nullptr);
		// not root thus aprev is not null
		pointer anext = get_next(slab);
		pointer aprev = get_prev(slab);
		if(aprev) set_next(aprev, anext);
		if(anext) set_prev(anext, aprev);
		return anext;
	}

	/// @brief detach a slab from double list and return new head
	/// @param head the current head of double list
	/// @param slab the slab to detach (does not update pointers in slab)
	/// @return the new head (if changed), or null if last element in list
	/// @pre slab != nullptr && head != nullptr
	static pointer
	dlist_detach(pointer head, pointer slab) noexcept
	{
		assert(slab != nullptr && head != nullptr);
		if(slab == head) [[unlikely]]
		{
			// slab is root
			pointer anext = get_next(slab);
			head          = anext;
			if(anext) set_prev(anext, nullptr);
		}
		else
		{
			// not root thus aprev is not null
			pointer anext = get_next(slab);
			pointer aprev = get_prev(slab);
			assert(aprev != nullptr); // this is not root
			if(anext) set_prev(anext, aprev);
		}
		return head;
	}

	/// @brief push a detached slab onto a chain
	/// @param head chain head (can be null)
	/// @param detached_slab slab to push to chain
	/// @return new head (detached_slab)
	static pointer
	chain_push_detached(pointer head, pointer detached_slab) noexcept
	{
		assert(detached_slab != nullptr && detached_slab != head);
		set_prev(detached_slab, head);    // prev is next chain
		set_next(detached_slab, nullptr); // is not list thus next is null
		return detached_slab;
	}

	/// @brief push a list (from list_push_detached) to chain
	/// @param head chain head
	/// @param list_head list head to push
	/// @return new head (list_head)
	static pointer
	chain_push_list(pointer head, pointer list_head) noexcept
	{
		assert(list_head != nullptr && list_head != head);
		set_prev(list_head, head); // prev is next chain
		return list_head;
	}

	/// @brief detached (pop) a single slab from chain head
	/// @param head head of the chain to detach
	/// @return pair, first => new head, second => detached slab
	static std::pair<pointer, pointer>
	chain_pop_detach(pointer head) noexcept
	{
		assert(head != nullptr);
		pointer list_head = get_next(head);
		if(list_head != nullptr)
		{
			// is list
			// not root thus aprev is not null
			pointer anext = get_next(list_head);
			set_next(head, anext);
			return {head, list_head};
		}
		else
		{
			// not list
			pointer rlist = get_prev(head);
			return {rlist, head};
		}
	}
};

/// @brief Add support to slab_factory to manage a forward list of slabs.
///        O(1) reclaim operations.
///        slab.h[0] and slab.h[1] are managed by this adaptor.
/// @tparam Upstream
template<SlabFactory Upstream>
    requires FactoryTraitNone<Upstream, FactoryOwn>
class slab_link_pattern : public Upstream
{
public:
	using typename Upstream::pointer;
	using typename Upstream::size_type;
	using typename Upstream::value_type;

protected:
	using link_fn = slab_link_fn<std::remove_pointer_t<pointer>>;

public:
	~slab_link_pattern() { release(factory_chain_free_release<Upstream>); }

	static consteval uint32_t
	traits() noexcept
	{ return FactoryOwn | FactoryReuse; }

	using Upstream::alignment;
	using Upstream::element_size;

	pointer
	create()
	{
		pointer res;
		if(m_reuse) { res = pop_reuse(); }
		else
		{
			// allocate new
			res = Upstream::create();
		}
		push_front(res);
		return res;
	}
	void
	destroy(pointer ptr)
	{
		// reuse for later
		remove_from_list(ptr);
		push_reuse(ptr);
	}

	/// @brief only releases memory calimed for reuse
	/// @param free_upstream destorys memory upstream
	void
	release(bool free_upstream = true)
	{
		if(free_upstream)
		{
			release_list(m_root);
			pointer at = m_reuse;
			while(at != nullptr)
			{
				pointer next = link_fn::get_chain_link(at);
				release_list(at);
				at = next;
			}
		}
		m_root  = nullptr;
		m_reuse = nullptr;
	}
	void
	reclaim()
	{
		if(m_root != nullptr)
		{
			push_reuse_list(m_root);
			m_root = nullptr;
		}
	}

protected:
	pointer
	root() noexcept
	{ return m_root; }
	void
	push_front(pointer at) noexcept
	{ m_root = link_fn::dlist_push_detached(m_root, at); }
	void
	remove_from_list(pointer at) noexcept
	{ m_root = link_fn::dlist_detach(m_root, at); }
	void
	push_reuse(pointer at) noexcept
	{ m_reuse = link_fn::chain_push_detached(m_reuse, at); }
	void
	push_reuse_list(pointer front) noexcept
	{ m_reuse = link_fn::chain_push_list(m_reuse, front); }
	[[nodiscard]] pointer
	pop_reuse() noexcept
	{
		assert(m_reuse != nullptr);
		// reuse
		auto [reuse, elem] = link_fn::chain_pop_detach(m_reuse);
		m_reuse            = reuse;
		return elem;
	}
	void
	release_list(pointer at)
	{
		while(at != nullptr)
		{
			pointer next = link_fn::get_next(at);
			Upstream::destroy(at);
			at = next;
		}
	}

protected:
	pointer m_root  = nullptr;
	pointer m_reuse = nullptr;
};

namespace details
{
template<typename Slab>
constexpr bool
slab_valid_elem_size(size_t size, size_t align) noexcept
{
	if(!(std::popcount(align) == 1 && align <= alignof(max_align_t)))
		return false;
	if(!(size > 0 && (size < align || size % align == 0))) return false;
	if(!(size % Slab::size() == 0)) return false;
	return true;
}
} // namespace details

/// @brief params for reshaping SlabFactory
/// @tparam Fact
/// @tparam Size
/// @tparam Align
template<SlabFactory Fact, size_t Size, size_t Align, size_t MinSize = 1>
struct slab_reshape
{
	static constexpr bool dynamic = false;
	static_assert(
	    std::popcount(Align) == 1 && Align <= alignof(max_align_t),
	    "Must be a valid alignment");
	static_assert(
	    Size > 0 && (Size < Align || Size % Align == 0),
	    "Size must be less than or a multiple of Align");
	static_assert(
	    Size % Fact::value_type::size() == 0,
	    "Size must be a mulitple of Slab::size()");
	static constexpr size_t
	header() noexcept
	{ return Fact::value_type::size_header(); }
	static constexpr size_t
	size() noexcept
	{ return std::max(Size, MinSize); }
	static constexpr size_t
	align() noexcept
	{ return Align; }

	static constexpr size_t
	count() noexcept
	    requires requires { Fact::item_count(); }
	{
		return static_cast<size_t>(Fact::value_type::size())
		    * Fact::item_count() / size();
	}
	static constexpr size_t
	count(const Fact& F) noexcept
	{
		return static_cast<size_t>(Fact::value_type::size()) * F.item_count()
		    / size();
	}
};
template<SlabFactory Fact, size_t MinSize>
struct slab_reshape<Fact, 0, 0, MinSize>
{
	static constexpr bool dynamic = true;
	static constexpr size_t
	header() noexcept
	{ return Fact::value_type::size_header(); }
	constexpr size_t
	size() const noexcept
	{ return m_size; }
	constexpr size_t
	align() const noexcept
	{ return m_align; }
	constexpr size_t
	count(const Fact& F) const noexcept
	{
		return static_cast<size_t>(Fact::value_type::size()) * F.item_count()
		    / m_size;
	}

	constexpr bool
	set(uint32_t l_size, uint32_t l_align) noexcept
	{
		l_size = std::max(l_size, static_cast<uint32_t>(MinSize));
		if(!details::slab_valid_elem_size<typename Fact::value_type>(
		       l_size, l_align))
			return false;
		m_size  = l_size;
		m_align = l_align;
		return true;
	}

protected:
	uint32_t m_size  = 0;
	uint32_t m_align = 0;
};

namespace details
{

template<typename Reshape>
concept SlabReshapeCountConstexpr = requires(Reshape re) {
	requires !Reshape::dynamic;
	{ Reshape::count() } -> std::convertible_to<size_t>;
	typename std::integral_constant<size_t, Reshape::count()>;
};

} // namespace details

/// @brief cache the Reshape.count() value, for consteval stores no value
template<typename Reshape, typename Fact>
struct ReshapeCountCache
{
	static constexpr bool dynamic = true;

	void
	set(const Reshape& re, const Fact& fact) noexcept
	{ m_val = static_cast<uint32_t>(re.count(fact)); }

	uint32_t
	operator*() const noexcept
	{
		assert(m_val != 0);
		return m_val;
	}

	uint32_t m_val = 0;
};

template<details::SlabReshapeCountConstexpr Reshape, typename Fact>
struct ReshapeCountCache<Reshape, Fact>
{
	static constexpr bool dynamic = false;

	/// @brief does nothing
	constexpr void
	set(const Reshape& re, const Fact& fact) noexcept
	{ }

	constexpr uint32_t
	operator*() const noexcept
	{ return Reshape::count(); }
};

} // namespace warthog::memory::alloc

#endif // WARTHOG_MEMORY_ALLOC_SLAB_FACTORY_H
