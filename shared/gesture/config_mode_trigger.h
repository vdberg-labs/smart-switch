#pragma once

namespace SmartSwitch
{

/* Notified by the gesture engine when a channel has been held continuously past the
 * BLE-config-mode trigger threshold. This is a maintenance action, not a reportable gesture, so
 * it is kept separate from ISwitchProtocol. */
class IConfigModeTrigger {
public:
	virtual ~IConfigModeTrigger() = default;

	virtual void OnConfigModeRequested() = 0;
};

}
