#include "Dual_Tree.h"


dt_address dt_address::left_child() const {
	return{
		uint8_t(h + 1),
		A & ((~uint256_t(1)) << h)
	};
}

dt_address dt_address::right_child() const {
	return{
		uint8_t(h + 1),
		A | (uint256_t(1) << h)
	};
}

dt_address dt_address::left_parent() const {
	return{
		uint8_t(h - 1),
		A & ((~uint256_t(1)) << uint8_t(h - 1))
	};
}

dt_address dt_address::right_parent() const {
	return{
		uint8_t(h - 1),
		A | (uint256_t(1) << uint8_t(h - 1))
	};
}