#pragma once
#include <iostream>
#include <cstdint>

struct uint256_t {
public:
	uint8_t I[32];
	uint256_t();
	uint256_t(unsigned int a);
	uint256_t(std::string a);
	uint256_t(const char* const a);
	uint256_t(const uint8_t* a);
	uint256_t operator<<(uint8_t a) const;
	uint256_t operator>>(uint8_t a) const;
	uint256_t operator&(const uint256_t& a) const;
	uint256_t operator|(const uint256_t& a) const;
	uint256_t operator^(const uint256_t& a) const;
	uint256_t operator~() const;
	void operator=(unsigned int a);
	void operator=(const uint256_t& a);
	void operator=(std::string a); //Only if the string is of a valid form will this work "0xXXXX...XXXX"
	void operator=(const char* const a);
	void operator=(const uint8_t* a);
	void clear();
	operator uint32_t() const;
	bool operator==(const uint256_t a) const;
};

std::ostream& operator<<(std::ostream& os, const uint256_t& a);

namespace std {
	string to_string(const uint256_t& a);
}
