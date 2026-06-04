#ifndef WARTHOG_IO_LOG_H
#define WARTHOG_IO_LOG_H

/// @file log.h
///
/// Logging utility framework, where a user can provide at a high-level
/// data structure with function pointers to log messages as a single string
/// with a specific log level.
/// This will call a user-defined function (if able) that will output this
/// message as the user desires.
/// Default functions (output to stderr or file) are defined here.
///
/// The log_sink is a non-owning copyable struct that points to the data and
/// logging function calls. All logging is performed through log_sink. All
/// classes here are thread safe, follow comments for outlying cases.
///
/// Special classes inherit log_sink to provide default functionality.
/// log_sink_std should be used to write to std::cout and std::cerr.
/// log_sink_stream pipe to a stream or open a file stream.
///
/// The logs are made to be logged to a certain log_level.
/// The WARTHOG_TRACE and others in this header provide interfaces to log to a
/// special logger<> class.  This logger<> class knows at compile time the
/// minimum level to log, and with the use of a macro will be compiled out if
/// that level was not set at compile time when used with the logging macros.
///
/// The global logger can be acquired with the glog() (logger<>) or glogs()
/// (log_sink), and set with set_glog(log_sink).  The log level of the global
/// logger is set though the WARTHOG_LOG definition, with a default of 1(debug)
/// for debug builds, and 2(information) for release (NDEBUG defined). Macros
/// like WARTHOG_GTRACE will automatically write to the global logger.
///
/// The _IF will only log if a runtime if true.
/// The _FMT uses the std::format from C++20 to format the output messages.
/// The logging utilities uses dynamic memory std::strings to produce the final
/// log message strings, thus is better to be disabled for release builds
/// through the log level.
///
/// Log messages produce a messaged as "[TIME LEVEL] msg".
/// The time is formatted in ISO with space (yyyy-mm-dd hh:mm:ss), but can be
/// overridden with define cstring WARTHOG_LOG_TIME. Clock is in local system
/// time, but can be set to UTC by defining WARTHOG_LOG_CLOCK_UTC.
///
/// @author: Ryan Hechenberger
/// @created: 2025-09-09

#include <array>
#include <filesystem>
#include <format>
#include <fstream>
#include <memory>
#include <mutex>
#include <ranges>
#include <string_view>
#include <utility>
#include <warthog/constants.h>

// define utility for help with log messages
#define WARTHOG_STRING_(x) #x
#define WARTHOG_STRING(x) WARTHOG_STRING_(x)
#define WARTHOG_STRING2(x) WARTHOG_STRING(x)
#define WARTHOG_STRING3(x) WARTHOG_STRING(x)
#define WARTHOG_STRING4(x) WARTHOG_STRING(x)

#define WARTHOG_LINE WARTHOG_STRING2(__LINE__)
#define WARTHOG_FILENAME_LINE __FILE__ "@" WARTHOG_LINE

// default log levels, also for global logger
#ifdef WARTHOG_LOG
#define WARTHOG_DEFAULT_LOG_LEVEL WARTHOG_LOG
#elif !defined(NDEBUG)
#define WARTHOG_DEFAULT_LOG_LEVEL 1
#else
#define WARTHOG_DEFAULT_LOG_LEVEL 2
#endif

// specify the log time format
#ifdef WARTHOG_LOG_TIME
#define WARTHOG_LOG_TIME_FORMAT WARTHOG_LOG_TIME
#else
#define WARTHOG_LOG_TIME_FORMAT "%F %T"
#endif

// set to use utc time instead of local
#ifdef WARTHOG_LOG_CLOCK_UTC
#define WARTHOG_LOG_NOW (std::chrono::utc_clock::now())
#else
#define WARTHOG_LOG_NOW                                                       \
	(std::chrono::current_zone()->to_local(std::chrono::system_clock::now()))
#endif

namespace warthog::io
{

enum class log_level
{
	TRACE       = 0,
	DEBUG       = 1,
	INFORMATION = 2,
	WARNING     = 3,
	ERROR       = 4,
	CRITICAL    = 5,
	NONE        = 6
};

/// @brief the log class used to write out to a log.
///
/// Takes a pointer for data (must not be null to write).
/// The array call is the logger for each level.
/// log will write if able (data != null && call[level] != null).
struct log_sink
{
	using call_type = void(void*, std::string_view msg);
	void* data      = nullptr;
	std::array<call_type*, static_cast<size_t>(log_level::NONE)> call = {};

