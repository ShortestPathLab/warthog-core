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
	out << std::fixed << std::setprecision((int)this->precision());
	out << this->distance();
}

} // namespace warthog::util
