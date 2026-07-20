#pragma once

#include "gesture/switch_protocol.h"

#include <app/util/basic-types.h>

namespace SmartSwitch
{
class MatterSwitchProtocol : public ISwitchProtocol {
public:
	void OnGesture(uint8_t channel, Gesture gesture, uint8_t count) override;

private:
	static constexpr chip::EndpointId kFirstEndpoint = 1;
};
}
