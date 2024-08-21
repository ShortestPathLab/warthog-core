#include <warthog/util/cost_table.h>

#include <cmath>

namespace warthog::util
{

cost_table::cost_table(const char* filename) : cost_table()
{
	std::ifstream file(filename, std::fstream::in);
	if(!file.is_open())
	{
		std::cerr << "err; cost_table::cost_table "
		             "cannot open costs file: "
		          << filename << std::endl;
		exit(1);
	}

	while(!file.eof())
	{
		uint8_t terrain;
		warthog::cost_t cost;
		file >> terrain >> cost;
		if(!file.good())
		{
			std::cerr << "err; cost_table::cost_table "
			             "failed to parse cost for terrain `"
			          << terrain << "`" << std::endl;
			exit(1);
		}
		if(costs_[terrain] == costs_[terrain])
		{
			std::cerr
			    << "err; cost_table::cost_table "
			       "costs file contains multiple definitions for terrain `"
			    << terrain << "`" << std::endl;
			exit(1);
		}
		if(cost < 0.0)
		{
			std::cerr << "err; cost_table::cost_table "
			             "costs file specifies a negative cost for terrain `"
			          << terrain << "`" << std::endl;
			exit(1);
		}
		costs_[terrain] = cost;
		file >> std::ws;
	}

	file.close();
}

// Identifies the cost of the lowest-cost terrain on the specified map.
// If the map contains terrain to which no cost has been assigned, then NaN is
// returned.
cost_t
cost_table::lowest_cost(domain::vl_gridmap& map)
{
	warthog::cost_t lowest = INFINITY;
	for(uint32_t id = 0; id < map.width() * map.height(); id++)
	{
		auto cost = costs_[map.get_label(id)];
		if(std::isnan(cost))
		{
			// return NaN if any terrain cost is NaN
			return cost;
		}
		if(cost != 0 && cost < lowest) { lowest = cost; }
	}
	return lowest;
}

} // namespace warthog::util
