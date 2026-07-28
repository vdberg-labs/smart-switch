#include "matter_persistent_store.h"

#include <platform/KeyValueStoreManager.h>

using namespace chip::DeviceLayer::PersistedStorage;

namespace SmartSwitch
{

bool MatterPersistentStore::Get(const char *key, void *buf, size_t bufSize, size_t &outReadSize)
{
	return KeyValueStoreMgr().Get(key, buf, bufSize, &outReadSize) == CHIP_NO_ERROR;
}

bool MatterPersistentStore::Put(const char *key, const void *data, size_t size)
{
	return KeyValueStoreMgr().Put(key, data, size) == CHIP_NO_ERROR;
}

} /* namespace SmartSwitch */
