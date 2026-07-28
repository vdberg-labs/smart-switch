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
	/* Loads the persisted config. Falls back to EngineConfig::Defaults() if nothing is stored
	 * yet, or if the stored blob is the wrong size, fails to decode, or fails validation. */
	static EngineConfig Load(IPersistentStore &store);

	/* Validates cfg and persists it. Does not touch storage if validation fails. Returns false on
	 * invalid input or a storage write failure. */
	static bool Save(IPersistentStore &store, const EngineConfig &cfg);

	static void Encode(const EngineConfig &cfg, uint8_t out[kWireSize]);

	/* Returns false (and leaves out untouched) if len != kWireSize, the format version is
	 * unrecognized, or the decoded config fails EngineConfig::IsValid(). */
	static bool Decode(const uint8_t *in, size_t len, EngineConfig &out);

private:
	static constexpr const char *kKvsKey = "gst_cfg_v1";
	static constexpr uint8_t kFormatVersion = 1;
};

}
