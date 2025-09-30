#include <warthog/io/log.h>
#include <format>
#include <chrono>
#include <iostream>

namespace warthog::io
{

log_sink_stream::log_sink_stream() : log_sink{nullptr,
	{{&log_sink_stream::write_trace,&log_sink_stream::write_debug,
	&log_sink_stream::write_information,&log_sink_stream::write_warning,
	&log_sink_stream::write_error,&log_sink_stream::write_critical}}}
{ }

log_sink_stream::log_sink_stream(const std::filesystem::path& filename, std::ios_base::openmode mode) : log_sink_stream()
{
	data = this;
	owned_file_ = std::make_unique<std::ofstream>(filename, mode);
	log_stream_ = owned_file_.get();
}
log_sink_stream::log_sink_stream(std::ostream* stream) : log_sink_stream()
{
	data = this;
	log_stream_ = stream;
}

void log_sink_stream::open(const std::filesystem::path& filename, std::ios_base::openmode mode)
{
	data = this;
	owned_file_ = std::make_unique<std::ofstream>(filename, mode);
	log_stream_ = owned_file_.get();
}
void log_sink_stream::open(std::ostream& stream)
{
	data = this;
	owned_file_.release();
	log_stream_ = &stream;
}
void log_sink_stream::open_stderr()
{
	data = this;
	owned_file_.release();
	log_stream_ = &std::cerr;
}

// log_sink_stream

log_sink_std::log_sink_std() : log_sink_std(nullptr)
{ }
log_sink_std::log_sink_std(bool cerr) : log_sink_std(cerr ? &std::cerr : &std::cout)
{ }
log_sink_std::log_sink_std(std::ostream* stream) : log_sink{
	stream,
	{{&log_sink_std::write_trace,&log_sink_std::write_debug,
	&log_sink_std::write_information,&log_sink_std::write_warning,
	&log_sink_std::write_error,&log_sink_std::write_critical}}}
{ }

void log_sink_std::write_trace(void* logger, std::string_view msg)
{
	std::ostream* stream = static_cast<std::ostream*>(logger);
	assert(stream != nullptr);
	*stream << std::format("[{:" WARTHOG_LOG_TIME_FORMAT "} TRACE] {}\n", std::chrono::floor<std::chrono::seconds>(WARTHOG_LOG_NOW), msg);
}
void log_sink_std::write_debug(void* logger, std::string_view msg)
{
	std::ostream* stream = static_cast<std::ostream*>(logger);
	assert(stream != nullptr);
	*stream << std::format("[{:" WARTHOG_LOG_TIME_FORMAT "} DEBUG] {}\n", std::chrono::floor<std::chrono::seconds>(WARTHOG_LOG_NOW), msg);
}
void log_sink_std::write_information(void* logger, std::string_view msg)
{
	std::ostream* stream = static_cast<std::ostream*>(logger);
	assert(stream != nullptr);
	*stream << std::format("[{:" WARTHOG_LOG_TIME_FORMAT "}  INFO] {}\n", std::chrono::floor<std::chrono::seconds>(WARTHOG_LOG_NOW), msg);
}
void log_sink_std::write_warning(void* logger, std::string_view msg)
{
	std::ostream* stream = static_cast<std::ostream*>(logger);
	assert(stream != nullptr);
	*stream << std::format("[{:" WARTHOG_LOG_TIME_FORMAT "}  WARN] {}\n", std::chrono::floor<std::chrono::seconds>(WARTHOG_LOG_NOW), msg);
}
void log_sink_std::write_error(void* logger, std::string_view msg)
{
	std::ostream* stream = static_cast<std::ostream*>(logger);
	assert(stream != nullptr);
	*stream << std::format("[{:" WARTHOG_LOG_TIME_FORMAT "} ERROR] {}\n", std::chrono::floor<std::chrono::seconds>(WARTHOG_LOG_NOW), msg);
}
void log_sink_std::write_critical(void* logger, std::string_view msg)
{
	std::ostream* stream = static_cast<std::ostream*>(logger);
	assert(stream != nullptr);
	*stream << std::format("[{:" WARTHOG_LOG_TIME_FORMAT "}  CRIT] {}\n", std::chrono::floor<std::chrono::seconds>(WARTHOG_LOG_NOW), msg);
}

void log_sink_stream::write_trace(void* logger, std::string_view msg)
{
	log_sink_stream* log_stream = static_cast<log_sink_stream*>(logger);
	assert(log_stream != nullptr && log_stream->log_stream_);
	std::lock_guard lock(log_stream->lock_);
	*log_stream->log_stream_ << std::format("[{:" WARTHOG_LOG_TIME_FORMAT "} TRACE] {}\n", std::chrono::floor<std::chrono::seconds>(WARTHOG_LOG_NOW), msg);
}
void log_sink_stream::write_debug(void* logger, std::string_view msg)
{
	log_sink_stream* log_stream = static_cast<log_sink_stream*>(logger);
	assert(log_stream != nullptr && log_stream->log_stream_);
	std::lock_guard lock(log_stream->lock_);
	*log_stream->log_stream_ << std::format("[{:" WARTHOG_LOG_TIME_FORMAT "} DEBUG] {}\n", std::chrono::floor<std::chrono::seconds>(WARTHOG_LOG_NOW), msg);
}
void log_sink_stream::write_information(void* logger, std::string_view msg)
{
	log_sink_stream* log_stream = static_cast<log_sink_stream*>(logger);
	assert(log_stream != nullptr && log_stream->log_stream_);
	std::lock_guard lock(log_stream->lock_);
	*log_stream->log_stream_ << std::format("[{:" WARTHOG_LOG_TIME_FORMAT "}  INFO] {}\n", std::chrono::floor<std::chrono::seconds>(WARTHOG_LOG_NOW), msg);
}
void log_sink_stream::write_warning(void* logger, std::string_view msg)
{
	log_sink_stream* log_stream = static_cast<log_sink_stream*>(logger);
	assert(log_stream != nullptr && log_stream->log_stream_);
	std::lock_guard lock(log_stream->lock_);
	*log_stream->log_stream_ << std::format("[{:" WARTHOG_LOG_TIME_FORMAT "}  WARN] {}\n", std::chrono::floor<std::chrono::seconds>(WARTHOG_LOG_NOW), msg);
}
void log_sink_stream::write_error(void* logger, std::string_view msg)
{
	log_sink_stream* log_stream = static_cast<log_sink_stream*>(logger);
	assert(log_stream != nullptr && log_stream->log_stream_);
	std::lock_guard lock(log_stream->lock_);
	*log_stream->log_stream_ << std::format("[{:" WARTHOG_LOG_TIME_FORMAT "} ERROR] {}\n", std::chrono::floor<std::chrono::seconds>(WARTHOG_LOG_NOW), msg);
}
void log_sink_stream::write_critical(void* logger, std::string_view msg)
{
	log_sink_stream* log_stream = static_cast<log_sink_stream*>(logger);
	assert(log_stream != nullptr && log_stream->log_stream_);
	std::lock_guard lock(log_stream->lock_);
	*log_stream->log_stream_ << std::format("[{:" WARTHOG_LOG_TIME_FORMAT "}  CRIT] {}\n", std::chrono::floor<std::chrono::seconds>(WARTHOG_LOG_NOW), msg);
}

global_logger_type global_logger_(log_sink_std(true).sink());
global_logger_type& glog()
{
	return global_logger_;
}
const log_sink& glogs()
{
	return global_logger_.sink();
}
void set_glog(log_sink log)
{
	global_logger_ = log;
}

} // namespace warthog::io
