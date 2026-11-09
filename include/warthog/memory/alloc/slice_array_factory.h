#ifndef WARTHOG_MEMORY_ALLOC_SLICE_ARRAY_FACTORY_H
#define WARTHOG_MEMORY_ALLOC_SLICE_ARRAY_FACTORY_H

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

namespace warthog::memory::alloc
{

struct slice_array_factory_params
{
	constexpr slice_array_factory_params(
	    size_t l_size, size_t l_align) noexcept
	    : size(l_size), align(l_align)
	{ }
	size_t size;  ///< size of each single item
	size_t align; ///< alignment of each single item
};
template<typename T>
constexpr slice_array_factory_params slice_array_factory_params_type
    = slice_array_factory_params(sizeof(T), alignof(T));

/// @brief cache the Reshape.count() value, for consteval stores no value
template<typename Reshape, typename Fact, size_t Max2k>
struct ReshapeSlice2kCache
{
	static constexpr bool dynamic = true;

	void
	set(const Reshape& re, const Fact& fact) noexcept
	{
		m_val = std::min(
		    (uint32_t)std::bit_width(
		        static_cast<uint32_t>(re.count(fact)) - 1),
		    (uint32_t)Max2k);
	}

	uint32_t
	operator*() const noexcept
	{
		assert(m_val != 0);
		return m_val;
	}

	uint32_t m_val = 0;
};

template<
    details::SlabReshapeCountConstexpr Reshape, typename Fact, size_t Max2k>
struct ReshapeSlice2kCache<Reshape, Fact, Max2k>
{
	static constexpr bool dynamic = false;

	/// @brief does nothing
	constexpr void
	set(const Reshape& re, const Fact& fact) noexcept
	{ }

	constexpr uint32_t
	operator*() const noexcept
	{
		return std::min(
		    (uint32_t)std::bit_width(Reshape::count() - 1), (uint32_t)Max2k);
	}
};

/**
 * ArrayFactory that allows general purpose allocation and re-allocation of
 * arrays. Allocates elements to buffers of size 2^k, where k=ceil(log_2(n)).
 * Normal allocate just allocates the smallest 2^k array that fits n.
 * Use allocate_array to also get the array size (in elements Size).
 *
 * If Size == 0 && Align == 0: size is given to
 * setup(slice_array_factory_params). Otherwise: set size and align at compile
 * time..
 *
 * @tparam Overflow the alternative memory allocation when array is large,
 * these allocations are not reused.
 * @tparam Max2k the maximum number of k, also limited by the slab size to fit
 * 2^k * Size.
 * @tparam Size size of individual elements.
 * @tparam Align alignment of individual elements.
 */
template<
    SlabFactory Upstream, ByteFactory Overflow = void_factory,
    size_t Max2k = 16, size_t Min2k = 4, size_t Size = 0, size_t Align = 0>
class slice_array_factory : private overflow_pattern<Upstream, Overflow>
{
	using pattern  = overflow_pattern<Upstream, Overflow>;
	using size_set = slab_reshape<Upstream, Size, Align>;

	static_assert(Max2k > Min2k, "Max2k must be strictly larger than Min2k");

public:
	using upstream_factory = Upstream;
	using value_type       = std::byte;
	using pointer          = value_type*;
	using size_type        = size_t;
	using area             = upstream_factory::value_type;
	using overflow_area    = slab_memory_bytes;

protected:
	using link_fn     = slab_link_fn<area>;
	using overflow_fn = slab_link_fn<overflow_area>;

	struct Slab2k
	{
		area* l;         ///< list of slabs in use
		pointer re_elem; ///< first reuse element
	};

public:
	~slice_array_factory() { release(); }

	static consteval uint32_t
	traits() noexcept
	{
		return FactoryOwn | FactoryReuse;
	}

	constexpr size_type
	alignment() const noexcept
	{
		return m_slabSet.align();
	}
	constexpr size_type
	element_size() const noexcept
	{
		return m_slabSet.size();
	}