	constexpr void
	log(log_level level, std::string_view msg) const
	{
		if(can_log(level)) { (*call[static_cast<size_t>(level)])(data, msg); }
	}
	constexpr bool
	can_log(log_level level) const noexcept
	{
		return static_cast<size_t>(level) < call.size() && data != nullptr
		    && call[static_cast<size_t>(level)] != nullptr;
	}

	constexpr const log_sink&
	sink() const noexcept
	{
		return *this;
	}
};

/// @brief concept that check if class is shaped with a log_sink.
template<typename Sink>
concept LogSink = requires(Sink S, log_level level, std::string_view msg) {
	S.log(level, msg);
	{ S.can_log(level) } -> std::same_as<bool>;
	{ std::as_const(S).sink() } -> std::convertible_to<log_sink>;
};

/// @brief sink to cout|cerr
///
/// Is thread safe as long as std::ios_base::sync_with_stdio(false) has not
/// been called. Can copy go log_sink and then be destructed without issue.
struct log_sink_std : log_sink
{
	// cerr = true will sink to cerr, false to cout
	log_sink_std();
	log_sink_std(bool cerr);
	static void
	write_trace(void*, std::string_view msg);
	static void
	write_debug(void*, std::string_view msg);
	static void
	write_information(void*, std::string_view msg);
	static void
	write_warning(void*, std::string_view msg);
	static void
	write_error(void*, std::string_view msg);
	static void
	write_critical(void*, std::string_view msg);
};

/// @brief sink to a passed stream, or open and own a filestream.
///
/// This class is thread-safe through use of a mutex.
/// Must be kept in scope at all times while its log_sink is in use.
class log_sink_stream : public log_sink
{
public:
	log_sink_stream();
	log_sink_stream(
	    const std::filesystem::path& filename,
	    std::ios_base::openmode mode
	    = std::ios_base::out | std::ios_base::app);
	log_sink_stream(std::ostream* stream);
	~log_sink_stream() = default;

	void
	open(
	    const std::filesystem::path& filename,
	    std::ios_base::openmode mode
	    = std::ios_base::out | std::ios_base::app);
	void
	open(std::ostream& stream);

	static void
	write_trace(void*, std::string_view msg);
	static void
	write_debug(void*, std::string_view msg);
	static void
	write_information(void*, std::string_view msg);
	static void
	write_warning(void*, std::string_view msg);
	static void
	write_error(void*, std::string_view msg);
	static void
	write_critical(void*, std::string_view msg);

protected:
	std::ostream* log_stream_ = nullptr;
	std::unique_ptr<std::ofstream> owned_file_;
	std::mutex lock_;
};
static_assert(LogSink<log_sink_stream>, "log_stream_sink must be a log_sink.");

/// @brief the logger class that manipulates a log_sink.
/// @tparam MinLevel the minimum logging level, derived at compile time.
///         Defaults to the programs default log level.
///
/// A wrapper class for a log_sink that allows to control the log_level.
/// If just using a log_sink, it will always log at every level.
/// This class is handled specially by the logging macros to only log if
/// log_level is a min_level. When setting a long_sink, will clear all pointers
/// below MinLevel.
template<
    log_level MinLevel = static_cast<log_level>(WARTHOG_DEFAULT_LOG_LEVEL)>
struct logger : log_sink
{
	constexpr logger() noexcept = default;
	constexpr logger(log_sink sink) : log_sink(sink)
	{
		for(auto& c : call | std::views::take(static_cast<int>(MinLevel)))
		{
			c = nullptr;
		}
	}

	constexpr logger&
	operator=(const logger&) noexcept
	    = default;
	constexpr logger&
	operator=(log_sink sink) noexcept
	{
		for(auto& c : call | std::views::take(static_cast<int>(MinLevel)))
		{
			c = nullptr;
		}
		return *this;
	}

	/// @brief Overrides MinLevel to level. Can only raise, not lower it.
	/// @param level
	constexpr void
	set_min_level(log_level level) noexcept
	{
		assert(
		    static_cast<size_t>(level)
		    <= static_cast<size_t>(log_level::NONE));
		for(auto& c : call | std::views::take(static_cast<int>(level)))
		{
			c = nullptr;
		}
	}

