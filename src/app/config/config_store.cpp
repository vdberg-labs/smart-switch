#include "config_store.h"

#include <platform/KeyValueStoreManager.h>

#include <zephyr/sys/byteorder.h>

using namespace chip::DeviceLayer::PersistedStorage;

namespace SmartSwitch
{

void ConfigStore::Encode(const EngineConfig &cfg, uint8_t out[kWireSize])
{
	out[0] = kFormatVersion;
	out[1] = cfg.numChannels;
	sys_put_le16(static_cast<uint16_t>(cfg.pollMs), &out[2]);
	sys_put_le16(static_cast<uint16_t>(cfg.longPressMs), &out[4]);
	sys_put_le16(static_cast<uint16_t>(cfg.multiGapMs), &out[6]);
	out[8] = cfg.channels[0].maxTaps;
	out[9] = cfg.channels[0].holdEnabled ? 1 : 0;
	out[10] = cfg.channels[1].maxTaps;
	out[11] = cfg.channels[1].holdEnabled ? 1 : 0;
}

bool ConfigStore::Decode(const uint8_t *in, size_t len, EngineConfig &out)
{
	if (len != kWireSize || in[0] != kFormatVersion) {
		return false;
	}

	EngineConfig cfg{};
	cfg.numChannels = in[1];
	cfg.pollMs = sys_get_le16(&in[2]);
	cfg.longPressMs = sys_get_le16(&in[4]);
	cfg.multiGapMs = sys_get_le16(&in[6]);
	cfg.channels[0] = ChannelConfig{ in[8], in[9] != 0 };
	cfg.channels[1] = ChannelConfig{ in[10], in[11] != 0 };

	if (!cfg.IsValid()) {
		return false;
	}

	out = cfg;
	return true;
}

EngineConfig ConfigStore::Load()
{
	uint8_t buf[kWireSize];
	size_t readBytes = 0;
	CHIP_ERROR err = KeyValueStoreMgr().Get(kKvsKey, buf, sizeof(buf), &readBytes);
	if (err != CHIP_NO_ERROR) {
		return EngineConfig::Defaults();
	}

	EngineConfig cfg;
	if (!Decode(buf, readBytes, cfg)) {
		return EngineConfig::Defaults();
	}

	return cfg;
}

CHIP_ERROR ConfigStore::Save(const EngineConfig &cfg)
{
	if (!cfg.IsValid()) {
		return CHIP_ERROR_INVALID_ARGUMENT;
	}

	uint8_t buf[kWireSize];
	Encode(cfg, buf);
	return KeyValueStoreMgr().Put(kKvsKey, buf, sizeof(buf));
}

} /* namespace SmartSwitch */
