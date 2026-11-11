#include "image_confirmation.h"

#include "app/matter_event_handler.h"

#include <platform/CHIPDeviceLayer.h>

#include <zephyr/dfu/mcuboot.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/reboot.h>

LOG_MODULE_DECLARE(app, CONFIG_CHIP_APP_LOG_LEVEL);

using namespace chip::DeviceLayer;

namespace
{
constexpr k_timeout_t kRevertTimeout = K_MINUTES(5);
constexpr k_timeout_t kHealthyPeriod = K_SECONDS(60);

bool sAwaitingConfirmation;
bool sMayWriteConfirmation;
k_timer sRevertTimer;
k_work_delayable sConfirmWork;

void Revert(k_timer *)
{
	sys_reboot(SYS_REBOOT_WARM);
}

void Confirm(k_work *)
{
	sMayWriteConfirmation = true;

	int error = boot_write_img_confirmed();
	if (error != 0) {
		LOG_ERR("Could not confirm the new firmware (%d), it will be reverted", error);
		return;
	}

	k_timer_stop(&sRevertTimer);
	LOG_INF("New firmware confirmed");
}

void OnMatterEvent(const ChipDeviceEvent *event, intptr_t)
{
	if (event->Type == DeviceEventType::kServerReady && sAwaitingConfirmation) {
		k_work_schedule(&sConfirmWork, kHealthyPeriod);
	}
}
} /* namespace */

/* Nordic's Matter startup confirms every image early in boot, so those calls are ignored until
 * the firmware has shown it works. Linked in with --wrap. */
extern "C" int __real_boot_write_img_confirmed(void);

extern "C" int __wrap_boot_write_img_confirmed(void)
{
	return sMayWriteConfirmation ? __real_boot_write_img_confirmed() : 0;
}

namespace SmartSwitch::ImageConfirmation
{

void ArmRevertTimer()
{
	if (mcuboot_swap_type() != BOOT_SWAP_TYPE_REVERT) {
		return;
	}

	LOG_INF("New firmware is on trial");
	sAwaitingConfirmation = true;
	k_work_init_delayable(&sConfirmWork, Confirm);
	k_timer_init(&sRevertTimer, Revert, nullptr);
	k_timer_start(&sRevertTimer, kRevertTimeout, K_NO_WAIT);
}

CHIP_ERROR ConfirmOnceServerIsHealthy()
{
	return Nrf::Matter::RegisterEventHandler(OnMatterEvent, 0);
}

} /* namespace SmartSwitch::ImageConfirmation */
