// Copyright The OpenTelemetry Authors
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <fstream>
#include <iostream>
#include <string>
#include <map>

#include <opentelemetry/sdk/trace/processor.h>
#include <opentelemetry/sdk/trace/span_data.h>
#include <opentelemetry/trace/span_context.h>
#include <opentelemetry/trace/tracer.h>


extern std::map<std::string, opentelemetry::trace::SpanContext*> spanContextMap;
extern std::shared_ptr<trace_api::TracerProvider> global_provider;
extern std::atomic<bool> stop_thread;

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
 * All calls to the configured SpanExporter are synchronized using a
 * spin-lock on an atomic_flag.
 */


class ProfileSpanProcessor : public SpanProcessor
{
public:
	explicit ProfileSpanProcessor() noexcept;

	std::unique_ptr<Recordable> MakeRecordable() noexcept override;

	void OnStart(Recordable & record, const opentelemetry::trace::SpanContext&
		parent_context) noexcept override;

	void OnEnd(std::unique_ptr<Recordable> &&record) noexcept override;

	bool ForceFlush(std::chrono::microseconds /* timeout */) noexcept override;

  	bool Shutdown(std::chrono::microseconds timeout =
		(std::chrono::microseconds::max)()) noexcept override;

	~ProfileSpanProcessor() override;

private:
	const char* begin_file_name = "/proc/latency-tracker-begin";
	const char* end_file_name = "/proc/latency-tracker-end";

	//std::fstream begin_file_ostream {}, end_file_ostream {};
	FILE* begin_file_fd, *end_file_fd;
};
}  // namespace trace
}  // namespace sdk
OPENTELEMETRY_END_NAMESPACE
