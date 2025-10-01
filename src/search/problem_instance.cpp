#include <warthog/search/problem_instance.h>

namespace warthog::search
{

std::ostream&
operator<<(std::ostream& str, const problem_instance& pi)
{
	pi.print(str);

	return str;
}

std::ostream&
operator<<(
    std::ostream& str, const search_problem_instance& pi)
{
	pi.print(str);

	return str;
}

}
