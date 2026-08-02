#pragma once

#include "gesture_config.h"
#include "persistent_store.h"

#include <cstddef>
#include <cstdint>

namespace SmartSwitch
{

class ConfigStore {
public:
	static constexpr size_t kWireSize = 12;

	static constexpr const char *kKvsKey = "gst_cfg_v1";

	static EngineConfig Load(IPersistentStore &store);

	static bool Save(IPersistentStore &store, const EngineConfig &cfg);

	static void Encode(const EngineConfig &cfg, uint8_t out[kWireSize]);

	static bool Decode(const uint8_t *in, size_t len, EngineConfig &out);

private:
	static constexpr uint8_t kFormatVersion = 1;
};

}
