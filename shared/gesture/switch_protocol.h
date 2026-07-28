#pragma once

#include <cstdint>

namespace SmartSwitch
{

enum class Gesture {
	InitialPress, /* contact closed (button down) */
	ShortRelease, /* contact opened with no hold/multi-tap in progress */
	Tap,          /* a completed tap sequence; `count` = number of taps (1..maxTaps) */
	LongPress,    /* contact held past the hold threshold */
	LongRelease,  /* contact opened after a LongPress */
};

class ISwitchProtocol {
public:
	virtual ~ISwitchProtocol() = default;

	virtual void OnGesture(uint8_t channel, Gesture gesture, uint8_t count) = 0;
};

}
