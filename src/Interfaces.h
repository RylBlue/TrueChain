#pragma once
#include <cstdint>

class sizeable {
public:
	virtual uint64_t getSize() const = 0;
};