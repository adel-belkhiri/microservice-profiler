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
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <errno.h>
#include <string.h>
#include <sys/ioctl.h>

#include "microservice-profile-base/module_api.h"
#include "microservice-profile-base/module_abi.h"
#include "microservice-profile-base/memory.h"

struct microservice_profiler_module_state {
	int registered;
	FILE* fd;
};

static struct microservice_profiler_module_state* state = NULL;

static int microservice_profiler_module_ioctl(
	long latency_threshold, int cmd)
{
	struct microservice_profiler_module_msg info;

	if (!(state && state->fd))
		return -1;

	info.cmd = cmd;
	strncpy(info.service_name, "Test Service", SERVICE_NAME_MAX_SIZE);
	//info.latency_threshold = latency_threshold;

	return ioctl(state->fd->_fileno, MICROSERVICE_PROFILER_MODULE_IOCTL, &info);
}

/*
 * API functions
 */
int microservice_profiler_module_is_registered()
{
	return (state && state->registered);
}

int microservice_profiler_module_register(long latency_threshold)
{
	int ret = 0;

	/* if initialized, then reset configuration */
	if (microservice_profiler_module_is_registered()) {
		microservice_profiler_module_unregister();
	}
	state = calloc(1, sizeof(*state));
	if (!state) {
		return -ENOMEM;
	}

	/* open ioctl fd */
	state->fd = fopen(SPAN_LATENCY_TRACKER_PROC_PATH, "rw");
	if (!state->fd) {
		ret = -ENOENT;
		goto error_fd;
	}

	/* install signal handler before registration */
	ret = microservice_profiler_module_ioctl(
		latency_threshold, MICROSERVICE_PROFILER_MODULE_REGISTER);

	if (ret != 0)
		goto error_ioctl;
	state->registered = 1;
	return ret;

error_ioctl:
	fclose(state->fd);
error_fd:
	FREE(state);
	return ret;
}

int microservice_profiler_module_unregister()
{
	int ret = 0;
	if (microservice_profiler_module_is_registered()) {
		ret = microservice_profiler_module_ioctl(0, MICROSERVICE_PROFILER_MODULE_UNREGISTER);
		fclose(state->fd);
		FREE(state);
	}
	return ret;
}

int UnregisterMonitoringThread()
{
	int ret = 0;
	if (microservice_profiler_module_is_registered()) {
		ret = microservice_profiler_module_ioctl(0, MICROSERVICE_PROFILER_MODULE_UNREGISTER);
	}
	return ret;
}
