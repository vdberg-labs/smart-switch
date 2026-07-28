#pragma once

#include <cstddef>

namespace SmartSwitch
{

class IPersistentStore {
public:
	virtual ~IPersistentStore() = default;

	virtual bool Get(const char *key, void *buf, size_t bufSize, size_t &outReadSize) = 0;

	virtual bool Put(const char *key, const void *data, size_t size) = 0;
};

}
