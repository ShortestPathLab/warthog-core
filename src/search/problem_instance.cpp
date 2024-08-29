#include <warthog/search/problem_instance.h>

std::ostream&
operator<<(std::ostream& str, const warthog::search::problem_instance& pi)
{
	pi.print(str);

	return str;
}

std::ostream&
operator<<(
    std::ostream& str, const warthog::search::search_problem_instance& pi)
{
	pi.print(str);

	return str;
}