	constexpr uint32_t
	max2k() const noexcept
	{
		return *m_slab2kMax;
	}

	constexpr uint32_t
	size2k(size_type elems) const noexcept
	{
		assert(elems > 0);
		[[assume(elems > 0)]];
		return std::bit_width((uint32_t)(elems - 1));
	}

	/// @brief setup(std::tuple<OverflowParams>, ...)
	template<typename... T>
	constexpr bool
	setup(T&&... args)
	    requires(!size_set::dynamic)
	{
		if(!pattern::setup(std::forward<T>(args)...)) return false;
		m_slab2kMax.set(m_slabSet, upstream());
		if((1ull << Min2k) * element_size() < sizeof(pointer)) return false;

		return max2k() >= Min2k;
	}
	/// @brief setup(block_factory_params, std::tuple<OverflowParams>, ...)
	template<typename... T>
	constexpr bool
	setup(slice_array_factory_params param, T&&... args)
	    requires(size_set::dynamic)
	{
		if(!pattern::setup(std::forward<T>(args)...)) return false;
		if(!m_slabSet.set(param.size, param.align)) return false;
		if((1ull << Min2k) * element_size() < sizeof(pointer)) return false;
		m_slab2kMax.set(m_slabSet, upstream());

		return max2k() >= Min2k;
	}

	[[nodiscard]] pointer
	allocate(size_type elems)
	{
		return allocate_array(elems).first;
	}
	void
	deallocate(pointer ptr, size_type elems)
	{
		deallocate_array(ptr, elems);
	}

	std::pair<pointer, size_type>
	allocate_array(size_type min_elems)
	{
		if(min_elems == 0) return {nullptr, 0};
		uint32_t bucket2k = size2k(min_elems);
		if(bucket2k > max2k()) [[unlikely]]
		{
			// overflow
			return {elem_over_new(min_elems), min_elems};
		}
		bucket2k = std::max(bucket2k, (uint32_t)Min2k);
		assert(Min2k <= bucket2k && bucket2k <= max2k());
		return {elem2k_new(bucket2k), 1ull << bucket2k};
	}
	void
	deallocate_array(pointer ptr, size_type elems)
	{
		if(ptr == nullptr || elems == 0) return;
		uint32_t bucket2k = size2k(elems);
		if(bucket2k > max2k()) [[unlikely]]
		{
			// overflow
			elem_over_del(ptr);
			return;
		}
		bucket2k = std::max(bucket2k, (uint32_t)Min2k);
		assert(Min2k <= bucket2k && bucket2k <= max2k());
		elem2k_del(bucket2k, ptr);
	}

