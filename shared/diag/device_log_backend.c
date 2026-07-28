/* Custom Zephyr log backend that captures every LOG_INF/WRN/ERR message in the system (from any
 * module, not just this project's own code) into the retained device log (device_log.cpp). Modeled
 * directly on Zephyr's built-in zephyr/subsys/logging/backends/log_backend_ble.c, but sinks
 * formatted text into RAM instead of over BLE notifications. */

#include <zephyr/logging/log_backend.h>
#include <zephyr/logging/log_output.h>

#include <stddef.h>
#include <stdint.h>

/* Implemented in device_log.cpp (C++, namespaced) — these two are its plain-C bridge. */
extern void SmartSwitch_DeviceLog_AppendRaw(const uint8_t *data, size_t len);
extern void SmartSwitch_DeviceLog_OnBackendInit(void);

static uint8_t sOutputBuf[128];

static int device_log_char_out(uint8_t *data, size_t length, void *output_ctx)
{
	ARG_UNUSED(output_ctx);
	SmartSwitch_DeviceLog_AppendRaw(data, length);
	return (int)length;
}

LOG_OUTPUT_DEFINE(log_output_device_log, device_log_char_out, sOutputBuf, sizeof(sOutputBuf));

static void process(const struct log_backend *backend, union log_msg_generic *msg)
{
	ARG_UNUSED(backend);
	log_format_func_t log_output_func = log_format_func_t_get(LOG_OUTPUT_TEXT);
	log_output_func(&log_output_device_log, &msg->log, LOG_OUTPUT_FLAG_LEVEL | LOG_OUTPUT_FLAG_TIMESTAMP);
}

static void init(const struct log_backend *backend)
{
	ARG_UNUSED(backend);
	SmartSwitch_DeviceLog_OnBackendInit();
}

static void panic(const struct log_backend *backend)
{
	ARG_UNUSED(backend);
}

const struct log_backend_api device_log_backend_api = {
	.process = process,
	.panic = panic,
	.init = init,
};

LOG_BACKEND_DEFINE(device_log_backend, device_log_backend_api, true);
