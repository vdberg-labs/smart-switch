#include "zigbee_factory_reset.h"
#include "zigbee_persistent_store.h"

#include "shared/config/config_store.h"

#include "zboss_cpp.h"

#include <zephyr/sys/reboot.h>

namespace SmartSwitch
{
namespace
{
void RebootAfterReset(zb_uint8_t)
{
	sys_reboot(SYS_REBOOT_WARM);
}

void DoReset(zb_uint8_t)
{
	ZigbeePersistentStore::EraseKey(ConfigStore::kKvsKey);
	zb_bdb_reset_via_local_action(0);
	ZB_SCHEDULE_APP_ALARM(RebootAfterReset, 0, ZB_MILLISECONDS_TO_BEACON_INTERVAL(2000));
}
} /* namespace */

void ZigbeeFactoryReset::OnFactoryResetRequested()
{
	ZB_SCHEDULE_APP_CALLBACK(DoReset, 0);
}

} /* namespace SmartSwitch */
