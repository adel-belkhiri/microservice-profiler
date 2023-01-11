#include <fstream>
#include <stdio.h>
#include <string.h>
#include <cstdint>
#include <map>
#include <atomic>
#include <chrono>
#include <thread>
#include <unistd.h>

#include <fcntl.h>

#include <opentelemetry/sdk/trace/processor.h>
#include <opentelemetry/sdk/trace/span_data.h>
#include <opentelemetry/trace/span_context.h>
#include <opentelemetry/trace/provider.h>

#include "profile-span-processor.h"

namespace trace_api = opentelemetry::trace;
namespace trace_sdk = opentelemetry::sdk;
namespace nostd     = opentelemetry::nostd;
namespace context   = opentelemetry::context;

std::map<std::string, trace_api::SpanContext*> spanContextMap;
std::shared_ptr<trace_api::TracerProvider> global_provider;

std::atomic<bool> stop_thread (false);


OPENTELEMETRY_BEGIN_NAMESPACE
namespace sdk
{
namespace trace
{

/**
 * This span processor intercepts span begin/end and send corresponding events through
 * the interface of latency-tracker: /proc/latency-tracker-begin and /proc/latency-tracker-end
 *
 * OnEnd and ForceFlush are no-ops.
 *
 */
ProfileSpanProcessor::ProfileSpanProcessor() noexcept
{

	begin_file_fd = fopen(begin_file_name, "w");
	end_file_fd = fopen(end_file_name, "w");

	if(begin_file_fd == nullptr || end_file_fd == nullptr) {
		std::cerr << "Problem opening latency tracker begin/end files"
				<< std::endl;
		return;
	}

	setbuf(begin_file_fd, nullptr);
	setbuf(end_file_fd, nullptr);
}

std::unique_ptr<Recordable> ProfileSpanProcessor::MakeRecordable() noexcept
{
	return std::unique_ptr<Recordable>(new SpanData);
}

void ProfileSpanProcessor::OnStart(Recordable & record,
		const trace_api::SpanContext & parent_context) noexcept
{
	char span_id_str[trace_api::SpanId::kSize * 2 + 1];
	char trace_id_str[trace_api::TraceId::kSize * 2 + 1];

	/* Ignore syscalls transformed into spans start with "__" */
	auto spanData = static_cast<sdk::trace::SpanData *>(&record);
	if(spanData->GetName().substr(0, 2) == "__")
		return;

	/* Write the span id in the /proc/latency-begin-file */
	uint64_t start_ts = spanData->GetStartTime().time_since_epoch().count();
	auto spanId = spanData->GetSpanId();

	char  (&c)[16] = *static_cast<char (*)[16]>(static_cast<void*>(span_id_str));
	spanId.ToLowerBase16(c);
	span_id_str[16] = '\0';

	auto traceId = spanData->GetTraceId();
	char  (&d)[32] = *static_cast<char (*)[32]>(static_cast<void*>(trace_id_str));
	traceId.ToLowerBase16(d);
	trace_id_str[32] = '\0';

	if(begin_file_fd != (void*) 0) {
		//write(begin_file_fd, msg);
		fprintf(begin_file_fd, "%lx:%s:%s\n", start_ts, span_id_str, trace_id_str);
		//fprintf(begin_file_fd, "%s\n", span_id_str);
		//fflush(begin_file_fd);
	}
}

void ProfileSpanProcessor::OnEnd(std::unique_ptr<Recordable> &&record) noexcept
{

	char span_id_str[trace_api::SpanId::kSize * 2 + 1];

	/* ignore syscalls transformed into spans as their names start with "__" */
	auto spanData = static_cast<sdk::trace::SpanData *>(record.get());
	if(spanData->GetName().substr(0, 2) == "__")
		return;

	/* Write the span id in the /proc/latency-end-file */
	auto spanId = spanData->GetSpanId();
	spanId.ToLowerBase16(nostd::span<char, 16>(reinterpret_cast<char *>(span_id_str), 16));
	span_id_str[16] = '\0';

	if (end_file_fd != (void*) 0) {
		//end_file_ostream.write(span_id_str, sizeof(span_id_str));
		//end_file_ostream.flush();
		fprintf(end_file_fd, "%s\n", span_id_str);
		//fflush(end_file_fd);
	}
}

bool ProfileSpanProcessor::ForceFlush(std::chrono::microseconds /* timeout */) noexcept
{
	return true;
}

bool ProfileSpanProcessor::Shutdown(std::chrono::microseconds timeout) noexcept
{
	stop_thread = true;
	std::cout << "shutting down profiling span processor " << std::endl;
	return true;
}

ProfileSpanProcessor::~ProfileSpanProcessor()
{
	if(begin_file_fd)
		fclose(begin_file_fd);

	if(end_file_fd)
		fclose(end_file_fd);
}

}  // namespace trace
}  // namespace sdk
OPENTELEMETRY_END_NAMESPACE
