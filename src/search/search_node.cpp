#include <warthog/search/search_node.h>

namespace warthog::search {

void
search_node::print(std::ostream& out) const
{
	out << "search_node id:" << get_id().id;
	out << " p_id: ";
	out << parent_id_.id;
	out << " g: " << g_ << " f: " << this->get_f() << " ub: " << ub_
		<< " expanded: " << get_expanded() << " "
		<< " search_number_: " << search_number_;
}

std::ostream&
operator<<(std::ostream& str, const warthog::search::search_node& sn)
{
	sn.print(str);

	return str;
}

} // namespace warthog::search
