#include <warthog/util/helpers.h>

#include <warthog/scenario/scenario_manager.h>
#include <warthog/search/search.h>
#include <warthog/search/solution.h>

#include <cstdint>
#include <fstream>
#include <iostream>

namespace warthog::util
{

bool
load_integer_labels(const char* filename, std::vector<uint32_t>& labels)
{
	std::ifstream ifs(filename, std::ios_base::in);
	if(!ifs.good())
	{
		std::cerr << "\nerror trying to load file " << filename << std::endl;
		ifs.close();
		return false;
	}

	while(true)
	{
		// skip comment lines
		while(ifs.peek() == '#' || ifs.peek() == 'c' || ifs.peek() == '%')
		{
			while(ifs.get() != '\n')
				;
		}

		uint32_t tmp;
		ifs >> tmp;
		if(!ifs.good()) { break; }
		labels.push_back(tmp);
	}
	ifs.close();
	return true;
}

bool
load_integer_labels_dimacs(const char* filename, std::vector<uint32_t>& labels)
{
	// add a dummy to the front of the list if the labels are for use with
	// a dimacs graph. Such graphs use a 1-indexed scheme for node ids. we
	// add the dummy so we can use the dimacs ids without conversion
	labels.push_back(0);
	return load_integer_labels(filename, labels);
}

void
value_index_swap_array(std::vector<uint32_t>& vec)
{
	// re-maps @param vec s.t. for each x and i
	// v[i] = x becomes v[x] = i
	std::vector<uint32_t> tmp;
	tmp.resize(vec.size());
	for(uint32_t i = 0; i < vec.size(); i++)
	{
		tmp.at(i) = vec.at(i);
	}

	assert((*std::min_element(tmp.begin(), tmp.end())) == 0);
	assert((*std::max_element(tmp.begin(), tmp.end())) == vec.size() - 1);
	for(uint32_t i = 0; i < tmp.size(); i++)
	{
		vec.at(tmp.at(i)) = i;
	}
}

} // namespace warthog::util
