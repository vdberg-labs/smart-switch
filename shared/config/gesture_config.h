#pragma once

#include <cstdint>

namespace SmartSwitch
{

inline constexpr uint8_t kMaxChannels = 2;

inline constexpr uint8_t kMinMaxTaps = 1;
inline constexpr uint8_t kMaxMaxTaps = 5;

inline constexpr uint32_t kMinPollMs = 1;
inline constexpr uint32_t kMaxPollMs = 50;

inline constexpr uint32_t kMinLongPressMs = 200;
inline constexpr uint32_t kMaxLongPressMs = 5000;

inline constexpr uint32_t kMinMultiGapMs = 50;
inline constexpr uint32_t kMaxMultiGapMs = 2000;

struct ChannelConfig {
	uint8_t maxTaps;
	bool holdEnabled;
};

struct EngineConfig {
	uint8_t numChannels;
	uint32_t pollMs;
	uint32_t longPressMs;
	uint32_t multiGapMs;
	ChannelConfig channels[kMaxChannels];

	static EngineConfig Defaults()
	{
		return EngineConfig{ 2, 5, 600, 250, { ChannelConfig{ 3, true }, ChannelConfig{ 3, true } } };
	}

	bool IsValid() const
	{
		if (numChannels < 1 || numChannels > kMaxChannels) {
			return false;
		}
		if (pollMs < kMinPollMs || pollMs > kMaxPollMs) {
			return false;
		}
		if (longPressMs < kMinLongPressMs || longPressMs > kMaxLongPressMs) {
			return false;
		}
		if (multiGapMs < kMinMultiGapMs || multiGapMs > kMaxMultiGapMs) {
			return false;
		}
		for (const auto &ch : channels) {
			if (ch.maxTaps < kMinMaxTaps || ch.maxTaps > kMaxMaxTaps) {
				return false;
			}
		}
		return true;
	}
};

}
