#pragma once

namespace SmartSwitch
{

/* Whether the BLE config-mode advertising window is currently open. Queried by
 * GestureConfigService to decide whether the config characteristic(s) may be read/written.
 */
class IBleConfigModeState {
public:
	virtual ~IBleConfigModeState() = default;

	virtual bool IsActive() const = 0;
};

}
