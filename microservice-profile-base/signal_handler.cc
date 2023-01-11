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
 *
 *  Inspired from https://github.com/fdoray/lttng-profile.git, by Francois Doray.
 */
#include "microservice-profile-base/signal_handler.h"

#include "microservice-profile-base/get_monotonic_time.h"
#include "microservice-profile-base/stacktrace.h"

namespace microservice_profile
{

namespace
{

// Maximum stack size to capture.
const size_t kMaxStackSize = 60;

}  // namespace

void SignalHandler(int sig_nr, siginfo_t* info, void* context)
{
  uint64_t start = GetMonotonicTime();

  void* buffer[kMaxStackSize];
  size_t size = StackTrace(buffer, kMaxStackSize, context);

  uint64_t overhead = GetMonotonicTime() - start;

  if (info->si_code == SI_USER)
  {
    //tracepoint(lttng_profile,
    //           off_cpu_sample,
    //           size,
    //           buffer,
    //           overhead);
  }
  else
  {
    //tracepoint(lttng_profile,
    //           on_cpu_sample,
    //           size,
    //           buffer,
    //           overhead);
  }
}

}  // namespace microservice_profile
