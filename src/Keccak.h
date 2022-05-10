#pragma once
#include "Essential_Types.h"

//This is a haphazard implementation that does not appear to conform to most other keccak implementations
//But it appears that it will work well enough for testing the rest of the code
//TODO, use/ modify an actual Keccak library
uint256_t custom_keccak256(uint32_t byte_length, const uint8_t* data, const uint256_t& partial_hash); //uint32_t is used due to EasyArray<> only having uint32_t as a length specifier. This can be easily modified later (lots of small changes throught the whole codebase though).