#include <catch2/catch_template_test_macros.hpp>
#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>
#include <catch2/generators/catch_generators_adapters.hpp>
#include <catch2/generators/catch_generators_random.hpp>
#include <warthog/domain/grid.h>

TEST_CASE("grid utility tests", "[unit][grid]")
{
	using namespace warthog::grid;
	// const cardinals in clockwise order
	constexpr std::array<direction, 4> Cardinal{NORTH, EAST, SOUTH, WEST};
	constexpr std::array<direction, 4> ICardinal{
	    NORTHEAST, SOUTHEAST, SOUTHWEST, NORTHWEST};
	constexpr std::array<direction_id, 4> CardinalID{
	    NORTH_ID, EAST_ID, SOUTH_ID, WEST_ID};
	constexpr std::array<direction_id, 4> ICardinalID{
	    NORTHEAST_ID, SOUTHEAST_ID, SOUTHWEST_ID, NORTHWEST_ID};

	SECTION("cardinals and intercardinals checks and conversions")
	{
		// Cardinal and InterCardinals are checked correctly
		for(auto d : CardinalID)
		{
			CHECK(is_cardinal_id(d));
			CHECK_FALSE(is_intercardinal_id(d));
		}
		for(auto d : ICardinalID)
		{
			CHECK(is_intercardinal_id(d));
			CHECK_FALSE(is_cardinal_id(d));
		}

		// convert betweein direction and direction_id
		for(int i = 0; i < 4; ++i)
		{
			CHECK(to_dir(CardinalID[i]) == Cardinal[i]);
			CHECK(to_dir_id(Cardinal[i]) == CardinalID[i]);
			CHECK(to_dir(ICardinalID[i]) == ICardinal[i]);
			CHECK(to_dir_id(ICardinal[i]) == ICardinalID[i]);
		}
	}

	SECTION("directions rotate")
	{
		// rotate directions correctly
		for(int i = 0; i < 4; ++i)
		{
			// CW 90
			CHECK(dir_id_cw90(CardinalID[i]) == CardinalID[(i + 1) % 4]);
			CHECK(dir_id_cw90(ICardinalID[i]) == ICardinalID[(i + 1) % 4]);
			CHECK(dir_cw90(Cardinal[i]) == Cardinal[(i + 1) % 4]);
			CHECK(dir_cw90(ICardinal[i]) == ICardinal[(i + 1) % 4]);

			// CCW 90
			CHECK(dir_id_ccw90(CardinalID[i]) == CardinalID[(i + 3) % 4]);
			CHECK(dir_id_ccw90(ICardinalID[i]) == ICardinalID[(i + 3) % 4]);
			CHECK(dir_ccw90(Cardinal[i]) == Cardinal[(i + 3) % 4]);
			CHECK(dir_ccw90(ICardinal[i]) == ICardinal[(i + 3) % 4]);

			// flip/180
			CHECK(dir_id_flip(CardinalID[i]) == CardinalID[(i + 2) % 4]);
			CHECK(dir_id_flip(ICardinalID[i]) == ICardinalID[(i + 2) % 4]);
			CHECK(dir_flip(Cardinal[i]) == Cardinal[(i + 2) % 4]);
			CHECK(dir_flip(ICardinal[i]) == ICardinal[(i + 2) % 4]);

			// CW 45
			CHECK(dir_id_cw45(CardinalID[i]) == ICardinalID[i]);
			CHECK(dir_id_cw45(ICardinalID[i]) == CardinalID[(i + 1) % 4]);
			CHECK(dir_cw45(Cardinal[i]) == ICardinal[i]);
			CHECK(dir_cw45(ICardinal[i]) == Cardinal[(i + 1) % 4]);

			// CCW 45
			CHECK(dir_id_ccw45(CardinalID[i]) == ICardinalID[(i + 3) % 4]);
			CHECK(dir_id_ccw45(ICardinalID[i]) == CardinalID[i]);
			CHECK(dir_ccw45(Cardinal[i]) == ICardinal[(i + 3) % 4]);
			CHECK(dir_ccw45(ICardinal[i]) == Cardinal[i]);
		}
	}

	SECTION("directions grid_id adjust")
	{
#define WART_DG_WIDTH 164
		// direction adjust
		CHECK(dir_id_adj(NORTH_ID, WART_DG_WIDTH) == -WART_DG_WIDTH);
		CHECK(dir_id_adj(EAST_ID, WART_DG_WIDTH) == 1);
		CHECK(dir_id_adj(SOUTH_ID, WART_DG_WIDTH) == WART_DG_WIDTH);
		CHECK(dir_id_adj(WEST_ID, WART_DG_WIDTH) == -1);
		CHECK(dir_id_adj(NORTHEAST_ID, WART_DG_WIDTH) == -WART_DG_WIDTH + 1);
		CHECK(dir_id_adj(SOUTHEAST_ID, WART_DG_WIDTH) == WART_DG_WIDTH + 1);
		CHECK(dir_id_adj(SOUTHWEST_ID, WART_DG_WIDTH) == WART_DG_WIDTH - 1);
		CHECK(dir_id_adj(NORTHWEST_ID, WART_DG_WIDTH) == -WART_DG_WIDTH - 1);

		// invert intercardinal results
		for(auto d : ICardinalID)
		{
			CHECK(
			    dir_id_adj_inv_intercardinal(d, dir_id_adj(d, WART_DG_WIDTH))
			    == WART_DG_WIDTH);
		}

		// vertical adjust
		CHECK(dir_id_adj_vert(NORTH_ID, WART_DG_WIDTH) == -WART_DG_WIDTH);
		CHECK(dir_id_adj_vert(EAST_ID, WART_DG_WIDTH) == 0);
		CHECK(dir_id_adj_vert(SOUTH_ID, WART_DG_WIDTH) == WART_DG_WIDTH);
		CHECK(dir_id_adj_vert(WEST_ID, WART_DG_WIDTH) == 0);
		CHECK(dir_id_adj_vert(NORTHEAST_ID, WART_DG_WIDTH) == -WART_DG_WIDTH);
		CHECK(dir_id_adj_vert(SOUTHEAST_ID, WART_DG_WIDTH) == WART_DG_WIDTH);
		CHECK(dir_id_adj_vert(SOUTHWEST_ID, WART_DG_WIDTH) == WART_DG_WIDTH);
		CHECK(dir_id_adj_vert(NORTHWEST_ID, WART_DG_WIDTH) == -WART_DG_WIDTH);

		// horizontal adjust
		CHECK(dir_id_adj_hori(NORTH_ID) == 0);
		CHECK(dir_id_adj_hori(EAST_ID) == 1);
		CHECK(dir_id_adj_hori(SOUTH_ID) == 0);
		CHECK(dir_id_adj_hori(WEST_ID) == -1);
		CHECK(dir_id_adj_hori(NORTHEAST_ID) == 1);
		CHECK(dir_id_adj_hori(SOUTHEAST_ID) == 1);
		CHECK(dir_id_adj_hori(SOUTHWEST_ID) == -1);
		CHECK(dir_id_adj_hori(NORTHWEST_ID) == -1);

		// check dir_id_adj = dir_id_adj_vert + dir_id_adj_hori
		for(auto d : CardinalID)
		{
			CHECK(
			    dir_id_adj_vert(d, WART_DG_WIDTH) + dir_id_adj_hori(d)
			    == dir_id_adj(d, WART_DG_WIDTH));
		}
		for(auto d : ICardinalID)
		{
			CHECK(
			    dir_id_adj_vert(d, WART_DG_WIDTH) + dir_id_adj_hori(d)
			    == dir_id_adj(d, WART_DG_WIDTH));
		}
	}

	SECTION("directions to points")
	{
		CHECK(dir_unit_point(NORTH_ID) == spoint(0, -1));
		CHECK(dir_unit_point(EAST_ID) == spoint(1, 0));
		CHECK(dir_unit_point(SOUTH_ID) == spoint(0, 1));
		CHECK(dir_unit_point(WEST_ID) == spoint(-1, 0));
		CHECK(dir_unit_point(NORTHEAST_ID) == spoint(1, -1));
		CHECK(dir_unit_point(SOUTHEAST_ID) == spoint(1, 1));
		CHECK(dir_unit_point(SOUTHWEST_ID) == spoint(-1, 1));
		CHECK(dir_unit_point(NORTHWEST_ID) == spoint(-1, -1));
	}

	SECTION("points to direction")
	{
		auto i = GENERATE(take(100, random(uint32_t(0), uint32_t(10'000))));
		auto j = GENERATE(take(100, random(uint32_t(0), uint32_t(10'000))));
		point a(
		    static_cast<uint16_t>(i % 100), static_cast<uint16_t>(i / 100));
		point b(
		    static_cast<uint16_t>(j % 100), static_cast<uint16_t>(j / 100));

		auto [dx, dy] = point_signed_diff(a, b);
		REQUIRE(dx == static_cast<int32_t>(b.x) - static_cast<int32_t>(a.x));
		REQUIRE(dy == static_cast<int32_t>(b.y) - static_cast<int32_t>(a.y));
		direction_id res = point_to_direction_id(a, b);
		REQUIRE(static_cast<uint32_t>(res) < 8);
		if(dx == 0)
		{
			// NORTH/SOUTH
			if(dy < 0)
				CHECK(res == NORTH_ID);
			else if(dy > 0)
				CHECK(res == SOUTH_ID);
			else
				CHECK(a == b);
		}
		else if(dy == 0)
		{
			// EAST/WEST
			if(dx < 0)
				CHECK(res == WEST_ID);
			else if(dx > 0)
				CHECK(res == EAST_ID);
			else
				CHECK(a == b); // should never reach
		}
		else if(dy < 0)
		{
			// NE/NW
			if(dx < 0)
				CHECK(res == NORTHWEST_ID);
			else
				CHECK(res == NORTHEAST_ID);
		}
		else
		{ // dy < 0
			// SE/SW
			if(dx < 0)
				CHECK(res == SOUTHWEST_ID);
			else
				CHECK(res == SOUTHEAST_ID);
		}
	}
}
