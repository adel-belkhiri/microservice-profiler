/*
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
#include <microservice_profile.h>

#include <iostream>
#include <signal.h>
#include <stddef.h>
#include <string.h>

#define UNW_LOCAL_ONLY
#include <libunwind.h>

#include "microservice-profile-base/profiling_timer.h"
#include "microservice-profile-base/signal_handler.h"

extern "C" {
#include "microservice-profile-base/module_api.h"
}

namespace
{

// Profiling timer period (microseconds).
const long kTimerPeriod = 1000;  // 1 ms

// Minimum duration that a thread must spend in a span
//  in order to generate an off_cpu_sample event (nanoseconds).
const long minSpanDuration = 100000; // 0.1 ms

// Signal
const int kSignal = SIGPROF;

bool InstallSignalHandler()
{
  struct sigaction sigact;
  struct sigaction sigact_old;

  memset(&sigact, 0, sizeof(sigact));
  sigact.sa_sigaction = &microservice_profile::SignalHandler;
  sigact.sa_flags = SA_RESTART | SA_SIGINFO;
  int ret = sigaction(kSignal, &sigact, &sigact_old);

  if (ret != 0)
  {
    sigaction(kSignal, &sigact_old, &sigact);
    return false;
  }

  return true;
}

}  // namespace

void StartMicroserviceProfile()
{
  // Use thread-local storage for libunwind caches, in order to avoid locks.
  int ret;
  //unw_set_caching_policy(unw_local_addr_space, UNW_CACHE_PER_THREAD);

  // Install the signal handler.
   if (!InstallSignalHandler())
  {
    std::cerr << "LTTng-profile: "
              << "Unable to install a SIGPROF signal handler. "
              << "LTTng-profile will not be enabled." << std::endl;
    return;
  }

  // Start profiling timer.
/*   if (!lttng_profile::StartProfilingTimer(kTimerPeriod))
  {
    std::cerr << "LTTng-profile: "
              << "Unable to start profiling timer. "
              << "No CPU profiling events will be generated." << std::endl;
  } */

  // Register the monitored application with the kernel module.
  ret = microservice_profiler_module_register(minSpanDuration);
  if ( ret != 0)
  {
    std::cerr << "Microservice-profiler: "
              << "Unable to communicate with the kernel module. "
              << "Microservice is not registered."
	      	  << " -- return value : " << ret
              << std::endl;
  }
}
