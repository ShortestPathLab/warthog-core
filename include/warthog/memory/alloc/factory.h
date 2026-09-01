#ifndef WARTHOG_MEMORY_ALLOC_FACTORY_H
#define WARTHOG_MEMORY_ALLOC_FACTORY_H

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

#include <concepts>
#include <cstddef>

namespace warthog::memory::alloc
{

enum factory_traits : uint32_t
{
	FactoryDefault = 0,
	FactorySource  = 1 << 0, /// allocations with no upstream (i.e. malloc)
	FactoryOwn     = 1 << 1, /// owns and will release memory
	FactoryReuse   = 1 << 2, /// factory will attempt to reuse memory
	FactoryNoFree
	= 1 << 3, /// factory has deallocate or destroy, but it does nothing
	FactoryPointer = 1 << 4, /// factory is a pointer to another factory
};

namespace details
{

template<typename Fact>
concept Factory_base = requires(Fact f) {
	// not movable or copyable
	requires std::default_initializable<Fact>;
	requires !std::movable<Fact>;
	requires !std::copyable<Fact>;

	// required types
	typename Fact::value_type;
	typename Fact::pointer;
	typename Fact::size_type;

	// required functions, can be static or member
	{ f.alignment() } -> std::same_as<typename Fact::size_type>;
	{ f.element_size() } -> std::same_as<typename Fact::size_type>;
	{ Fact::traits() } -> std::same_as<uint32_t>;

	requires((Fact::traits() & FactorySource) != 0) || requires {
		{ f.upstream() };
	};
};

/**
 * Class is a array factory.  Provides allocation of array's.
 */
template<typename Fact>
concept ArrayFactory_base = details::Factory_base<Fact> && requires(Fact f) {
	requires requires(
	    typename Fact::pointer ptr, typename Fact::size_type elem) {
		// array allocation
		{ f.allocate(elem) } -> std::same_as<typename Fact::pointer>;
		{ f.deallocate(ptr, elem) };
	};
};

/**
 * Class is a single element factory.  Provides allocation of single elements.
 */
template<typename Fact>
concept SingleFactory_base = details::Factory_base<Fact> && requires(Fact f) {
	requires requires(typename Fact::pointer ptr) {
		// single allocation
		{ f.create() } -> std::same_as<typename Fact::pointer>;
		{ f.destroy(ptr) };
	};
};

} // namespace details

class void_factory
{
public:
	using value_type = std::byte;
	using pointer    = value_type*;
	using size_type  = size_t;

	static consteval uint32_t
	traits() noexcept
	{
		return FactorySource | FactoryNoFree;
	}

	constexpr void_factory() noexcept = default;
	void_factory(const void_factory&) = delete;
	void_factory
	operator=(const void_factory&)
	    = delete;

	constexpr bool
	setup() noexcept
	{
		return true;
	}

	static consteval size_type
	alignment() noexcept
	{
		return alignof(max_align_t);
	}
	static consteval size_type
	element_size() noexcept
	{
		return 1;
	}

	constexpr pointer
	allocate(size_type)
	{
		return {};
	}
	constexpr void
	deallocate(pointer)
	{ }
	constexpr void
	deallocate(pointer, size_type)
	{ }
};

/**
 * Class is a array factory.  Provides allocation of array's.
 */
template<typename Fact>
concept ArrayFactory
    = details::ArrayFactory_base<Fact> && !details::SingleFactory_base<Fact>;

/**
 * Class is a single element factory.  Provides allocation of single elements.
 */
template<typename Fact>
concept SingleFactory
    = details::SingleFactory_base<Fact> && !details::ArrayFactory_base<Fact>;

/**
 * Class is a factory.  Designed to provide flexible memory generation.
 */
template<typename Fact>
concept Factory = ArrayFactory<Fact> || SingleFactory<Fact>;

template<typename Fact, uint32_t T>
concept FactoryTraitAll = Factory<Fact> && (Fact::traits() & T) == T;
template<typename Fact, uint32_t T>
concept FactoryTraitAny = Factory<Fact> && (Fact::traits() & T) != 0;
template<typename Fact, uint32_t T>
concept FactoryTraitNone = Factory<Fact> && (Fact::traits() & T) == 0;

// setup or destructor calls release if it has one, free_upstream is set to
// false iff: upstream is FactoryNoFree OR (upstream is FactoryOwn AND upstream
// is not FactoryPointer)
template<Factory Fact>
inline constexpr bool factory_chain_free_release
    = FactoryTraitNone<Fact, FactoryNoFree>
    || (FactoryTraitNone<Fact, FactoryOwn>
        && FactoryTraitAny<Fact, FactoryPointer>);

/**
 * Class is a byte factory.  Provides allocation for byte object.
 */
template<typename Fact>
concept ByteFactory = ArrayFactory<Fact> && requires(Fact f) {
	requires std::same_as<typename Fact::value_type, std::byte>;
	requires Fact::alignment() == alignof(max_align_t);
	requires Fact::element_size() == 1;
};

/**
 * A byte factory that supports dynamic align allocation.
 */
template<typename Fact>
concept AlignByteFactory = ByteFactory<Fact>
    && requires(Fact f, Fact::pointer ptr, Fact::size_type elem) {
	       { f.allocate(elem, elem) } -> std::same_as<typename Fact::pointer>;
	       { f.deallocate(ptr, elem, elem) };
       };

/**
 * Class is a factory.  Designed to provide flexible memory generation.
 */
template<typename Fact>
concept FreeArrayFactory
    = ArrayFactory<Fact> && requires(Fact f, typename Fact::pointer ptr) {
	      { f.deallocate(ptr) };
      };

template<typename Fact>
concept VoidFactory = std::same_as<Fact, void_factory>;

static_assert(
    ByteFactory<void_factory> && VoidFactory<void_factory>
        && FreeArrayFactory<void_factory>,
    "void_factory must be a valid ByteFactory");

/**
 * Class has memory release functions.
 */
template<typename Fact>
concept ReleaseFactory = Factory<Fact> && requires(Fact f, bool b) {
	{ f.release() };
	{ f.release(b) };
};

/**
 * Class has memory reclaim functions.
 */
template<typename Fact>
concept ReclaimFactory = ReleaseFactory<Fact> && requires(Fact f) {
	{ f.reclaim() };
};

/// @brief Get factory upstream
/// @param F factory
/// @return F.upstream()
Factory auto&
upstream(Factory auto& F) noexcept
{
	return F.upstream();
}

/// @brief Gets a upstream factory matching Fact type
/// @param F factory
/// @return Factory of type Fact, const auto deduced based on F
template<Factory Fact>
Factory auto&
upstream(Factory auto& F) noexcept
{
	Factory auto& up = F.upstream();
	if constexpr(std::same_as<std::remove_cvref_t<decltype(up)>, Fact>)
	{
		return up;
	}
	else { return upstream<Fact>(up); }
}

} // namespace warthog::memory::alloc

#endif // WARTHOG_MEMORY_ALLOC_FACTORY_ARRAY_H
