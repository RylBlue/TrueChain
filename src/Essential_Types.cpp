#include "Essential_Types.h"


uint256_t::uint256_t() {
	clear();
}

uint256_t::uint256_t(const uint8_t* a) {
	operator=(a);
}

uint256_t::uint256_t(unsigned int a) {
	operator=(a);
}

uint256_t::uint256_t(std::string a) {
	operator=(a);
}

uint256_t::uint256_t(const char* const a) {
	operator=(a);
}

uint256_t uint256_t::operator<<(uint8_t a) const {
	uint256_t to_return;
	unsigned int offset = a / 8;
	unsigned int final_shift = a % 8;
	uint8_t remainder = 0;
	for (unsigned int i = 0; (offset + i) < 32; ++i) {
		to_return.I[offset + i] = (I[i] << final_shift) | remainder;
		remainder = I[i] >> (8 - final_shift);
	}
	return to_return;
}

uint256_t uint256_t::operator>>(uint8_t a) const {
	uint256_t to_return;
	unsigned int offset = a / 8;
	unsigned int final_shift = a % 8;
	uint8_t remainder = 0;
	for (unsigned int i = 31 - offset; (offset + i) < 32 && i < 32; --i) {
		to_return.I[i] = (I[offset + i] >> final_shift) | remainder;
		remainder = I[offset + i] << (8 - final_shift);
	}
	return to_return;
}

uint256_t uint256_t::operator|(const uint256_t& a) const {
	uint256_t to_return;
	for (unsigned int i = 0; i < 32; ++i) {
		to_return.I[i] = I[i] | a.I[i];
	}
	return to_return;
}

uint256_t uint256_t::operator&(const uint256_t& a) const {
	uint256_t to_return;
	for (unsigned int i = 0; i < 32; ++i) {
		to_return.I[i] = I[i] & a.I[i];
	}
	return to_return;
}

uint256_t uint256_t::operator^(const uint256_t& a) const {
	uint256_t to_return;
	for (unsigned int i = 0; i < 32; ++i) {
		to_return.I[i] = I[i] ^ a.I[i];
	}
	return to_return;
}


uint256_t uint256_t::operator~() const {
	uint256_t to_return;
	for (unsigned int i = 0; i < 32; ++i) {
		to_return.I[i] = ~I[i];
	}
	return to_return;
}

void uint256_t::operator=(const uint256_t& a) {
	for (unsigned int i = 0; i < 32; ++i) {
		I[i] = a.I[i];
	}
}

void uint256_t::operator=(unsigned int a) {
	unsigned int i = 0;
	for (; i < 4; ++i) {
		I[i] = (uint8_t)((a >> (8 * i)) & 0xFF);
	}
	for (; i < 32; ++i) {
		I[i] = 0;
	}
}

void uint256_t::operator=(const uint8_t* a) {
	for (unsigned int i = 0; i < 32; ++i) {
		I[i] = a[i];
	}
}

std::ostream& operator<<(std::ostream& os, const uint256_t& a) {
	os << "0x";
	for (unsigned int i = 0; i < 32; ++i) {
		uint8_t temp = a.I[31 - i];
		switch (temp & 0xF0) {
		case 0x00: os << "0"; break;
		case 0x10: os << "1"; break;
		case 0x20: os << "2"; break;
		case 0x30: os << "3"; break;
		case 0x40: os << "4"; break;
		case 0x50: os << "5"; break;
		case 0x60: os << "6"; break;
		case 0x70: os << "7"; break;
		case 0x80: os << "8"; break;
		case 0x90: os << "9"; break;
		case 0xA0: os << "A"; break;
		case 0xB0: os << "B"; break;
		case 0xC0: os << "C"; break;
		case 0xD0: os << "D"; break;
		case 0xE0: os << "E"; break;
		case 0xF0: os << "F"; break;
		}
		switch (temp & 0x0F) {
		case 0x00: os << "0"; break;
		case 0x01: os << "1"; break;
		case 0x02: os << "2"; break;
		case 0x03: os << "3"; break;
		case 0x04: os << "4"; break;
		case 0x05: os << "5"; break;
		case 0x06: os << "6"; break;
		case 0x07: os << "7"; break;
		case 0x08: os << "8"; break;
		case 0x09: os << "9"; break;
		case 0x0A: os << "A"; break;
		case 0x0B: os << "B"; break;
		case 0x0C: os << "C"; break;
		case 0x0D: os << "D"; break;
		case 0x0E: os << "E"; break;
		case 0x0F: os << "F"; break;
		}
	}
	return os;
}

void uint256_t::operator=(std::string a) {
	//Only if the string is of a valid form will this work "0xXXXX...XXXX"
	clear();
	if (a.length() != 66 || a.at(0) != '0' || a.at(1) != 'x') {
		return;
	}
	for (unsigned int i = 2; i < 66; ++i) {
		char temp = a.at(i);
		if ((temp < 48 || temp>57) && (temp < 65 || temp>70)) {
			return;
		}
	}
	for (unsigned int i = 2; i < 66; ++i) {
		char c = a.at(i);
		uint8_t temp;
		switch (c) {
		case '0': temp = 0x00; break;
		case '1': temp = 0x01; break;
		case '2': temp = 0x02; break;
		case '3': temp = 0x03; break;
		case '4': temp = 0x04; break;
		case '5': temp = 0x05; break;
		case '6': temp = 0x06; break;
		case '7': temp = 0x07; break;
		case '8': temp = 0x08; break;
		case '9': temp = 0x09; break;
		case 'A': temp = 0x0A; break;
		case 'B': temp = 0x0B; break;
		case 'C': temp = 0x0C; break;
		case 'D': temp = 0x0D; break;
		case 'E': temp = 0x0E; break;
		case 'F': temp = 0x0F; break;
		}
		if ((i & 1) == 0) {
			temp <<= 4;
		}
		I[31 - (i - 2) / 2] |= temp;
	}

}

void uint256_t::operator=(const char* const a) {
	operator=(std::string(a));
}

void uint256_t::clear() {
	for (unsigned int i = 0; i < 32; ++i) {
		I[i] = 0;
	}
}

uint256_t::operator uint32_t() const {
	return (I[0] | (I[1] << 8) | (I[2] << 16) | (I[3] << 24));
}

bool uint256_t::operator==(const uint256_t a) const {
	for (unsigned int i = 0; i < 32; ++i) {
		if (I[i] != a.I[i]) {
			return false;
		}
	}
	return true;
}

#include <sstream>
std::string std::to_string(const uint256_t& a) {
	std::stringstream s("");
	s << a;
	return s.str();
}