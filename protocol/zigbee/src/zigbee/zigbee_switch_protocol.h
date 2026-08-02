#pragma once

#include "shared/gesture/switch_protocol.h"

#include "zboss_cpp.h"

namespace SmartSwitch
{

class ZigbeeSwitchProtocol : public ISwitchProtocol {
public:
	void OnGesture(uint8_t channel, Gesture gesture, uint8_t count) override;

private:
	static constexpr zb_uint8_t kFirstEndpoint = 1;

	static void SendReport(zb_bufid_t bufid, zb_uint16_t arg);

	static void OnSendConfirm(zb_bufid_t bufid);
};

}