	/// @brief only releases memory calimed for reuse
	/// @param free_upstream destorys memory upstream
	void
	release(bool free_upstream = true)
	{
		elem_over_release();
		for(Slab2k& sk : m_slabs)
		{
			slab_free_list(sk.l);
		}
		m_slabs = {};
		slab_free_reuse();
	}
	void
	reclaim()
	{
		elem_over_release();
		slab_reclaim();
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

protected:
	// handle slabs

	[[nodiscard]] area*
	slab_new()
	{
		if(m_slabReuse)
		{
			// reuse existing slab
			auto [h, s] = link_fn::list_pop_detach(m_slabReuse);
			m_slabReuse = h;
			return s;
		}
		else
		{
			// create new slab
			return upstream_factory::create();
		}
	}

	void
	slab_reclaim()
	{
		for(Slab2k& sk : m_slabs)
		{
			if(sk.l)
			{
				m_slabReuse = link_fn::chain_push_list(m_slabReuse, sk.l);
				sk.l        = nullptr;
				sk.re_elem  = nullptr;
			}
		}
	}

	void
	slab_free_list(area* slab)
	{
		while(slab != nullptr)
		{
			area* snext = link_fn::get_next(slab);
			upstream_factory::destroy(slab);
			slab = snext;
		}
	}

	void
	slab_free_reuse()
	{
		for(area* slab = m_slabReuse; slab != nullptr;)
		{
			area* snext = link_fn::get_chain_link(slab);
			slab_free_list(link_fn::get_chain_list(slab));
			slab = snext;
		}
		m_slabReuse = nullptr;
	}

	// handle 2k elements and slabs

	pointer
	elem2k_new(uint32_t i)
	{
		assert(Min2k <= i && i <= max2k());
		Slab2k& s = m_slabs[i - Min2k];
		if(s.re_elem)
		{
			pointer ret = s.re_elem;
			s.re_elem   = *reinterpret_cast<pointer*>(ret);
			return ret;
		}
		// new element
		if(s.l == nullptr || s.l->h[1].u64 >= (1ull << max2k())) [[unlikely]]
		{
			// new slab
			area* slab     = slab_new();
			s.l            = link_fn::list_push_detached(s.l, slab);
			slab->h[1].u64 = 0;
		}
		pointer ret = reinterpret_cast<pointer>(
		    +s.l->data + s.l->h[1].u64 * element_size());
		s.l->h[1].u64 += 1ull << i;
		return ret;
	}

	void
	elem2k_del(uint32_t i, pointer p)
	{
		assert(Min2k <= i && i <= max2k());
		pointer ret                    = m_slabs[i - Min2k].re_elem;
		*reinterpret_cast<pointer*>(p) = ret;
		m_slabs[i - Min2k].re_elem     = p;
	}

	pointer
	elem_over_new(size_type elems)
	{
		static_assert(overflow_area::align() >= sizeof(uint32_t));
		size_type over_size = overflow_area::size_n(
		    elems * element_size() + overflow_area::align());
		overflow_area* a = reinterpret_cast<overflow_area*>(
		    pattern::overflow_allocate(over_size));
		m_slabOverflow = overflow_fn::dlist_push_detached(m_slabOverflow, a);
		*reinterpret_cast<uint32_t*>(+a->data) = over_size;
		return reinterpret_cast<pointer>(+a->data) + overflow_area::align();
	}
	void
	elem_over_del(pointer p)
	{
		overflow_area* a = reinterpret_cast<overflow_area*>(
		    p - (overflow_area::size_header() + overflow_area::align()));
		m_slabOverflow = overflow_fn::dlist_detach(m_slabOverflow, a);
		if constexpr(FactoryTraitNone<
		                 typename pattern::overflow_type, FactoryNoFree>)
		{
			pattern::overflow_deallocate(
			    reinterpret_cast<pointer>(a),
			    *reinterpret_cast<uint32_t*>(+a->data));
		}
	}
	void
	elem_over_release()
	{
		if constexpr(FactoryTraitNone<
		                 typename pattern::overflow_type, FactoryNoFree>)
		{
			// only free overflow if it is supported, otherwise memory is
			// wasted until released upstream
			for(overflow_area* a = m_slabOverflow; a != nullptr;)
			{
				overflow_area* anext = overflow_fn::get_next(a);
				pattern::overflow_deallocate(
				    reinterpret_cast<pointer>(a),
				    *reinterpret_cast<uint32_t*>(+a->data));
				a = anext;
			}
		}
		m_slabOverflow = nullptr;
	}

protected:
	area* m_slabReuse             = nullptr;
	overflow_area* m_slabOverflow = nullptr;
	[[no_unique_address]] size_set m_slabSet;
	[[no_unique_address]] ReshapeSlice2kCache<
	    size_set, upstream_factory, Max2k> m_slab2kMax
	    = {};
	std::array<Slab2k, Max2k - Min2k + 1> m_slabs = {};
};

template<
    typename T, SlabFactory Upstream, ByteFactory Overflow = void_factory,
    size_t Max2k = 16, size_t Min2k = 4>
using slice_array_factory_type = slice_array_factory<
    Upstream, Overflow, Max2k, Min2k, sizeof(T), alignof(T)>;

} // namespace warthog::memory::alloc

#endif // WARTHOG_MEMORY_ALLOC_SLICE_ARRAY_FACTORY_H
