#include <warthog/io/stream_listener.h>
#include <exception>
#include <fstream>

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

void stream_open(const std::filesystem::path& filename);
void stream(std::ostream& stream);
void stream_share(const shared_stream_t& stream);
void stream_share(const stream_listener& stream);
void stream_stdin();
void stream_stderr();
void clear_stream();

} // namespace warthog::io
