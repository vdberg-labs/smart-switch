#pragma once

namespace SmartSwitch
{

class IBleConfigModeState {
public:
	virtual ~IBleConfigModeState() = default;

	virtual bool IsActive() const = 0;

	virtual void SuppressResumeOnDisconnect() = 0;
};

}
