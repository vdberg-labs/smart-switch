#pragma once

#include "../../../shared/config/persistent_store.h"

namespace SmartSwitch
{

/* IPersistentStore backed by Matter's existing CHIP KVS
 * (chip::DeviceLayer::PersistedStorage::KeyValueStoreMgr())
 */
class MatterPersistentStore : public IPersistentStore {
public:
	bool Get(const char *key, void *buf, size_t bufSize, size_t &outReadSize) override;
	bool Put(const char *key, const void *data, size_t size) override;
};

}
