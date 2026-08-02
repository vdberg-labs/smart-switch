#include "zigbee_gesture_config_attrs.h"

#include "shared/config/config_store.h"

#include <zephyr/logging/log.h>

LOG_MODULE_DECLARE(app, CONFIG_LOG_DEFAULT_LEVEL);

namespace SmartSwitch
{

void ZigbeeGestureConfigAttrs::Init(GestureEngine &engine, IPersistentStore &store)
{
	mEngine = &engine;
	mStore = &store;

	EngineConfig cfg = ConfigStore::Load(store);
	numChannels = cfg.numChannels;
	pollMs = static_cast<zb_uint16_t>(cfg.pollMs);
	longPressMs = static_cast<zb_uint16_t>(cfg.longPressMs);
	multiGapMs = static_cast<zb_uint16_t>(cfg.multiGapMs);
	ch1MaxTaps = cfg.channels[0].maxTaps;
	ch1HoldEnabled = cfg.channels[0].holdEnabled ? ZB_TRUE : ZB_FALSE;
	ch2MaxTaps = cfg.channels[1].maxTaps;
	ch2HoldEnabled = cfg.channels[1].holdEnabled ? ZB_TRUE : ZB_FALSE;
}

void ZigbeeGestureConfigAttrs::OnAttrWritten()
{
	if (!mEngine || !mStore) {
		return;
	}

	EngineConfig cfg;
	cfg.numChannels = numChannels;
	cfg.pollMs = pollMs;
	cfg.longPressMs = longPressMs;
	cfg.multiGapMs = multiGapMs;
	cfg.channels[0] = ChannelConfig{ ch1MaxTaps, ch1HoldEnabled != ZB_FALSE };
	cfg.channels[1] = ChannelConfig{ ch2MaxTaps, ch2HoldEnabled != ZB_FALSE };

	if (!cfg.IsValid()) {
		LOG_WRN("Gesture config attribute write rejected: resulting config is invalid");
		return;
	}

	if (!ConfigStore::Save(*mStore, cfg)) {
		LOG_WRN("Failed to persist gesture config from Zigbee attribute write");
	}
	mEngine->ApplyConfig(cfg);
	LOG_INF("Gesture config updated via Zigbee attribute write");
}

} /* namespace SmartSwitch */
