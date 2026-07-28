#include "matter_commissioning_info.h"

#include <app/server/Server.h>
#include <platform/CHIPDeviceLayer.h>
#include <setup_payload/OnboardingCodesUtil.h>
#include <setup_payload/QRCodeSetupPayloadGenerator.h>

#include <cstdio>

using namespace chip;

namespace SmartSwitch
{

bool MatterCommissioningInfo::GetCommissioningInfoJson(char *buf, size_t bufSize, size_t &outLen)
{
	const bool windowOpen = Server::GetInstance().GetCommissioningWindowManager().IsCommissioningWindowOpen();
	const bool commissioned = Server::GetInstance().GetFabricTable().FabricCount() > 0;

	PayloadContents payload;
	if (GetPayloadContents(payload, RendezvousInformationFlags(RendezvousInformationFlag::kBLE)) != CHIP_NO_ERROR) {
		return false;
	}

	char qrCode[QRCodeBasicSetupPayloadGenerator::kMaxQRCodeBase38RepresentationLength + 1];
	MutableCharSpan qrCodeSpan(qrCode);
	if (GetQRCode(qrCodeSpan, payload) != CHIP_NO_ERROR) {
		return false;
	}

	char manualCode[kManualSetupLongCodeCharLength + 1];
	MutableCharSpan manualCodeSpan(manualCode);
	if (GetManualPairingCode(manualCodeSpan, payload) != CHIP_NO_ERROR) {
		return false;
	}

	int written = snprintf(buf, bufSize,
				"{\"protocol\":\"matter\",\"qrCode\":\"%s\",\"manualPairingCode\":\"%s\","
				"\"windowOpen\":%s,\"commissioned\":%s}",
				qrCode, manualCode, windowOpen ? "true" : "false", commissioned ? "true" : "false");
	if (written < 0 || static_cast<size_t>(written) >= bufSize) {
		return false;
	}

	outLen = static_cast<size_t>(written);
	return true;
}

}
