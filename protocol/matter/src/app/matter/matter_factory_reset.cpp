#include "matter_factory_reset.h"

#include <app/server/Server.h>

namespace SmartSwitch
{

void MatterFactoryReset::OnFactoryResetRequested()
{
	chip::ServerScheduleFactoryReset();
}

} /* namespace SmartSwitch */
