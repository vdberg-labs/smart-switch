#include "device_log.h"

#include <zephyr/drivers/hwinfo.h>
#include <zephyr/kernel.h>

#include <cstdio>
#include <cstring>

namespace SmartSwitch
{
namespace DeviceLog
{
namespace
{
constexpr uint32_t kMagic = 0x474F4C44; /* "DLOG" */
constexpr size_t kCapacity = 4096;

struct Region {
	uint32_t magic;
	uint32_t writeOffset;
	uint8_t wrapped;
	char data[kCapacity];
};

/* Excluded from Zephyr's BSS-zeroing (see CLAUDE.md's Persistent device log section for the
 * TF-M/linker source trace confirming this survives a warm reset on this board/SDK). */
__attribute__((section(".noinit"))) Region sRegion;

/* Regular (zero-per-boot) scratch buffer: Snapshot() linearizes the ring into here on demand, so
 * the BLE read callback never needs its own buffer. Deliberately, to avoid repeating the
 * BT-RX-workqueue stack overflow already hit once by a large stack buffer in a GATT callback. */
char sSnapshot[kCapacity + 1];

k_spinlock sLock;

void AppendLocked(const char *data, size_t len)
{
	for (size_t i = 0; i < len; i++) {
		sRegion.data[sRegion.writeOffset] = data[i];
		sRegion.writeOffset++;
		if (sRegion.writeOffset == kCapacity) {
			sRegion.writeOffset = 0;
			sRegion.wrapped = 1;
		}
	}
}

const char *ResetCauseString(uint32_t cause)
{
	if (cause & RESET_WATCHDOG) {
		return "WATCHDOG";
	}
	if (cause & RESET_SOFTWARE) {
		return "SOFTWARE";
	}
	if (cause & RESET_POR) {
		return "POWER-ON";
	}
	if (cause & RESET_PIN) {
		return "PIN";
	}
	if (cause == 0) {
		return "UNKNOWN";
	}
	return "OTHER";
}

} /* namespace */

const char *Snapshot(size_t &outLen)
{
	k_spinlock_key_t key = k_spin_lock(&sLock);
	size_t len;
	if (!sRegion.wrapped) {
		memcpy(sSnapshot, sRegion.data, sRegion.writeOffset);
		len = sRegion.writeOffset;
	} else {
		/* Ring has wrapped: oldest data starts at writeOffset (the next slot about to be
		 * overwritten), followed by the start of the buffer up to writeOffset again. */
		size_t tailLen = kCapacity - sRegion.writeOffset;
		memcpy(sSnapshot, sRegion.data + sRegion.writeOffset, tailLen);
		memcpy(sSnapshot + tailLen, sRegion.data, sRegion.writeOffset);
		len = kCapacity;
	}
	k_spin_unlock(&sLock, key);

	sSnapshot[len] = '\0';
	outLen = len;
	return sSnapshot;
}

/* Called by device_log_backend.c's process() callback for every formatted log line. */
extern "C" void SmartSwitch_DeviceLog_AppendRaw(const uint8_t *data, size_t len)
{
	k_spinlock_key_t key = k_spin_lock(&sLock);
	AppendLocked(reinterpret_cast<const char *>(data), len);
	k_spin_unlock(&sLock, key);
}

/* Called once by device_log_backend.c's init() callback, before any message is processed. */
extern "C" void SmartSwitch_DeviceLog_OnBackendInit(void)
{
	if (sRegion.magic != kMagic) {
		/* First-ever boot, or the retained region didn't survive (shouldn't happen on a warm
		 * reset per the .noinit trace above, but fail safe rather than trust garbage). */
		sRegion.magic = kMagic;
		sRegion.writeOffset = 0;
		sRegion.wrapped = 0;
	}

	uint32_t cause = 0;
	hwinfo_get_reset_cause(&cause);
	hwinfo_clear_reset_cause();

	char marker[64];
	int n = snprintf(marker, sizeof(marker), "\n===== BOOT (reset cause: %s) =====\n", ResetCauseString(cause));
	if (n > 0) {
		size_t len = static_cast<size_t>(n) < sizeof(marker) ? static_cast<size_t>(n) : sizeof(marker) - 1;
		SmartSwitch_DeviceLog_AppendRaw(reinterpret_cast<const uint8_t *>(marker), len);
	}
}

} /* namespace DeviceLog */
} /* namespace SmartSwitch */
