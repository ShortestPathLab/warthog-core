#include <warthog/search/solution.h>

std::ostream&
operator<<(std::ostream& str, warthog::search::solution& sol)
{
	sol.print_path(str);
	str << std::endl;
	sol.print_metrics(str);
	str << std::endl;
	return str;
}
