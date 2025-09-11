#include <warthog/io/stream_observer.h>
#include <warthog/io/posthoc_trace.h>
#include <exception>
#include <fstream>
#include <iostream>

namespace warthog::io
{

stream_observer::stream_observer(const std::filesystem::path& filename, std::ios_base::openmode mode)
{
	shared_stream_ = std::make_shared<std::ofstream>(filename, mode);
	stream_ = shared_stream_.get();
}
stream_observer::stream_observer(std::ostream& stream)
{
	stream_ = &stream;
}
stream_observer::stream_observer(const shared_stream_t& stream)
{
	shared_stream_ = stream;
	stream_ = shared_stream_.get();
}
stream_observer::~stream_observer() = default;

void stream_observer::stream_open(const std::filesystem::path& filename, std::ios_base::openmode mode)
{
	shared_stream_ = std::make_shared<std::ofstream>(filename, mode);
	stream_ = shared_stream_.get();
}
void stream_observer::stream(std::ostream& stream)
{
	stream_ = &stream;
	shared_stream_ = nullptr;
}
void stream_observer::stream_share(const shared_stream_t& stream)
{
	shared_stream_ = stream;
	stream_ = shared_stream_.get();
}
void stream_observer::stream_share(const stream_observer& stream)
{
	shared_stream_ = stream.shared_stream_;
	stream_ = stream.stream_;
}
void stream_observer::stream_stdout()
{
	shared_stream_ = nullptr;
	stream_ = &std::cout;
}
void stream_observer::stream_stderr()
{
	shared_stream_ = nullptr;
	stream_ = &std::cerr;
}
void stream_observer::clear_stream()
{
	shared_stream_ = nullptr;
	stream_ = nullptr;
}

void posthoc_trace::print_posthoc_header()
{
	if (*this) {
		stream() << R"posthoc(version: 1.4.0
events:
)posthoc";
	}
}

} // namespace warthog::io
