#pragma once

namespace SmartSwitch
{
class IConfigModeTrigger {
public:
	virtual ~IConfigModeTrigger() = default;

	virtual void OnConfigModeRequested() = 0;
};
}
