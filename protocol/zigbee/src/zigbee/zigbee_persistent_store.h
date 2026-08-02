#pragma once

#include "shared/config/persistent_store.h"

namespace SmartSwitch
{

class ZigbeePersistentStore : public IPersistentStore {
public:
	int Init();

	bool Get(const char *key, void *buf, size_t bufSize, size_t &outReadSize) override;
	bool Put(const char *key, const void *data, size_t size) override;

	static void EraseKey(const char *key);
};

}
