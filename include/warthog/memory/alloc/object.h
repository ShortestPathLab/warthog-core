#ifndef WARTHOG_MEMORY_ALLOC_OBJECT_H
#define WARTHOG_MEMORY_ALLOC_OBJECT_H

/*
MIT License

Copyright (c) 2024 Ryan Hechenberger

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

#include <bit>
#include <memory>

namespace warthog::memory::alloc
{

template<size_t Size, size_t Align>
struct alignas(Align) object_bytes_size : std::array<std::byte, Size>
{ };
template<typename T>
using object_bytes = object_bytes_size<sizeof(T), alignof(T)>;

constexpr size_t
pad_alignment(size_t size, size_t align)
{
	assert(align != 0 && std::popcount(align) == 1);
	// [[assume(numeric::popcount(align) == 1)]];
	return size + ((align - (size & (align - 1))) & ~align);
}
template<typename AlignTo, typename AlignFrom = void>
constexpr size_t
pad_type_alignment(size_t size)
{
	if constexpr(std::same_as<AlignFrom, void>)
	{
		return pad_alignment(size, alignof(AlignTo));
	}
	else if constexpr(alignof(AlignTo) == alignof(AlignFrom))
	{
		assert(size == pad_alignment(size, alignof(AlignFrom)));
		return size;
	}
	else
	{
		return pad_alignment(size, alignof(AlignTo));
	}
}

#define WARTHOG_VAR_STRUCT(type, member)                                      \
	(::warthog::memory::alloc::pad_type_alignment<type>(                      \
	    offsetof(type, member)))
#define WARTHOG_VAR_STRUCT_SIZE(type, member, size)                           \
	WARTHOG_VAR_STRUCT_DYNAMIC_SIZE(type, member, size)
#define WARTHOG_VAR_STRUCT_DYNAMIC_SIZE(type, member, size)                   \
	(::warthog::memory::alloc::pad_type_alignment<                            \
	    type, decltype(type::member)>(                                        \
	    offsetof(type, member[0]) + sizeof(decltype(type::member)) * size))
#define WARTHOG_VAR_STRUCT_TYPE(type, member, member_type)                    \
	(::warthog::memory::alloc::pad_type_alignment<type, member_type>(         \
	    offsetof(type, member)))

/**
 * Operates same as std::align, except if returned an adjusted value, will
 * adjust ptr and space by size.
 */
inline void*
align_adjust(
    std::size_t alignment, std::size_t size, void*& ptr, std::size_t& space)
{
	void* p = std::align(alignment, size, ptr, space);
	if(p)
	{
		ptr    = static_cast<void*>(static_cast<std::byte*>(ptr) + size);
		space -= size;
	}
	return p;
}

} // namespace warthog::memory::alloc

#endif // WARTHOG_MEMORY_ALLOC_OBJECT_H
