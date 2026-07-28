#pragma once

#include "shared/gesture/network_removal_trigger.h"

namespace SmartSwitch
{
class MatterNetworkRemoval : public INetworkRemovalTrigger {
public:
	void OnNetworkRemovalRequested() override;
};

}
