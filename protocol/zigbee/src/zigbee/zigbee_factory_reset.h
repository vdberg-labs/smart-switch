#pragma once

#include "shared/gesture/factory_reset_trigger.h"

namespace SmartSwitch
{

class ZigbeeFactoryReset : public IFactoryResetTrigger {
public:
	void OnFactoryResetRequested() override;
};

}
