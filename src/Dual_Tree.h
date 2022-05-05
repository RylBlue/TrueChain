#pragma once
#include "Essential_Types.h"


struct dt_address {
public:
	uint8_t h;
	uint256_t A;
	dt_address left_child() const;
	dt_address right_child() const;
	dt_address left_parent() const;
	dt_address right_parent() const;
};
