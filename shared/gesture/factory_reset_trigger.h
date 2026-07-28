#pragma once

namespace SmartSwitch
{

class IFactoryResetTrigger {
public:
	virtual ~IFactoryResetTrigger() = default;

	virtual void OnFactoryResetRequested() = 0;
};

}
