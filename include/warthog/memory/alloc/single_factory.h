#ifndef WARTHOG_MEMORY_ALLOC_SINGLE_FACTORY_H
#define WARTHOG_MEMORY_ALLOC_SINGLE_FACTORY_H

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

#include <cstring>
#include <memory>

namespace warthog::memory::alloc
{

/**
 * @brief Converts an ArrayFactory into a SingleFactory allocator
 * @tparam Upstream upstream factory to pull allocations from
 * @tparam Elems number of upstream elements to use for single create(), for
 * ByteFactory will be size of object.
 * @tparam Alignment alignment of object to create.
 *
 * Converts an allocate() syntax into a create() for single elements.
 * Pass 0,0 for Elems and Alignment to create a dynamic (run-time) setting of
 * Elems and Alignment, params set through setup().
 */
template<
    ArrayFactory Upstream, size_t Elems,
    size_t Alignment = alignof(max_align_t)>
class single_factory : private Upstream
{
	using size_set = dynamic_size<Elems, Alignment>;

public:
	static_assert(
	    Alignment != 0 || (Elems == 0 && Alignment == 0),
	    "Alignment can only be 0 if elements are 0 (for dynamic).");
	using upstream_factory = Upstream;
	using typename Upstream::pointer;
	using typename Upstream::size_type;
	using typename Upstream::value_type;

	static consteval uint32_t
	traits() noexcept
	{ return FactoryDefault; }

	constexpr size_type
	alignment() const noexcept
	{ return m_size.align(); }
	constexpr size_type
	element_size() const noexcept
	{ return Upstream::element_size() * m_size.elements(); }

	/// @brief setup(...)
	template<typename... T>
	constexpr bool
	setup(T&&... args)
	    requires(!size_set::dynamic)
	{
		if(!Upstream::setup(std::forward<T>(args)...)) return false;
		return m_size.set(upstream());
	}
	template<typename... T>
	constexpr bool
	setup(dynamic_factory_params param, T&&... args)
	    requires(size_set::dynamic)
	{
		if(!Upstream::setup(std::forward<T>(args)...)) return false;
		return m_size.set(upstream(), param.elems, param.align);
	}

	[[nodiscard]] pointer
	create()
	{
		if constexpr(AlignByteFactory<Upstream>)
		{
			return Upstream::allocate(element_size(), alignment());
		}
		else
		{
			return Upstream::allocate(element_size());
		}
	}
	void
	destroy(pointer ptr)
	{
		if constexpr(AlignByteFactory<Upstream>)
		{
			return Upstream::deallocate(ptr, element_size(), alignment());
		}
		else
		{
			return Upstream::deallocate(ptr, element_size());
		}
	}

	constexpr upstream_factory&
	upstream() noexcept
	{ return static_cast<upstream_factory&>(*this); }
	constexpr const upstream_factory&
	upstream() const noexcept
	{ return static_cast<const upstream_factory&>(*this); }

private:
	[[no_unique_address]] size_set m_size;
};
template<ByteFactory Upstream, typename T>
using single_factory_type = single_factory<Upstream, sizeof(T), alignof(T)>;

} // namespace warthog::memory::alloc

#endif // WARTHOG_MEMORY_ALLOC_SINGLE_FACTORY_H
