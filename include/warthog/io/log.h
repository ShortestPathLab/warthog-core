#ifndef WARTHOG_IO_LOG_H
#define WARTHOG_IO_LOG_H

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
#include <ranges>
#include <utility>
#include <format>

#ifdef WARTHOG_LOG
#define WARTHOG_DEFAULT_LOG_LEVEL WARTHOG_LOG
#elif !defined(NDEBUG)
#define WARTHOG_DEFAULT_LOG_LEVEL 1
#else
#define WARTHOG_DEFAULT_LOG_LEVEL 2
#endif

#define WARTHOG_TIME_FORMAT "%FT%TZ"

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

	constexpr void log(log_level level, std::string_view msg) const
	{
		if (can_log(level))
		{
			(*call[static_cast<size_t>(level)])(data, msg);
		}
	}
	constexpr bool can_log(log_level level) const noexcept
	{
		return static_cast<size_t>(level) < call.size() && data != nullptr && call[static_cast<size_t>(level)] != nullptr;
	}

	constexpr const log_sink& sink() const noexcept { return *this; }
};

/// sink to cout|cerr
/// Is thread safe as long as std::ios_base::sync_with_stdio(false) for cout and cerr.
/// User provided file stream is not thread safe.
struct log_std_sink : log_sink
{
	// cerr = true will sink to cerr, false to cout
	log_std_sink();
	log_std_sink(bool cerr = true);
	log_std_sink(std::ostream* stream);
	static void write_trace(void*, std::string_view msg);
	static void write_debug(void*, std::string_view msg);
	static void write_information(void*, std::string_view msg);
	static void write_warning(void*, std::string_view msg);
	static void write_error(void*, std::string_view msg);
	static void write_critical(void*, std::string_view msg);
};

template <typename Sink>
concept LogSink = requires (Sink S, log_level level, std::string_view msg)
{
	S.log(level, msg);
	{ S.can_log(level) } -> std::same_as<bool>;
	{ std::as_const(S).sink() } -> std::convertible_to<log_sink>;
};

class log_stream_sink : public log_sink
{
public:
	log_stream_sink();
	log_stream_sink(const std::filesystem::path& filename, std::ios_base::openmode mode = std::ios_base::out | std::ios_base::app);
	log_stream_sink(std::ostream* stream);
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

	using log_sink::log;
	using log_sink::can_log;
	using log_sink::sink;

protected:
	std::ostream* log_stream_ = nullptr;
	std::unique_ptr<std::ofstream> owned_file_;
	std::mutex lock_;
};
static_assert(LogSink<log_stream_sink>, "log_stream_sink must be a log_sink.");

template <log_level MinLevel = static_cast<log_level>(WARTHOG_DEFAULT_LOG_LEVEL)>
struct logger : log_sink
{
	constexpr logger() noexcept = default;
	constexpr logger(log_sink sink) : log_sink(sink)
	{
		for (auto& c : call | std::views::take(static_cast<int>(MinLevel))) {
			c = nullptr;
		}
	}
	
	constexpr logger& operator=(const logger&) noexcept = default;
	constexpr logger& operator=(log_sink sink) noexcept
	{
		for (auto& c : call | std::views::take(static_cast<int>(MinLevel))) {
			c = nullptr;
		}
		return *this;
	}

	constexpr void set_min_level(log_level level) noexcept
	{
		assert(static_cast<size_t>(level) <= static_cast<size_t>(log_level::NONE));
		for (auto& c : call | std::views::take(static_cast<int>(level))) {
			c = nullptr;
		}
	}

	constexpr static log_level min_level = MinLevel;
	template <log_level L>
	constexpr static bool supports_level = MinLevel != log_level::NONE && static_cast<size_t>(L) >= static_cast<size_t>(MinLevel);
};

namespace details {
template <typename T>
struct is_logger : std::false_type
{ };
template <log_level Level>
struct is_logger<logger<Level>> : std::true_type
{ };
};

template <typename Log>
concept Logger = details::is_logger<Log>::value;
template <typename Log, log_level L>
concept LoggerLevel = Logger<Log> && requires {
	requires Log::template supports_level<L>;
};

#define WARTHOG_LOG(lg,level,msg) lg.log(msg)

#define WARTHOG_LOG_LEVEL_(lg,level,msg) \
{if constexpr (::warthog::io::Logger<decltype(lg)>) { \
	if constexpr (::warthog::io::LoggerLevel<decltype(lg), level>) { \
		lg.log(level, msg); \
	} \
} else { \
	lg.log(level, msg); \
}}

#define WARTHOG_TRACE(lg,msg) WARTHOG_LOG_LEVEL_(lg,::warthog::io::log_level::TRACE,msg)
#define WARTHOG_DEBUG(lg,msg) WARTHOG_LOG_LEVEL_(lg,::warthog::io::log_level::DEBUG,msg)
#define WARTHOG_INFORMATION(lg,msg) WARTHOG_LOG_LEVEL_(lg,::warthog::io::log_level::INFORMATION,msg)
#define WARTHOG_WARNING(lg,msg) WARTHOG_LOG_LEVEL_(lg,::warthog::io::log_level::WARNING,msg)
#define WARTHOG_ERROR(lg,msg) WARTHOG_LOG_LEVEL_(lg,::warthog::io::log_level::ERROR,msg)
#define WARTHOG_CRITICAL(lg,msg) WARTHOG_LOG_LEVEL_(lg,::warthog::io::log_level::CRITICAL,msg)

#define WARTHOG_LOG_LEVEL_FMT_(lg,level,...) \
{if constexpr (::warthog::io::Logger<decltype(lg)>) { \
	if constexpr (::warthog::io::LoggerLevel<decltype(lg), level>) { \
		lg.log(level, std::format(__VA_ARGS__)); \
	} \
} else { \
	lg.log(level, std::format(__VA_ARGS__)); \
}}

#define WARTHOG_TRACE_FMT(lg,...) WARTHOG_LOG_LEVEL_FMT_(lg,::warthog::io::log_level::TRACE,__VA_ARGS__)
#define WARTHOG_DEBUG_FMT(lg,...) WARTHOG_LOG_LEVEL_FMT_(lg,::warthog::io::log_level::DEBUG,__VA_ARGS__)
#define WARTHOG_INFORMATION_FMT(lg,...) WARTHOG_LOG_LEVEL_FMT_(lg,::warthog::io::log_level::INFORMATION,__VA_ARGS__)
#define WARTHOG_WARNING_FMT(lg,...) WARTHOG_LOG_LEVEL_FMT_(lg,::warthog::io::log_level::WARNING,__VA_ARGS__)
#define WARTHOG_ERROR_FMT(lg,...) WARTHOG_LOG_LEVEL_FMT_(lg,::warthog::io::log_level::ERROR,__VA_ARGS__)
#define WARTHOG_CRITICAL_FMT(lg,...) WARTHOG_LOG_LEVEL_FMT_(lg,::warthog::io::log_level::CRITICAL,__VA_ARGS__)

// global logger

using global_logger = logger<>;
global_logger& glog();
const log_sink& glogs();
void set_glog(log_sink log);

} // namespace warthog::io

#endif // WARTHOG_IO_LOG_H
