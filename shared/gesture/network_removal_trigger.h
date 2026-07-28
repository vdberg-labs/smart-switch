#pragma once

namespace SmartSwitch
{

class INetworkRemovalTrigger {
public:
	virtual ~INetworkRemovalTrigger() = default;

	virtual void OnNetworkRemovalRequested() = 0;
};

}
