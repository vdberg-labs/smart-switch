#include "zigbee_persistent_store.h"

#include "zboss_cpp.h"

#include <zephyr/settings/settings.h>
#include <zephyr/logging/log.h>

#include <cstdio>
#include <cstring>

LOG_MODULE_DECLARE(app, CONFIG_LOG_DEFAULT_LEVEL);

namespace SmartSwitch
{
namespace
{
constexpr const char *kSubtreePrefix = "ssw";

void FullKey(const char *key, char *out, size_t outSize)
{
	snprintf(out, outSize, "%s/%s", kSubtreePrefix, key);
}

constexpr size_t kMaxKeyLen = 32;
constexpr size_t kMaxDataLen = 32;

char sPendingKey[kMaxKeyLen];
uint8_t sPendingData[kMaxDataLen];
size_t sPendingSize;

void DoPut(zb_uint8_t)
{
	int rc = settings_save_one(sPendingKey, sPendingData, sPendingSize);
	if (rc != 0) {
		LOG_WRN("Deferred settings write for '%s' failed: %d", sPendingKey, rc);
	}
}

struct DirectLoadCtx {
	void *buf;
	size_t bufSize;
	size_t outReadSize;
	bool found;
};

int DirectLoadCb(const char *key, size_t len, settings_read_cb read_cb, void *cb_arg, void *param)
{
	auto *ctx = static_cast<DirectLoadCtx *>(param);
	size_t toRead = len < ctx->bufSize ? len : ctx->bufSize;
	ssize_t rc = read_cb(cb_arg, ctx->buf, toRead);
	if (rc >= 0) {
		ctx->outReadSize = static_cast<size_t>(rc);
		ctx->found = true;
	}
	return 0;
}
} /* namespace */

int ZigbeePersistentStore::Init()
{
	return settings_subsys_init();
}

bool ZigbeePersistentStore::Get(const char *key, void *buf, size_t bufSize, size_t &outReadSize)
{
	char fullKey[32];
	FullKey(key, fullKey, sizeof(fullKey));

	DirectLoadCtx ctx{ buf, bufSize, 0, false };
	settings_load_subtree_direct(fullKey, DirectLoadCb, &ctx);

	outReadSize = ctx.outReadSize;
	return ctx.found;
}

bool ZigbeePersistentStore::Put(const char *key, const void *data, size_t size)
{
	if (size > kMaxDataLen) {
		LOG_WRN("Persistent store write for '%s' too large (%zu > %zu)", key, size, kMaxDataLen);
		return false;
	}

	FullKey(key, sPendingKey, sizeof(sPendingKey));
	memcpy(sPendingData, data, size);
	sPendingSize = size;

	ZB_SCHEDULE_APP_CALLBACK(DoPut, 0);
	return true;
}

void ZigbeePersistentStore::EraseKey(const char *key)
{
	char fullKey[32];
	FullKey(key, fullKey, sizeof(fullKey));

	int rc = settings_delete(fullKey);
	if (rc != 0) {
		LOG_WRN("Failed to erase persisted key '%s': %d", key, rc);
	}
}

} /* namespace SmartSwitch */
