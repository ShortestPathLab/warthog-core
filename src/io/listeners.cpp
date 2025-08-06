#include <warthog/io/stream_listener.h>
#include <exception>
#include <fstream>
#include <iostream>

namespace warthog::io
{

stream_listener::stream_listener(const std::filesystem::path& filename, std::ios_base::openmode mode)
{
	shared_stream_ = std::make_shared<std::ofstream>(filename, mode);
	stream_ = shared_stream_.get();
}
stream_listener::stream_listener(std::ostream& stream)
{
	stream_ = &stream;
}
stream_listener::stream_listener(const shared_stream_t& stream)
{
	shared_stream_ = stream;
	stream_ = shared_stream_.get();
}
stream_listener::~stream_listener() = default;

void stream_listener::stream_open(const std::filesystem::path& filename, std::ios_base::openmode mode)
{
	shared_stream_ = std::make_shared<std::ofstream>(filename, mode);
	stream_ = shared_stream_.get();
}
void stream_listener::stream(std::ostream& stream)
{
	stream_ = &stream;
	shared_stream_ = nullptr;
}
void stream_listener::stream_share(const shared_stream_t& stream)
{
	shared_stream_ = stream;
	stream_ = shared_stream_.get();
}
void stream_listener::stream_share(const stream_listener& stream)
{
	shared_stream_ = stream.shared_stream_;
	stream_ = stream.stream_;
}
void stream_listener::stream_stdout()
{
	shared_stream_ = nullptr;
	stream_ = &std::cout;
}
void stream_listener::stream_stderr()
{
	shared_stream_ = nullptr;
	stream_ = &std::cerr;
}
void stream_listener::clear_stream()
{
	shared_stream_ = nullptr;
	stream_ = nullptr;
}

} // namespace warthog::io
