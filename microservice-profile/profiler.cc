/*
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; only
 * version 2.1 of the License.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */

#include <thread>
#include <iostream>
#include <atomic>
#include <unistd.h>
#include <fcntl.h>
#include <poll.h>
#include <charconv>
#include <cstring>
#include <unistd.h>

#include <microservice_profile.h>
#include <opentelemetry/trace/provider.h>
#include <opentelemetry/common/timestamp.h>
#include <opentelemetry/nostd/span.h>
#include <opentelemetry/trace/context.h>
#include <opentelemetry/trace/propagation/detail/hex.h>

#include "profile-span-processor.h"

#define SERVICE_NAME_MAX_SIZE 28
#define SPAN_ID_MAX_SIZE 32
#define SYSCALL_NAME_MAX_SIZE 16


namespace trace_api = opentelemetry::trace;
namespace nostd     = opentelemetry::nostd;
namespace context   = opentelemetry::context;


struct syscall_desc {
	char name[SYSCALL_NAME_MAX_SIZE];
	uint64_t start_system;
	uint64_t start_steady;
	uint64_t end_steady;
};

namespace microservice_profile
{

class Profiler
{
public:
  	Profiler();
  	~Profiler();

private:
	int OpenRelayFile();
	void StartReaderThreads(int fd);
    void ReaderThread(int fd);
	void ReadAnnotation(int relay_fd);
	void InjectAnnotation(uint32_t nb_syscalls, char* header_buf,
		struct syscall_desc *syscalls);

private:
	std::unique_ptr<std::thread> reader_thread;
	const char* app_dirname = "/sys/kernel/debug/latency/spans/default/channels";
	//std::atomic<bool> stop_thread = false;
};

Profiler::Profiler()
{
	int fd;

    StartMicroserviceProfile();
	fd = OpenRelayFile();

	if(fd > 0) {
		StartReaderThreads(fd);
	} else {
		std::cerr<< "Exiting .." <<std::endl;
		exit(-1);
	}
}

/*
 * Open the relay file associated with this process
 */
int Profiler::OpenRelayFile()
{
	char tmp[4096];
	int relay_file_descr;
	pid_t pid;

	pid = getpid();

	sprintf(tmp, "%s/rchan-%d-0", this->app_dirname, pid);
	relay_file_descr = open(tmp, O_RDONLY);

	//std::cout << "opening the relay file " << tmp << ", fd = " << relay_file_descr<< std::endl;

	if (relay_file_descr < 0) {
		std::cerr << "Couldn't open the relay file: "
		          << tmp
				  << std::endl;
		return -1;
	}
	return relay_file_descr;
}


nostd::shared_ptr<trace_api::Tracer> get_tracer()
{
  auto provider = trace_api::Provider::GetTracerProvider();
  return provider->GetTracer("monitoring library");
}

void Profiler::InjectAnnotation(uint32_t nb_syscalls, char* header_buf,
	struct syscall_desc *syscalls) {

	uint8_t span_id_bytes[8];
	uint8_t trace_id_bytes[16];
	bool res;
	char* span_id_hex, *trace_id_hex;
	trace_api::StartSpanOptions startOptions, startOptionsSyscalls;
	trace_api::EndSpanOptions endOptions, endOptionsSyscalls;

	//std::cout << "- Number of syscalls : " << nb_syscalls << std::endl;

	span_id_hex = header_buf + 12;
	trace_id_hex = header_buf + 28;

	res = trace_api::propagation::detail::HexToBinary(nostd::string_view (span_id_hex, 16),
		span_id_bytes, sizeof(span_id_bytes));
	res &= trace_api::propagation::detail::HexToBinary(nostd::string_view (trace_id_hex, 32),
		trace_id_bytes, sizeof(trace_id_bytes));

	if(res == false) {
		std::cerr << "Error! problem converting span/trace ids hex strings to bytes .."
				  << std::endl;
		return;
	}

	//std::cout << "----- span_id_hex: " << span_id_hex << ":" << std::endl;
	/* Recreate the span context */
	trace_api::SpanContext span_context(
		trace_api::TraceId(trace_id_bytes),
		trace_api::SpanId(span_id_bytes),
		trace_api::TraceFlags((uint8_t) true),
		false);

	//nostd::shared_ptr<trace_api::Span>
	//	outer_span{new trace_api::DefaultSpan(span_context)};
	//auto context = context::RuntimeContext::GetCurrent();
    //auto new_ctx = trace_api::SetSpan(context, outer_span);
	//trace_api::SetSpan(new_ctx, outer_span);
	//context::RuntimeContext::Attach(new_ctx);


