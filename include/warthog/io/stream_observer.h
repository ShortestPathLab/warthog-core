#ifndef WARTHOG_IO_STEAM_OBSERVER_H
#define WARTHOG_IO_STEAM_OBSERVER_H

// io/stream_observer.h
//
// The stream observer is a base class for observers that can open and own a
// filestream, or pass another filestream.
//
// Is designed to be used with observer tuples.
// Inherited class will call stream() to get the current stream for output.
// Using the observer methodology, event functions will be given that will
// write to output in certain ways.
//
// @author: Ryan Hechenberger
// @created: 2025-08-01
//

#include <filesystem>
#include <memory>
#include <ostream>
#include <warthog/constants.h>
#include <warthog/forward.h>

namespace warthog::io
{

/// @brief A base-stream observer.
///
/// Inherit and give event functions for observers to call.
/// shared_stream_t is a shared_ptr to an std::ostream, and can be passed
/// around to other stream_observer.
class stream_observer
{
public:
	using shared_stream_t = std::shared_ptr<std::ostream>;
	stream_observer()     = default;
	stream_observer(
	    const std::filesystem::path& filename,
	    std::ios_base::openmode mode = std::ios_base::out);
	stream_observer(std::ostream& stream);
	stream_observer(const shared_stream_t& stream);
	~stream_observer();

	/// @brief open a file stream, store in a shared_stream_t
	void
	stream_open(
	    const std::filesystem::path& filename,
	    std::ios_base::openmode mode = std::ios_base::out);
	/// @brief pass an existing stream, must remain in
	void
	stream(std::ostream& stream);
	/// @brief pass a shared stream
	void
	stream_share(const shared_stream_t& stream);
	/// @brief copy stream from another stream_observer
	void
	stream_share(const stream_observer& stream);
	/// @brief sets to std::cout
	void
	stream_stdout();
	/// @brief sets to std::cerr
	void
	stream_stderr();
	/// @brief unsets the stream
	void
	clear_stream();

	operator bool() const noexcept { return stream_ != nullptr; }

	/// @brief undefined behaviour if no stream is open (asserts on debug)
	std::ostream&
	stream() const noexcept
	{
		assert(stream_ != nullptr);
		return *stream_;
	}
	const shared_stream_t&
	shared_stream() const noexcept
	{
		return shared_stream_;
	}

private:
	std::ostream* stream_ = nullptr;
	shared_stream_t shared_stream_;
};

} // namespace warthog::io

#endif // WARTHOG_IO_STEAM_OBSERVER_H
