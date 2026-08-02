#pragma once

#include "shared/gesture/network_removal_trigger.h"

namespace SmartSwitch
{

class ZigbeeNetworkRemoval : public INetworkRemovalTrigger {
public:
	void OnNetworkRemovalRequested() override;
};

}