	constexpr static log_level min_level = MinLevel;
	template<log_level L>
	constexpr static bool supports_level = MinLevel != log_level::NONE
	    && static_cast<size_t>(L) >= static_cast<size_t>(MinLevel);
};

namespace details
{
template<typename T>
struct is_logger : std::false_type
{ };
template<log_level Level>
struct is_logger<logger<Level>> : std::true_type
{ };
};

template<typename Log>
concept Logger = details::is_logger<Log>::value;
template<typename Log, log_level L>
concept LoggerLevel
    = Logger<Log> && requires { requires Log::template supports_level<L>; };

#define WARTHOG_LOG_LEVEL_(cond, lg, level, msg)                              \
	{                                                                         \
		if constexpr(::warthog::io::Logger<decltype(lg)>)                     \
		{                                                                     \
			if constexpr(::warthog::io::LoggerLevel<decltype(lg), level>)     \
			{                                                                 \
				if(cond) (lg).log(level, msg);                                \
			}                                                                 \
		}                                                                     \
		else                                                                  \
		{                                                                     \
			if(cond) (lg).log(level, msg);                                    \
		}                                                                     \
	}

#define WARTHOG_LOG(lg, level, msg) (lg).log(level, msg)
#define WARTHOG_LOG_IF(cond, lg, level, msg)                                  \
	{                                                                         \
		if(cond) (lg).log(level, msg);                                        \
	}

// Write msg (string_view) to log lg.
#define WARTHOG_TRACE(lg, msg)                                                \
	WARTHOG_LOG_LEVEL_(true, lg, ::warthog::io::log_level::TRACE, msg)
#define WARTHOG_DEBUG(lg, msg)                                                \
	WARTHOG_LOG_LEVEL_(true, lg, ::warthog::io::log_level::DEBUG, msg)
#define WARTHOG_INFO(lg, msg)                                                 \
	WARTHOG_LOG_LEVEL_(true, lg, ::warthog::io::log_level::INFORMATION, msg)
#define WARTHOG_WARN(lg, msg)                                                 \
	WARTHOG_LOG_LEVEL_(true, lg, ::warthog::io::log_level::WARNING, msg)
#define WARTHOG_ERROR(lg, msg)                                                \
	WARTHOG_LOG_LEVEL_(true, lg, ::warthog::io::log_level::ERROR, msg)
#define WARTHOG_CRIT(lg, msg)                                                 \
	WARTHOG_LOG_LEVEL_(true, lg, ::warthog::io::log_level::CRITICAL, msg)

// Write msg (string_view) to log lg if runtime cond is true.
#define WARTHOG_TRACE_IF(cond, lg, msg)                                       \
	WARTHOG_LOG_LEVEL_(cond, lg, ::warthog::io::log_level::TRACE, msg)
#define WARTHOG_DEBUG_IF(cond, lg, msg)                                       \
	WARTHOG_LOG_LEVEL_(cond, lg, ::warthog::io::log_level::DEBUG, msg)
#define WARTHOG_INFO_IF(cond, lg, msg)                                        \
	WARTHOG_LOG_LEVEL_(cond, lg, ::warthog::io::log_level::INFORMATION, msg)
#define WARTHOG_WARN_IF(cond, lg, msg)                                        \
	WARTHOG_LOG_LEVEL_(cond, lg, ::warthog::io::log_level::WARNING, msg)
#define WARTHOG_ERROR_IF(cond, lg, msg)                                       \
	WARTHOG_LOG_LEVEL_(cond, lg, ::warthog::io::log_level::ERROR, msg)
#define WARTHOG_CRIT_IF(cond, lg, msg)                                        \
	WARTHOG_LOG_LEVEL_(cond, lg, ::warthog::io::log_level::CRITICAL, msg)

#define WARTHOG_LOG_LEVEL_FMT_(cond, lg, level, ...)                          \
	{                                                                         \
		if constexpr(::warthog::io::Logger<decltype(lg)>)                     \
		{                                                                     \
			if constexpr(::warthog::io::LoggerLevel<decltype(lg), level>)     \
			{                                                                 \
				if(cond) (lg).log(level, std::format(__VA_ARGS__));           \
			}                                                                 \
		}                                                                     \
		else                                                                  \
		{                                                                     \
			if(cond) (lg).log(level, std::format(__VA_ARGS__));               \
		}                                                                     \
	}

#define WARTHOG_LOG_FMT(lg, level, ...)                                       \
	(lg).log(level, std::format(__VA_ARGS__))
#define WARTHOG_LOG_FMT_IF(cond, lg, level, ...)                              \
	{                                                                         \
		if(cond) (lg).log(level, std::format(__VA_ARGS__));                   \
	}

// Write formatted message to log, pass as format,args...
#define WARTHOG_TRACE_FMT(lg, ...)                                            \
	WARTHOG_LOG_LEVEL_FMT_(                                                   \
	    true, lg, ::warthog::io::log_level::TRACE, __VA_ARGS__)
#define WARTHOG_DEBUG_FMT(lg, ...)                                            \
	WARTHOG_LOG_LEVEL_FMT_(                                                   \
	    true, lg, ::warthog::io::log_level::DEBUG, __VA_ARGS__)
#define WARTHOG_INFO_FMT(lg, ...)                                             \
	WARTHOG_LOG_LEVEL_FMT_(                                                   \
	    true, lg, ::warthog::io::log_level::INFORMATION, __VA_ARGS__)
#define WARTHOG_WARN_FMT(lg, ...)                                             \
	WARTHOG_LOG_LEVEL_FMT_(                                                   \
	    true, lg, ::warthog::io::log_level::WARNING, __VA_ARGS__)
#define WARTHOG_ERROR_FMT(lg, ...)                                            \
	WARTHOG_LOG_LEVEL_FMT_(                                                   \
	    true, lg, ::warthog::io::log_level::ERROR, __VA_ARGS__)
#define WARTHOG_CRIT_FMT(lg, ...)                                             \
	WARTHOG_LOG_LEVEL_FMT_(                                                   \
	    true, lg, ::warthog::io::log_level::CRITICAL, __VA_ARGS__)

// Write formatted message to log if cond is true, pass as format,args...
#define WARTHOG_TRACE_FMT_IF(cond, lg, ...)                                   \
	WARTHOG_LOG_LEVEL_FMT_(                                                   \
	    cond, lg, ::warthog::io::log_level::TRACE, __VA_ARGS__)
#define WARTHOG_DEBUG_FMT_IF(cond, lg, ...)                                   \
	WARTHOG_LOG_LEVEL_FMT_(                                                   \
	    cond, lg, ::warthog::io::log_level::DEBUG, __VA_ARGS__)
#define WARTHOG_INFO_FMT_IF(cond, lg, ...)                                    \
	WARTHOG_LOG_LEVEL_FMT_(                                                   \
	    cond, lg, ::warthog::io::log_level::INFORMATION, __VA_ARGS__)
#define WARTHOG_WARN_FMT_IF(cond, lg, ...)                                    \
	WARTHOG_LOG_LEVEL_FMT_(                                                   \
	    cond, lg, ::warthog::io::log_level::WARNING, __VA_ARGS__)
#define WARTHOG_ERROR_FMT_IF(cond, lg, ...)                                   \
	WARTHOG_LOG_LEVEL_FMT_(                                                   \
	    cond, lg, ::warthog::io::log_level::ERROR, __VA_ARGS__)
#define WARTHOG_CRIT_FMT_IF(cond, lg, ...)                                    \
	WARTHOG_LOG_LEVEL_FMT_(                                                   \
	    cond, lg, ::warthog::io::log_level::CRITICAL, __VA_ARGS__)

// global logger

using global_logger_type = logger<>;
/// @brief get the global logger, default is set to std::cerr through
/// log_sink_std
global_logger_type&
glog();
/// @brief get the global log_sink
const log_sink&
glogs();
/// @brief sets the global log_sink
void
set_glog(log_sink log);

/// @brief A logger that sets itself to glog()
/// @tparam MinLevel
template<log_level MinLevel = log_level::DEBUG>
struct global_logger : logger<MinLevel>
{
	using logger = typename global_logger::logger;
	constexpr global_logger() : logger(glog().sink()) { }

