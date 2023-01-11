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
 * Inspired from https://github.com/fdoray/lttng-profile.git, by Francois Doray.
 */
#ifndef MICROSERVICE_PROFILER_MODULE_ABI_H_
#define MICROSERVICE_PROFILER_MODULE_ABI_H_

#define SERVICE_NAME_MAX_SIZE 28

#define MODULE_CONTROL_FILE "mod_ctl"
#define SPAN_LATENCY_TRACKER_PROC_PATH "/proc/latency-tracker-spans/" MODULE_CONTROL_FILE


enum microservice_profiler_module_cmd {
  MICROSERVICE_PROFILER_MODULE_REGISTER = 0,
  MICROSERVICE_PROFILER_MODULE_UNREGISTER = 1,
};

/*
 * Structure to send messages to the kernel module.
 */
struct microservice_profiler_module_msg {
  int cmd;                 /* Command */
  char service_name[SERVICE_NAME_MAX_SIZE];

  //long latency_threshold;  /* Latency threshold to identify long spans. */
} __attribute__((packed));


#define MICROSERVICE_PROFILER_MODULE_IOCTL  _IO(0xF6, 0x91)

#endif
