#pragma once

#include <cstddef>

namespace SmartSwitch
{

class ICommissioningInfoProvider {
public:
	virtual ~ICommissioningInfoProvider() = default;

	virtual bool GetCommissioningInfoJson(char *buf, size_t bufSize, size_t &outLen) = 0;
};

}
