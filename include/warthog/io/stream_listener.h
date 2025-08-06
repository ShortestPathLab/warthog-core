#ifndef WARTHOG_IO_STREAM_LISTENER_H
#define WARTHOG_IO_STREAM_LISTENER_H

// listener/stream_listener.h
//
// A search listener is a callback class that executes specialised
// code for partiular search events, such as when:
//  - a node is generated
//  - a node is expanded
//  - a node is relaxed
//
//  This class implements dummy listener with empty event handlers.
//
// @author: Ryan Hechenberger
// @created: 2025-08-01
//

#include <warthog/constants.h>
#include <warthog/forward.h>
#include <memory>
#include <filesystem>
#include <ostream>

namespace warthog::io
{

class stream_listener
{
public:
	using shared_stream_t = std::shared_ptr<std::ostream>;
	stream_listener() = default;
	stream_listener(const std::filesystem::path& filename, std::ios_base::openmode mode = std::ios_base::out);
	stream_listener(std::ostream& stream);
	stream_listener(const shared_stream_t& stream);
	~stream_listener();

	void stream_open(const std::filesystem::path& filename, std::ios_base::openmode mode = std::ios_base::out);
	void stream(std::ostream& stream);
	void stream_share(const shared_stream_t& stream);
	void stream_share(const stream_listener& stream);
	void stream_stdout();
	void stream_stderr();
	void clear_stream();

	operator bool() const noexcept { return stream_ != nullptr; }

	std::ostream& stream() noexcept { assert(stream_ != nullptr); return *stream_; }
	const shared_stream_t& shared_stream() noexcept { return shared_stream_; }

private:
	std::ostream* stream_ = nullptr;
	shared_stream_t shared_stream_;
};

using void_listener = stream_listener;

} // namespace warthog::io

#endif // WARTHOG_IO_STREAM_LISTENER_H
