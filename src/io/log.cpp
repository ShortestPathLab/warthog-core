#include <warthog/io/log.h>
#include <format>
#include <chrono>
#include <iostream>

namespace warthog::io
{

log_stream_sink::log_stream_sink() : log_sink{nullptr,
	{{&log_stream_sink::write_trace,&log_stream_sink::write_debug,
	&log_stream_sink::write_information,&log_stream_sink::write_warning,
	&log_stream_sink::write_error,&log_stream_sink::write_critical}}}
{ }

log_stream_sink::log_stream_sink(bool err)
{
	if (err) {
		data = this;
		log_stream_ = &std::cerr;
	}
}
log_stream_sink::log_stream_sink(const std::filesystem::path& filename, std::ios_base::openmode mode)
{
	data = this;
	owned_file_ = std::make_unique<std::ofstream>(filename, mode);
	log_stream_ = owned_file_.get();
}
log_stream_sink::log_stream_sink(std::ostream& stream)
{
	data = this;
	log_stream_ = &stream;
}

void log_stream_sink::open(const std::filesystem::path& filename, std::ios_base::openmode mode)
{
	data = this;
	owned_file_ = std::make_unique<std::ofstream>(filename, mode);
	log_stream_ = owned_file_.get();
}
void log_stream_sink::open(std::ostream& stream)
{
	data = this;
	owned_file_.release();
	log_stream_ = &stream;
}
void log_stream_sink::open_stderr()
{
	data = this;
	owned_file_.release();
	log_stream_ = &std::cerr;
}

void log_stream_sink::write_trace(void* logger, std::string_view msg)
{
	log_stream_sink* log_stream = static_cast<log_stream_sink*>(logger);
	assert(log_stream != nullptr && log_stream->log_stream_);
	std::lock_guard lock(log_stream->lock_);
	*log_stream->log_stream_ << std::format("[{:" WARTHOG_TIME_FORMAT "} TRACE] {}", std::chrono::utc_clock::now(), msg);
}
void log_stream_sink::write_debug(void* logger, std::string_view msg)
{
	log_stream_sink* log_stream = static_cast<log_stream_sink*>(logger);
	assert(log_stream != nullptr && log_stream->log_stream_);
	std::lock_guard lock(log_stream->lock_);
	*log_stream->log_stream_ << std::format("[{:" WARTHOG_TIME_FORMAT "} DEBUG] {}", std::chrono::utc_clock::now(), msg);
}
void log_stream_sink::write_information(void* logger, std::string_view msg)
{
	log_stream_sink* log_stream = static_cast<log_stream_sink*>(logger);
	assert(log_stream != nullptr && log_stream->log_stream_);
	std::lock_guard lock(log_stream->lock_);
	*log_stream->log_stream_ << std::format("[{:" WARTHOG_TIME_FORMAT "}  INFO] {}", std::chrono::utc_clock::now(), msg);
}
void log_stream_sink::write_warning(void* logger, std::string_view msg)
{
	log_stream_sink* log_stream = static_cast<log_stream_sink*>(logger);
	assert(log_stream != nullptr && log_stream->log_stream_);
	std::lock_guard lock(log_stream->lock_);
	*log_stream->log_stream_ << std::format("[{:" WARTHOG_TIME_FORMAT "}  WARN] {}", std::chrono::utc_clock::now(), msg);
}
void log_stream_sink::write_error(void* logger, std::string_view msg)
{
	log_stream_sink* log_stream = static_cast<log_stream_sink*>(logger);
	assert(log_stream != nullptr && log_stream->log_stream_);
	std::lock_guard lock(log_stream->lock_);
	*log_stream->log_stream_ << std::format("[{:" WARTHOG_TIME_FORMAT "} ERROR] {}", std::chrono::utc_clock::now(), msg);
}
void log_stream_sink::write_critical(void* logger, std::string_view msg)
{
	log_stream_sink* log_stream = static_cast<log_stream_sink*>(logger);
	assert(log_stream != nullptr && log_stream->log_stream_);
	std::lock_guard lock(log_stream->lock_);
	*log_stream->log_stream_ << std::format("[{:" WARTHOG_TIME_FORMAT "}  CRIT] {}", std::chrono::utc_clock::now(), msg);
}

} // namespace warthog::io
