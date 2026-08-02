#include "zigbee_switch_protocol.h"
#include "zb_gesture_switch_cluster.h"

#include <zephyr/logging/log.h>

LOG_MODULE_DECLARE(app, CONFIG_LOG_DEFAULT_LEVEL);

namespace SmartSwitch
{
namespace
{
uint8_t WireGesture(Gesture gesture)
{
	switch (gesture) {
	case Gesture::InitialPress:
		return ZB_ZCL_GESTURE_SWITCH_GESTURE_INITIAL_PRESS;
	case Gesture::ShortRelease:
		return ZB_ZCL_GESTURE_SWITCH_GESTURE_SHORT_RELEASE;
	case Gesture::Tap:
		return ZB_ZCL_GESTURE_SWITCH_GESTURE_TAP;
	case Gesture::LongPress:
		return ZB_ZCL_GESTURE_SWITCH_GESTURE_LONG_PRESS;
	case Gesture::LongRelease:
		return ZB_ZCL_GESTURE_SWITCH_GESTURE_LONG_RELEASE;
	}
	return ZB_ZCL_GESTURE_SWITCH_GESTURE_INITIAL_PRESS;
}
} /* namespace */

void ZigbeeSwitchProtocol::OnGesture(uint8_t channel, Gesture gesture, uint8_t count)
{
	const uint16_t arg = (static_cast<uint16_t>(channel) << 12) |
			      (static_cast<uint16_t>(WireGesture(gesture)) << 8) | count;

	zb_ret_t err = zb_buf_get_out_delayed_ext(SendReport, arg, 0);
	if (err != RET_OK) {
		LOG_ERR("Gesture report dropped, no ZBOSS buffer available (err %d)", err);
	}
}

void ZigbeeSwitchProtocol::SendReport(zb_bufid_t bufid, zb_uint16_t arg)
{
	const zb_uint8_t channel = (arg >> 12) & 0xF;
	const zb_uint8_t gesture = (arg >> 8) & 0xF;
	const zb_uint8_t count = arg & 0xFF;
	const zb_uint8_t ep = kFirstEndpoint + channel;

	ZB_ZCL_GESTURE_SWITCH_SEND_REPORT_REQ(bufid, ep, OnSendConfirm, channel, gesture, count);
}

void ZigbeeSwitchProtocol::OnSendConfirm(zb_bufid_t bufid)
{
	zb_ret_t status = zb_buf_get_status(bufid);
	if (status != RET_OK) {
		LOG_ERR("Gesture report send failed (status %d)", status);
	} else {
		LOG_INF("Gesture report sent successfully");
	}
	zb_buf_free(bufid);
}

} /* namespace SmartSwitch */