	constexpr global_logger&
	operator=(const global_logger&) noexcept
	    = default;

	/// @brief Updates the sink to the new glog sink.  Must be called if
	/// set_glog was used.
	void
	resync_global()
	{
		static_cast<logger&>(*this) = glog().sink();
	}
};

// the global version of the marcos, omits the passing of a logger.
#define WARTHOG_GLOG(level, msg) WARTHOG_LOG(::warthog::io::glog(), level, msg)
#define WARTHOG_GTRACE(msg) WARTHOG_TRACE(::warthog::io::glog(), msg)
#define WARTHOG_GDEBUG(msg) WARTHOG_DEBUG(::warthog::io::glog(), msg)
#define WARTHOG_GINFO(msg) WARTHOG_INFO(::warthog::io::glog(), msg)
#define WARTHOG_GWARN(msg) WARTHOG_WARN(::warthog::io::glog(), msg)
#define WARTHOG_GERROR(msg) WARTHOG_ERROR(::warthog::io::glog(), msg)
#define WARTHOG_GCRIT(msg) WARTHOG_CRIT(::warthog::io::glog(), msg)
#define WARTHOG_GLOG_IF(cond, level, msg)                                     \
	WARTHOG_LOG_IF(cond, ::warthog::io::glog(), level, msg)
#define WARTHOG_GTRACE_IF(cond, msg)                                          \
	WARTHOG_TRACE_IF(cond, ::warthog::io::glog(), msg)
#define WARTHOG_GDEBUG_IF(cond, msg)                                          \
	WARTHOG_DEBUG_IF(cond, ::warthog::io::glog(), msg)
#define WARTHOG_GINFO_IF(cond, msg)                                           \
	WARTHOG_INFO_IF(cond, ::warthog::io::glog(), msg)
#define WARTHOG_GWARN_IF(cond, msg)                                           \
	WARTHOG_WARN_IF(cond, ::warthog::io::glog(), msg)
#define WARTHOG_GERROR_IF(cond, msg)                                          \
	WARTHOG_ERROR_IF(cond, ::warthog::io::glog(), msg)
#define WARTHOG_GCRIT_IF(cond, msg)                                           \
	WARTHOG_CRIT_IF(cond, ::warthog::io::glog(), msg)
#define WARTHOG_GLOG_FMT(level, ...)                                          \
	WARTHOG_LOG_FMT(::warthog::io::glog(), level, __VA_ARGS__)
#define WARTHOG_GTRACE_FMT(...)                                               \
	WARTHOG_TRACE_FMT(::warthog::io::glog(), __VA_ARGS__)
#define WARTHOG_GDEBUG_FMT(...)                                               \
	WARTHOG_DEBUG_FMT(::warthog::io::glog(), __VA_ARGS__)
#define WARTHOG_GINFO_FMT(...)                                                \
	WARTHOG_INFO_FMT(::warthog::io::glog(), __VA_ARGS__)
#define WARTHOG_GWARN_FMT(...)                                                \
	WARTHOG_WARN_FMT(::warthog::io::glog(), __VA_ARGS__)
#define WARTHOG_GERROR_FMT(...)                                               \
	WARTHOG_ERROR_FMT(::warthog::io::glog(), __VA_ARGS__)
#define WARTHOG_GCRIT_FMT(...)                                                \
	WARTHOG_CRIT_FMT(::warthog::io::glog(), __VA_ARGS__)
#define WARTHOG_GLOG_FMT_IF(cond, level, ...)                                 \
	WARTHOG_LOG_FMT_IF(cond, ::warthog::io::glog(), level, __VA_ARGS__)
#define WARTHOG_GTRACE_FMT_IF(cond, ...)                                      \
	WARTHOG_TRACE_FMT_IF(cond, ::warthog::io::glog(), __VA_ARGS__)
#define WARTHOG_GDEBUG_FMT_IF(cond, ...)                                      \
	WARTHOG_DEBUG_FMT_IF(cond, ::warthog::io::glog(), __VA_ARGS__)
#define WARTHOG_GINFO_FMT_IF(cond, ...)                                       \
	WARTHOG_INFO_FMT_IF(cond, ::warthog::io::glog(), __VA_ARGS__)
#define WARTHOG_GWARN_FMT_IF(cond, ...)                                       \
	WARTHOG_WARN_FMT_IF(cond, ::warthog::io::glog(), __VA_ARGS__)
#define WARTHOG_GERROR_FMT_IF(cond, ...)                                      \
	WARTHOG_ERROR_FMT_IF(cond, ::warthog::io::glog(), __VA_ARGS__)
#define WARTHOG_GCRIT_FMT_IF(cond, ...)                                       \
	WARTHOG_CRIT_FMT_IF(cond, ::warthog::io::glog(), __VA_ARGS__)

} // namespace warthog::io

#endif // WARTHOG_IO_LOG_H
