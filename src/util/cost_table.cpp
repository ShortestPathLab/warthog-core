#include <warthog/util/cost_table.h>

#include <warthog/io/log.h>

#include <cmath>
#include <limits>

namespace warthog::util
{

cost_table::cost_table(const char* filename) : cost_table()
{
	std::ifstream file(filename, std::fstream::in);
	if(!file.is_open())
	{
		WARTHOG_GERROR_FMT("cost_table cannot open costs file {}", filename);
		throw std::runtime_error("cost_table");
	}

	while(!file.eof())
	{
		uint8_t terrain;
		warthog::cost_t cost;
		file >> terrain >> cost;
		if(!file.good())
		{
			WARTHOG_GERROR_FMT("cost_table failed to parse cost for terrain `{}`", terrain);
			throw std::runtime_error("cost_table");
		}
		if(costs_[terrain] == costs_[terrain])
		{
			WARTHOG_GERROR_FMT("cost_table multiple definitions for terrain `{}`", terrain);
			throw std::runtime_error("cost_table");
		}
		if(cost < 0.0)
		{
			WARTHOG_GERROR_FMT("cost_table has negative cost for terrain `{}` at {}", terrain, cost);
			throw std::runtime_error("cost_table");
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
	warthog::cost_t lowest = std::numeric_limits<warthog::cost_t>::infinity();
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
