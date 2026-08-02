#pragma once

#include "shared/config/persistent_store.h"
#include "shared/gesture/gesture_engine.h"

#include "zboss_cpp.h"

namespace SmartSwitch
{

class ZigbeeGestureConfigAttrs {
public:
	void Init(GestureEngine &engine, IPersistentStore &store);

	void OnAttrWritten();

	zb_uint8_t numChannels;
	zb_uint16_t pollMs;
	zb_uint16_t longPressMs;
	zb_uint16_t multiGapMs;
	zb_uint8_t ch1MaxTaps;
	zb_bool_t ch1HoldEnabled;
	zb_uint8_t ch2MaxTaps;
	zb_bool_t ch2HoldEnabled;

private:
	GestureEngine *mEngine = nullptr;
	IPersistentStore *mStore = nullptr;
};

}
