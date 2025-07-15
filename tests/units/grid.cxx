#include <warthog/domain/grid.h>
#include <catch2/catch_template_test_macros.hpp>
#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>
#include <catch2/generators/catch_generators_adapters.hpp>
#include <catch2/generators/catch_generators_random.hpp>

TEST_CASE( "grid utility tests", "[unit][grid]" ) {
	using namespace warthog::grid;
	// const cardinals in clockwise order
	constexpr std::array<direction,4> Cardinal{NORTH, EAST, SOUTH, WEST};
	constexpr std::array<direction,4> ICardinal{NORTHEAST, SOUTHEAST, SOUTHWEST, NORTHWEST};
	constexpr std::array<direction_id,4> CardinalID{NORTH_ID, EAST_ID, SOUTH_ID, WEST_ID};
	constexpr std::array<direction_id,4> ICardinalID{NORTHEAST_ID, SOUTHEAST_ID, SOUTHWEST_ID, NORTHWEST_ID};
	
	SECTION( "cardinals and intercardinals checks and conversions" ) {
		// Cardinal and InterCardinals are checked correctly
		for (auto d : CardinalID) {
			CHECK(is_cardinal_id(d));
			CHECK_FALSE(is_intercardinal_id(d));
		}
		for (auto d : ICardinalID) {
			CHECK(is_intercardinal_id(d));
			CHECK_FALSE(is_cardinal_id(d));
		}

		// convert betweein direction and direction_id
		for (int i = 0; i < 4; ++i) {
			CHECK(to_dir(CardinalID[i]) == Cardinal[i]);
			CHECK(to_dir_id(Cardinal[i]) == CardinalID[i]);
			CHECK(to_dir(ICardinalID[i]) == ICardinal[i]);
			CHECK(to_dir_id(ICardinal[i]) == ICardinalID[i]);
		}
	}
	
	SECTION( "directions rotate" ) {
		// rotate directions correctly
		for (int i = 0; i < 4; ++i) {
			// CW
			CHECK(dir_id_cw(CardinalID[i]) == CardinalID[(i+1)%4]);
			CHECK(dir_id_cw(ICardinalID[i]) == ICardinalID[(i+1)%4]);
			CHECK(dir_cw(Cardinal[i]) == Cardinal[(i+1)%4]);
			CHECK(dir_cw(ICardinal[i]) == ICardinal[(i+1)%4]);

			// CCW
			CHECK(dir_id_ccw(CardinalID[i]) == CardinalID[(i+3)%4]);
			CHECK(dir_id_ccw(ICardinalID[i]) == ICardinalID[(i+3)%4]);
			CHECK(dir_ccw(Cardinal[i]) == Cardinal[(i+3)%4]);
			CHECK(dir_ccw(ICardinal[i]) == ICardinal[(i+3)%4]);

			// flip/180
			CHECK(dir_id_flip(CardinalID[i]) == CardinalID[(i+2)%4]);
			CHECK(dir_id_flip(ICardinalID[i]) == ICardinalID[(i+2)%4]);
			CHECK(dir_flip(Cardinal[i]) == Cardinal[(i+2)%4]);
			CHECK(dir_flip(ICardinal[i]) == ICardinal[(i+2)%4]);
		}
	}
	
	SECTION( "directions grid_id adjust" ) {
#define WART_DG_WIDTH 164
		// direction adjust
		CHECK(dir_id_adj(NORTH_ID, WART_DG_WIDTH) == -WART_DG_WIDTH);
		CHECK(dir_id_adj(EAST_ID, WART_DG_WIDTH) == 1);
		CHECK(dir_id_adj(SOUTH_ID, WART_DG_WIDTH) == WART_DG_WIDTH);
		CHECK(dir_id_adj(WEST_ID, WART_DG_WIDTH) == -1);
		CHECK(dir_id_adj(NORTHEAST_ID, WART_DG_WIDTH) == -WART_DG_WIDTH+1);
		CHECK(dir_id_adj(SOUTHEAST_ID, WART_DG_WIDTH) == WART_DG_WIDTH+1);
		CHECK(dir_id_adj(SOUTHWEST_ID, WART_DG_WIDTH) == WART_DG_WIDTH-1);
		CHECK(dir_id_adj(NORTHWEST_ID, WART_DG_WIDTH) == -WART_DG_WIDTH-1);
		
		// invert intercardinal results
		for (auto d : ICardinalID) {
			CHECK(dir_id_adj_inv_intercardinal(d, dir_id_adj(d, WART_DG_WIDTH)) == WART_DG_WIDTH);
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
		for (auto d : CardinalID) {
			CHECK(dir_id_adj_vert(d, WART_DG_WIDTH) + dir_id_adj_hori(d) == dir_id_adj(d, WART_DG_WIDTH));
		}
		for (auto d : ICardinalID) {
			CHECK(dir_id_adj_vert(d, WART_DG_WIDTH) + dir_id_adj_hori(d) == dir_id_adj(d, WART_DG_WIDTH));
		}
	}
}
