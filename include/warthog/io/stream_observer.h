#ifndef WARTHOG_IO_STEAM_OBSERVER_H
#define WARTHOG_IO_STEAM_OBSERVER_H

/// @file stream_observer.h
///
/// The stream observer is a base class for observers that can open and own a
/// filestream, or pass another filestream.
/// This is designed as a many-to-one observers to stream.
/// No inbuilt support for multi-threading, use locks in the observer function.
///
/// Is designed to be used with observer tuples.
/// Inherited class will call stream() to get the current stream for output.
/// Using the observer methodology, event functions will be given that will
/// write to output in certain ways.
///
/// @author: Ryan Hechenberger
/// @created: 2025-08-01

#include "fwd.h"

#include "log.h"
#include <warthog/constants.h>
#include <warthog/forward.h>

#include <filesystem>
#include <memory>
#include <ostream>

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
	/// @brief observer with no open stream
	stream_observer() noexcept = default;
	/// @brief copies other stream_observer stream
	stream_observer(const stream_observer& stream) noexcept = default;
	/// @brief ovserver with ostream, can pass std::cout, std::cerr or user
	/// supplied std::ostream
	stream_observer(std::ostream& stream) noexcept : stream_(&stream) { }

	/// @brief set stream
	void
	open(std::ostream& stream) noexcept
	{
		stream_ = &stream;
	}
	/// @brief unsets the stream
	void
	close() noexcept
	{
		stream_ = nullptr;
	}
	/// @brief undefined behaviour if no stream is open (asserts on debug)
	std::ostream&
	stream() const noexcept
	{
		assert(stream_ != nullptr);
		return *stream_;
	}

	operator bool() const noexcept { return stream_ != nullptr; }

protected:
	std::ostream* stream_ = nullptr;
};

/// @brief observer holds support for printing lines, send to a std::ostream or
/// connect to a log_sink.
class line_observer
{
public:
	/// @brief observer with no open stream
	line_observer() noexcept = default;
	/// @brief copies other line_observer
	line_observer(const line_observer& stream) noexcept = default;
	/// @brief ovserver with ostream, can pass std::cout, std::cerr or user
	/// supplied std::ostream
	line_observer(std::ostream& stream) noexcept : sink_(&stream) { }
	/// @brief copies other stream_observer stream
	line_observer(log_sink& logger, log_level level) noexcept
	{
		log(logger, level);
	}

	/// @brief pass a stream object
	void
	open(std::ostream& stream) noexcept
	{
		sink_.stream = &stream;
	}
	/// @brief unsets the stream
	void
	close() noexcept
	{
		sink_.stream = nullptr;
	}
	/// @brief pass a log to write line instead of a stream
	void
	log(log_sink& logger, log_level level) noexcept
	{
		sink_.log = &logger;
		type_     = (int)level;
		if((uint32_t)level >= (uint32_t)log_level::NONE)
		{
			WARTHOG_GWARN("line_observer::log level is out of range");
			close();
		}
	}

	operator bool() const noexcept { return sink_.stream != nullptr; }

	/// @brief writes line out (no newline needed), send to std::ostream if set
	/// to stream, to log at
	///        level if set to log_sink, otherwise do nothing
	void
	writeln(std::string_view msg)
	{
		if(sink_.stream != nullptr)
		{
			if(type_ < 0) { (*sink_.stream) << msg << '\n'; }
			else { WARTHOG_LOG(*sink_.log, (log_level)type_, msg); }
		}
	}

protected:
	union
	{
		std::ostream* stream = nullptr;
		log_sink* log;
	} sink_;
	int type_ = -1; ///< if <0: sink is type stream, otherwise is type log
};

} // namespace warthog::io

#endif // WARTHOG_IO_STEAM_OBSERVER_H
