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

TEMPLATE_TEST_CASE_SIG("bittable random testing", "[bittable][rng]",
	((typename T, size_t B), T, B),
	(uint8_t,1), (uint8_t,2), (uint8_t,8), (uint32_t,1), (uint32_t,16)
)
{
	using bittable = warthog::memory::bittable<uint32_t, T, B>;
	auto width = GENERATE(1,128,1000);
	auto height = GENERATE(1,128,1000);

	size_t element_count = bittable::calc_array_size(width, height);
	REQUIRE(element_count > 0);
	std::random_device rng_dev;
	std::ranlux48_base rng(rng_dev());
	std::vector<T> element_data(element_count, 0);
	bittable table;
	table.setup(element_data.data(), (uint32_t)width, (uint32_t)height);

	constexpr T value_zero = 0;
	constexpr T value_one_word = static_cast<T>(~0ull);
	constexpr T value_one = static_cast<T>(~(value_one_word << B));
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

	// auto booltable = GENERATE_COPY(chunk(height, chunk(width, random(uint8_t{0}, uint8_t{1}))));

	// SECTION("PRINT")
	// {
	// }
}
