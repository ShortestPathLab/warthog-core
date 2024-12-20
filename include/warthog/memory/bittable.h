#ifndef WARTHOG_MEMORY_BITTABLE_H
#define WARTHOG_MEMORY_BITTABLE_H

#include <cstring>
#include <cstdint>
#include <concepts>
#include <bit>
#include <span>
#include <warthog/constants.h>

namespace warthog::memory
{

namespace details {
struct bittable_Dimension
{
	uint32_t width;
	uint32_t height;
};
} // namespace details

/**
 * IdType: should be Identity, although integer is allowed.
 * ValueBits: size of bits stored for each value. Must be power of 2 and smaller than BaseType.
 */
template <typename IdType, std::unsigned_integral BaseType, size_t ValueBits = 1>
class bittable
{
public:
	static_assert(ValueBits <= sizeof(BaseType) * CHAR_BIT && std::popcount(ValueBits) == 1, "ValueBits must be to power of 2 and fit inside BaseType bits.");
	using value_type = BaseType;
	using id_value_type = std::conditional_t<sizeof(IdType) <= 4, uint32_t, uint64_t>;
	constexpr static size_t base_bit_width = std::bit_width(sizeof(BaseType) * CHAR_BIT - 1);
	constexpr static size_t value_bit_width = std::bit_width(ValueBits - 1);
	constexpr static id_value_type base_bit_mask = id_value_type{(uint64_t{1} << base_bit_width) - 1};
	constexpr static BaseType value_bit_mask = BaseType{(uint64_t{1} << ValueBits) - 1};

	static constexpr size_t calc_array_size(uint32_t width, uint32_t height) noexcept
	{
		const size_t total_bits = static_cast<size_t>(width) * height * ValueBits;
		return (total_bits + ((1u << base_bit_width) - 1)) >> base_bit_width;
	}

	constexpr bittable() noexcept : m_data{}, m_dim{}
	{ }

	constexpr void setup(value_type* ptr, uint32_t width, uint32_t height) noexcept
	{
		m_data = ptr;
		m_dim.width = width;
		m_dim.height = height;
	}

	constexpr IdType
	xy_to_id(uint32_t x, uint32_t y) const noexcept
	{
		assert(m_dim.width != 0);
		return {static_cast<size_t>(y) * m_dim.width + static_cast<size_t>(x)};
	}

	constexpr std::pair<uint32_t, uint32_t>
	id_to_xy(IdType id) const noexcept
	{
		assert(m_dim.width != 0);
		const id_value_type value = static_cast<id_value_type>(id);
		return {static_cast<uint32_t>(value % m_dim.width), static_cast<uint32_t>(value / m_dim.width)};
	}

	constexpr value_type* data() noexcept
	{
		return m_data;
	}
	constexpr const value_type* data() const noexcept
	{
		return m_data;
	}
	/// data return within span. size will be calculated with data_elements().
	constexpr std::span<value_type> data_span() noexcept
	{
		return {m_data, data_elements()};
	}
	/// data return within span. size will be calculated with data_elements().
	constexpr std::span<const value_type> data_span() const noexcept
	{
		return {m_data, data_elements()};
	}

	constexpr uint32_t width() const noexcept
	{
		return m_dim.width;
	}
	constexpr uint32_t height() const noexcept
	{
		return m_dim.height;
	}
	constexpr uint64_t size() const noexcept
	{
		return static_cast<uint64_t>(m_dim.width) * m_dim.height;
	}
	
	constexpr size_t data_elements() const noexcept
	{
		return calc_array_size(m_dim.width, m_dim.height);
	}

	constexpr void fill(value_type value) noexcept
	{
		assert(value <= value_bit_mask);
		if constexpr (ValueBits <= CHAR_BIT) {
			for (int i = ValueBits; i < CHAR_BIT; i <<= 1) {
				value = (value << i) | value;
			}
			std::memset(m_data, value, data_elements());
		} else {
			for (int i = ValueBits; i < sizeof(BaseType) * CHAR_BIT; i <<= 1) {
				value = (value << i) | value;
			}
			std::fill_n(m_data, data_elements(), value);
		}
	}
	constexpr void flip() noexcept
	{
		for (value_type *p = m_data, *pe = p + data_elements(); p != pe; ++p) {
			*p = ~*p;
		}
	}

	constexpr void set(IdType id, value_type value) noexcept
	{
		assert(uint64_t{id} < size());
		assert(value <= value_bit_mask);
		id_value_type idval = id_value_type{id} << value_bit_width;
		BaseType* data_pos = m_data + (idval >> base_bit_width);
		*data_pos = (*data_pos & (~value_bit_mask << (idval & base_bit_mask))) | (value << (idval & base_bit_mask));
	}
	constexpr void bit_or(IdType id, value_type value) noexcept
	{
		assert(uint64_t{id} < size());
		assert(value <= value_bit_mask);
		id_value_type idval = id_value_type{id} << value_bit_width;
		BaseType* data_pos = m_data + (idval >> base_bit_width);
		*data_pos |= value << (idval & base_bit_mask);
	}
	constexpr void bit_and(IdType id, value_type value) noexcept
	{
		assert(uint64_t{id} < size());
		assert(value <= value_bit_mask);
		id_value_type idval = id_value_type{id} << value_bit_width;
		BaseType* data_pos = m_data + (idval >> base_bit_width);
		*data_pos &= value << (idval & base_bit_mask);
	}
	constexpr void bit_xor(IdType id, value_type value) noexcept
	{
		assert(uint64_t{id} < size());
		assert(value <= value_bit_mask);
		id_value_type idval = id_value_type{id} << value_bit_width;
		BaseType* data_pos = m_data + (idval >> base_bit_width);
		*data_pos ^= value << (idval & base_bit_mask);
	}

	constexpr value_type get(IdType id) const noexcept
	{
		assert(uint64_t{id} < size());
		id_value_type idval = id_value_type{id} << value_bit_width;
		BaseType* data_pos = m_data + (idval >> base_bit_width);
		return (*data_pos >> (idval & base_bit_mask)) & value_bit_mask;
	}

	// return id position split
	// `m_data[first] >> second` will return the value as position id
	constexpr std::pair<uint32_t, uint32_t> id_split(IdType id) const noexcept
	{
		id_value_type idval = id_value_type{id} << value_bit_width;
		return {static_cast<uint32_t>(idval >> base_bit_width), static_cast<uint32_t>(idval & base_bit_mask)};
	}

protected:
	BaseType* m_data;
	details::bittable_Dimension m_dim;
};

} // namespace warthog::memory

#endif // WARTHOG_MEMORY_ARRAYLIST_H
