#include "matter_fabric_delegate.h"

#include <zephyr/logging/log.h>

LOG_MODULE_DECLARE(app, CONFIG_CHIP_APP_LOG_LEVEL);

namespace SmartSwitch
{

void MatterFabricDelegate::OnFabricRemoved(const chip::FabricTable &fabricTable, chip::FabricIndex fabricIndex)
{
	LOG_INF("Matter fabric %u removed (fabrics remaining: %u)", static_cast<unsigned>(fabricIndex),
		static_cast<unsigned>(fabricTable.FabricCount()));
}

void MatterFabricDelegate::OnFabricCommitted(const chip::FabricTable &fabricTable, chip::FabricIndex fabricIndex)
{
	LOG_INF("Matter fabric %u committed (fabrics now: %u)", static_cast<unsigned>(fabricIndex),
		static_cast<unsigned>(fabricTable.FabricCount()));
}

}