	startOptions.parent = span_context; //outer_span->GetContext();

	/* Get the tracer provider instance */
	auto provider = trace_api::Provider::GetTracerProvider();

	/* Create syscalls as spans */
	uint64_t ts;
	if(nb_syscalls > 0) {
		startOptions.start_system_time = opentelemetry::common::SystemTimestamp(
			std::chrono::nanoseconds(syscalls[0].start_system));
		startOptions.start_steady_time = opentelemetry::common::SteadyTimestamp(
			std::chrono::nanoseconds(syscalls[0].start_steady));

		auto tracer = provider->GetTracer("Monitoring library");
		auto outer_span = tracer->StartSpan(std::string("kernel"), startOptions);

		startOptionsSyscalls.parent = outer_span->GetContext();
		for (int i = 0; i < nb_syscalls; i++) {

			ts = syscalls[i].start_system;
			startOptionsSyscalls.start_system_time = opentelemetry::common::SystemTimestamp(
				std::chrono::nanoseconds(syscalls[i].start_system));
			startOptionsSyscalls.start_steady_time = opentelemetry::common::SteadyTimestamp(
				std::chrono::nanoseconds(syscalls[i].start_steady));

			//std::cout << "-> span_id_hex: " << span_id_hex << ", [" << ts << "] " << std::endl;

			if(stop_thread) {
				std::cout << "Monitoring thread exiting.." <<std::endl;
				exit(0);
			}

			auto tracer = provider->GetTracer("Monitoring library");
			auto span = tracer->StartSpan("__" + std::string(syscalls[i].name), startOptionsSyscalls);

			endOptionsSyscalls.end_steady_time = opentelemetry::common::SteadyTimestamp(std::chrono::nanoseconds(syscalls[i].end_steady));
			span->End(endOptionsSyscalls);

			//std::cout << "\t - syscall: " << syscalls[i].name
			//<< ", start_system: " << syscalls[i].start_system
			//<< ", start_steady: " << syscalls[i].start_steady
			//<< ", end_steady: " << syscalls[i].end_steady
			//<< std::endl;
		}

		endOptions.end_steady_time = opentelemetry::common::SteadyTimestamp(
			std::chrono::nanoseconds(syscalls[nb_syscalls - 1].end_steady)
			);
		outer_span->End(endOptions);
	}
}

/*
 * Read the relay file and inject annotation into the distributed trace
 */
void Profiler::ReadAnnotation(int relay_fd) {
	struct pollfd poll_fd;
	char header_buf[64];
	int rc;
	char service_name[SERVICE_NAME_MAX_SIZE];
	char span_id[SPAN_ID_MAX_SIZE];
	uint32_t nb_syscalls;
	struct syscall_desc syscalls[256];


	std::cout << "Monitoring thread starting ..." << std::endl;
	UnregisterMonitoringThread();

	//opentelemetry::nostd::shared_ptr<trace_api::TracerProvider> provider =
	//											trace_api::Provider::GetTracerProvider();

	poll_fd.fd = relay_fd;
	poll_fd.events = POLLIN;
	do {
		rc = poll(&poll_fd, 1, 1 );/*milli*/

		if (stop_thread) break;

		if (rc < 0) {
			std::cerr << "poll error: " << std::endl;
			break;
		}
		if (rc == 0) { /* timeout */
			continue;
		}
		else if (poll_fd.revents & POLLIN) {
			rc = read(relay_fd, header_buf, 64 /* header size */);
			if (rc < 0) {
				std::cerr << "Error reading from the relay file" << std::endl;
				continue;
			} else if (rc > 0) {
				memcpy(&nb_syscalls, header_buf, (sizeof(uint32_t)));
				if(nb_syscalls > 0) {
					rc = read(relay_fd, syscalls, nb_syscalls * sizeof(syscall_desc));
					if (rc < 0) {
						std::cerr << "Error reading from the relay file" << std::endl;
						continue;
					} else {
						InjectAnnotation(nb_syscalls, header_buf + 4, syscalls);
					}
				}
			}
		}

	} while (1);
	std::cout << "Monitoring thread exiting ..." << std::endl;
}

/*
 * Start reader threads
 */
void Profiler::StartReaderThreads(int fd) {
	reader_thread = std::make_unique<std::thread> (&Profiler::ReadAnnotation, this, fd);
	//reader_thread->detach();
}

Profiler::~Profiler()
{
	stop_thread = true;
	std::cout << "Main thread exiting .." << std::endl;
	if(reader_thread->joinable())
		reader_thread->join();
}

// Initialize the profiler when the library is loaded.
namespace
{
	Profiler profiler;
}

}  // namespace microservice_profile
