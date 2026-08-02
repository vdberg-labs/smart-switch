#include "zigbee_network_removal.h"

#include "zboss_cpp.h"

namespace SmartSwitch
{

void ZigbeeNetworkRemoval::OnNetworkRemovalRequested()
{
	ZB_SCHEDULE_APP_CALLBACK(zb_bdb_reset_via_local_action, 0);
}

} /* namespace SmartSwitch */
