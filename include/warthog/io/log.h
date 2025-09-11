#ifndef WARTHOG_IO_OBSERVER_H
#define WARTHOG_IO_OBSERVER_H

// io/log.h
//
// Logger class.
//
// @author: Ryan Hechenberger
// @created: 2025-09-09
//

#include <warthog/constants.h>
#include <memory>
#include <fstream>
#include <string_view>
#include <array>
#include <filesystem>
#include <mutex>

#ifdef WARTHOG_LOG
#define WARTHOG_DEFAULT_LOG_LEVEL WARTHOG_LOG
#elif !defined(NDEBUG)
#define WARTHOG_DEFAULT_LOG_LEVEL 1
#else
#define WARTHOG_DEFAULT_LOG_LEVEL 4
#endif

#define WARTHOG_TIME_FORMAT "%FT%T%Z"

namespace warthog::io
{


enum class log_level
{
	TRACE = 0,
	DEBUG = 1,
	INFORMATION = 2,
	WARNING = 3,
	ERROR = 4,
	CRITICAL = 5,
	NONE = 6
};

struct log_sink
{
	using call_type = void (void*, std::string_view msg);
	void* data = nullptr;
	std::array<call_type*, static_cast<size_t>(log_level::NONE)> call = {};

	void log(log_level level, std::string_view msg)
	{
		if (static_cast<size_t>(level) < call.size() && (std::bit_cast<uintptr_t>(data) | std::bit_cast<uintptr_t>(call[static_cast<size_t>(level)])) != 0)
		{
			(*call[static_cast<size_t>(level)])(data, msg);
		}
	}

	const log_sink& sink() const noexcept { return *this; }
};

class log_stream_sink : protected log_sink
{
public:
	log_stream_sink();
	log_stream_sink(bool err);
	log_stream_sink(const std::filesystem::path& filename, std::ios_base::openmode mode = std::ios_base::out | std::ios_base::app);
	log_stream_sink(std::ostream& stream);
	~log_stream_sink() = default;

	void open(const std::filesystem::path& filename, std::ios_base::openmode mode = std::ios_base::out | std::ios_base::app);
	void open(std::ostream& stream);
	void open_stderr();

	static void write_trace(void*, std::string_view msg);
	static void write_debug(void*, std::string_view msg);
	static void write_information(void*, std::string_view msg);
	static void write_warning(void*, std::string_view msg);
	static void write_error(void*, std::string_view msg);
	static void write_critical(void*, std::string_view msg);

	using log_sink::sink;

protected:
	std::ostream* log_stream_ = nullptr;
	std::unique_ptr<std::ofstream> owned_file_;
	std::mutex lock_;
};

} // namespace warthog::io

#endif // WARTHOG_IO_OBSERVER_H
