#pragma once

#include "shared/gesture/commissioning_info_provider.h"

namespace SmartSwitch
{

class ZigbeeNetworkInfo : public ICommissioningInfoProvider {
public:
	bool GetCommissioningInfoJson(char *buf, size_t bufSize, size_t &outLen) override;
};

}
