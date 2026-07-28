#include "matter_network_removal.h"

#include <app/server/Server.h>
#include <platform/CHIPDeviceLayer.h>

#include <zephyr/sys/reboot.h>

namespace SmartSwitch
{

void MatterNetworkRemoval::OnNetworkRemovalRequested()
{
	chip::DeviceLayer::SystemLayer().ScheduleLambda([]() {
		chip::Server::GetInstance().GetFabricTable().DeleteAllFabrics();
		sys_reboot(SYS_REBOOT_WARM);
	});
}

}
