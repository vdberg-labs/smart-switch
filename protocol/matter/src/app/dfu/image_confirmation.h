#pragma once

#include <lib/core/CHIPError.h>

namespace SmartSwitch::ImageConfirmation
{

#ifdef CONFIG_MCUBOOT_IMG_MANAGER
/* Reboots a freshly swapped-in image, which reverts it, unless it gets confirmed in time. */
void ArmRevertTimer();

/* Confirms a freshly swapped-in image once the Matter server has been up for a while. */
CHIP_ERROR ConfirmOnceServerIsHealthy();
#else
inline void ArmRevertTimer() {}
inline CHIP_ERROR ConfirmOnceServerIsHealthy()
{
	return CHIP_NO_ERROR;
}
#endif

} /* namespace SmartSwitch::ImageConfirmation */
