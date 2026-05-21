#include <warthog/util/experiment.h>

#include <iomanip>

namespace warthog::util
{

void
experiment::print(std::ostream& out)
{
	out << this->map() << "\t";
	out << this->mapwidth() << "\t";
	out << this->mapheight() << "\t";
	out << this->startx() << "\t";
	out << this->starty() << "\t";
	out << this->goalx() << "\t";
	out << this->goaly() << "\t";
	if (this->distance())
		out << std::setprecision(10) << *this->distance();
	else
		out << '-';
}

} // namespace warthog::util
