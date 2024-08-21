#include <warthog/search/problem_instance.h>

std::ostream&
operator<<(std::ostream& str, warthog::search::problem_instance& pi)
{
	pi.print(str);

	return str;
}
