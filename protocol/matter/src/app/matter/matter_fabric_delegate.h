#pragma once

#include <credentials/FabricTable.h>

namespace SmartSwitch
{
class MatterFabricDelegate : public chip::FabricTable::Delegate {
public:
	void OnFabricRemoved(const chip::FabricTable &fabricTable, chip::FabricIndex fabricIndex) override;
	void OnFabricCommitted(const chip::FabricTable &fabricTable, chip::FabricIndex fabricIndex) override;
};

}
