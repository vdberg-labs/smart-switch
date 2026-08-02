#include "zigbee_network_info.h"

#include "zboss_cpp.h"

#include <cstdio>

namespace SmartSwitch
{
namespace
{

void FormatReversedHex(const zb_uint8_t addr[8], char *out)
{
	for (int i = 0; i < 8; i++) {
		snprintf(out + i * 2, 3, "%02x", addr[7 - i]);
	}
}

} /* namespace */

bool ZigbeeNetworkInfo::GetCommissioningInfoJson(char *buf, size_t bufSize, size_t &outLen)
{
	bool joined = ZB_JOINED();

	char extPanIdHex[17] = "0000000000000000";
	char ieeeAddrHex[17] = "0000000000000000";
	zb_uint16_t panId = 0;
	zb_uint16_t shortAddr = 0;
	zb_uint8_t channel = 0;

	if (joined) {
		zb_ext_pan_id_t extPanId;
		zb_ieee_addr_t ieeeAddr;

		zb_get_extended_pan_id(extPanId);
		zb_get_long_address(ieeeAddr);
		FormatReversedHex(extPanId, extPanIdHex);
		FormatReversedHex(ieeeAddr, ieeeAddrHex);

		panId = zb_get_pan_id();
		shortAddr = zb_get_short_address();
		channel = zb_get_current_channel();
	}

	int written = snprintf(buf, bufSize,
				"{\"protocol\":\"zigbee\",\"supported\":false,\"joined\":%s,\"role\":\"router\","
				"\"panId\":%u,\"extendedPanId\":\"%s\",\"channel\":%u,\"shortAddress\":%u,"
				"\"ieeeAddress\":\"%s\"}",
				joined ? "true" : "false", panId, extPanIdHex, channel, shortAddr, ieeeAddrHex);

	if (written < 0 || static_cast<size_t>(written) >= bufSize) {
		return false;
	}
	outLen = static_cast<size_t>(written);
	return true;
}

} /* namespace SmartSwitch */
