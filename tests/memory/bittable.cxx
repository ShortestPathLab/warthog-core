#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_template_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>
#include <catch2/generators/catch_generators_adapters.hpp>
#include <catch2/generators/catch_generators_random.hpp>
#include <memory>
#include <random>
#include <algorithm>
#include <iostream>
#include <vector>
#include <cstdint>
#include <warthog/memory/bittable.h>

namespace {

template <typename bt, typename vec>
bool test_bittable_vector(const bt& b, const vec& v)
{
	for (size_t y = 0, i = 0; y < b.height(); ++y)
	for (size_t x = 0; x < b.width(); ++x, ++i) {
		if (b.xy_to_id(x, y) != i)
			return false;
		if (b.get(i) != v[y][x])
			return false;
	}
	return true;
}

}

TEMPLATE_TEST_CASE_SIG("bittable random testing", "[bittable][rng]",
	((typename T, size_t B), T, B),
	(uint8_t,1), (uint8_t,2), (uint8_t,8), (uint32_t,1), (uint32_t,16), (uint64_t,32), (uint64_t,64)
)
{
	using bittable = warthog::memory::bittable<uint32_t, T, B>;
	auto width = GENERATE(1,128,1000);
	auto height = GENERATE(1,128,1000);

	size_t element_count = bittable::calc_array_size(width, height);
	REQUIRE(element_count * sizeof(T) * CHAR_BIT >= width * height * B);
	std::random_device rng_dev;
	std::ranlux48_base rng(rng_dev());
	std::vector<T> element_data(element_count, 0);
	bittable table;
	table.setup(element_data.data(), (uint32_t)width, (uint32_t)height);

	constexpr T value_zero = 0;
	constexpr T value_one_word = static_cast<T>(~0ull);
	// done with lambda to silence erroneous -Wshift-count-overflow
	constexpr T value_one = [](){ if constexpr (B >= sizeof(T) * CHAR_BIT) {
		return value_one_word;
	 } else {
		return static_cast<T>(~(value_one_word << B));
	 }}();
	constexpr T value_alt_word = static_cast<T>(0xAAAA'AAAA'AAAA'AAAAull);
	constexpr T value_alt = value_alt_word & value_one;

	SECTION("WHOLE")
	{
		std::generate(element_data.begin(), element_data.end(), std::ref(rng));
		table.fill(value_zero);
		REQUIRE( std::all_of(element_data.begin(), element_data.end(), [](T v)noexcept { return v == value_zero; }) );
		table.flip();
		REQUIRE( std::all_of(element_data.begin(), element_data.end(), [](T v)noexcept { return v == value_one_word; }) );
		
		std::generate(element_data.begin(), element_data.end(), std::ref(rng));
		table.fill(value_one);
		REQUIRE( std::all_of(element_data.begin(), element_data.end(), [](T v)noexcept { return v == value_one_word; }) );
		table.flip();
		REQUIRE( std::all_of(element_data.begin(), element_data.end(), [](T v)noexcept { return v == value_zero; }) );
		
		if constexpr (B > 1) {
			std::generate(element_data.begin(), element_data.end(), std::ref(rng));
			table.fill(value_alt);
			REQUIRE( std::all_of(element_data.begin(), element_data.end(), [](T v)noexcept { return v == value_alt_word; }) );
			table.flip();
			REQUIRE( std::all_of(element_data.begin(), element_data.end(), [](T v)noexcept { return v == (T)~value_alt_word; }) );
		}
	}

	std::ranlux48_base rng_seed(GENERATE(random()));
	std::uniform_int_distribution<T> rng_value(0, bittable::value_bit_mask);
	std::vector<std::vector<T>> valuetable(height, std::vector<T>(width, 0));
	{
	for (size_t y = 0; y < table.height(); ++y)
	for (size_t x = 0; x < table.width(); ++x) {
		valuetable[y][x] = rng_value(rng_seed);
	}
	}

	SECTION("OPERATORS")
	{
		for (size_t y = 0; y < table.height(); ++y)
		for (size_t x = 0; x < table.width(); ++x) {
			table.set(table.xy_to_id(x, y), valuetable[y][x]);
		}
		CHECK( test_bittable_vector(table, valuetable) );

		auto op = GENERATE(0,1,2,3,4);

		std::uniform_int_distribution<int> rng_op(0, 3);
		std::uniform_int_distribution<uint32_t> rng_width(0, width-1);
		std::uniform_int_distribution<uint32_t> rng_height(0, height-1);

		for (size_t i = 0, ie = width * height; i < ie; ++i) {
			auto x = rng_width(rng_seed);
			auto y = rng_height(rng_seed);
			auto value = rng_value(rng_seed);
			uint32_t pos = table.xy_to_id(x, y);
			switch (op) {
			case 0:
				table.bit_and(pos, value);
				valuetable[y][x] &= value;
				break;
			case 1:
				table.bit_or(pos, value);
				valuetable[y][x] |= value;
				break;
			case 2:
				table.bit_xor(pos, value);
				valuetable[y][x] ^= value;
				break;
			default:
				table.bit_neg(pos);
				valuetable[y][x] = ~valuetable[y][x] & bittable::value_bit_mask;
				break;
			}
		}
		CHECK( test_bittable_vector(table, valuetable) );
	}
}
