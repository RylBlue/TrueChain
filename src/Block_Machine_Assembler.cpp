#include "Block_Machine_Assembler.h"

//(instruction << 32) | (reg4<<24) | (reg3<<16) | (reg2<<8) | (reg1);

uint64_t mach::NOP() {
	return 0;
}

uint64_t mach::clear(uint8_t clear_reg) {
	return ((uint64_t)0b00000000000000000000000000000001 << 32) | (0 << 24) | (0 << 16) | (0 << 8) | (clear_reg);
}
uint64_t mach::reinterpret(uint8_t reg, uint8_t prim_type) {
	return ((uint64_t)0b00000000000000000000000000000010 << 32) | (0 << 24) | (0 << 16) | ((uint64_t)prim_type << 8) | (reg);
}
uint64_t mach::set_type_and_clear(uint8_t clear_reg) {
	return ((uint64_t)0b00000000000000000000000000000011 << 32) | (0 << 24) | (0 << 16) | (0 << 8) | (clear_reg);
}
uint64_t mach::swap(uint8_t lhs, uint8_t rhs, bool CMP) {
	return ((uint64_t)0b00000000000000000000000000000100 << 32) | (0 << 24) | (CMP ? 1 : 0 << 16) | ((uint64_t)rhs << 8) | (lhs);
}
uint64_t mach::print(uint8_t reg, uint16_t print_flags) {
	return ((uint64_t)0b00000000000000000000000000001000 << 32) | (0 << 24) | (0 << 16) | ((uint64_t)print_flags << 8) | (reg);
}
uint64_t mach::add(uint8_t lhs, uint8_t rhs, uint8_t out_reg, bool CMP) {
	return ((uint64_t)0b00000000000000000000000000010000 << 32) | (CMP ? 1 : 0 << 24) | ((uint64_t)out_reg << 16) | ((uint64_t)rhs << 8) | (lhs);
}
uint64_t mach::sub(uint8_t lhs, uint8_t rhs, uint8_t out_reg, bool CMP) {
	return ((uint64_t)0b00000000000000000000000000010001 << 32) | (CMP ? 1 : 0 << 24) | ((uint64_t)out_reg << 16) | ((uint64_t)rhs << 8) | (lhs);
}
uint64_t mach::mul(uint8_t lhs, uint8_t rhs, uint8_t out_reg, bool CMP) {
	return ((uint64_t)0b00000000000000000000000000010010 << 32) | (CMP ? 1 : 0 << 24) | ((uint64_t)out_reg << 16) | ((uint64_t)rhs << 8) | (lhs);
}
uint64_t mach::div(uint8_t lhs, uint8_t rhs, uint8_t div_reg, uint8_t mod_reg) {
	return ((uint64_t)0b00000000000000000000000000010011 << 32) | ((uint64_t)mod_reg << 24) | ((uint64_t)div_reg << 16) | ((uint64_t)rhs << 8) | (lhs);
}
uint64_t mach::LSL(uint8_t lhs, uint8_t rhs, uint8_t out_reg, bool CMP) {
	return ((uint64_t)0b00000000000000000000000000010100 << 32) | (CMP ? 1 : 0 << 24) | ((uint64_t)out_reg << 16) | ((uint64_t)rhs << 8) | (lhs);
}
uint64_t mach::LSR(uint8_t lhs, uint8_t rhs, uint8_t out_reg, bool CMP) {
	return ((uint64_t)0b00000000000000000000000000010101 << 32) | (CMP ? 1 : 0 << 24) | ((uint64_t)out_reg << 16) | ((uint64_t)rhs << 8) | (lhs);
}
uint64_t mach::CMP(uint8_t lhs, uint8_t rhs) {
	return ((uint64_t)0b00000000000000000000000000011000 << 32) | (0 << 24) | (0 << 16) | ((uint64_t)rhs << 8) | (lhs);
}
uint64_t mach::parse_from_struct(uint8_t struct_reg, uint16_t var_index, uint8_t out_reg) {
	return ((uint64_t)0b00000000000000000000000000100000 << 32) | ((uint64_t)out_reg << 24) | (0 << 16) | ((uint64_t)var_index << 8) | (struct_reg);
}
uint64_t mach::push_to_struct(uint8_t struct_reg, uint16_t var_index, uint8_t in_reg) {
	return ((uint64_t)0b00000000000000000000000000100001 << 32) | ((uint64_t)in_reg << 24) | (0 << 16) | ((uint64_t)var_index << 8) | (struct_reg);
}
uint64_t mach::push(uint8_t reg1, uint8_t reg2, uint8_t reg3, uint8_t reg4) {
	return ((uint64_t)0b00000000000000000000000000100010 << 32) | ((uint64_t)reg4 << 24) | ((uint64_t)reg3 << 16) | ((uint64_t)reg2 << 8) | (reg1);
}
uint64_t mach::pop(uint32_t count) {
	return ((uint64_t)0b00000000000000000000000000100100 << 32) | (0 << 24) | (0 << 16) | (0 << 8) | (count);
}
uint64_t mach::jump_fn(uint32_t new_program_count) {
	return ((uint64_t)0b00000000000000000000000001111111 << 32) | (0 << 24) | (0 << 16) | (0 << 8) | (new_program_count);
}
uint64_t mach::jump_return() {
	return ((uint64_t)0b00000000000000000000000001000000 << 32) | (0 << 24) | (0 << 16) | (0 << 8) | (0);
}
uint64_t mach::conditional_jump_return(uint16_t condition_mask) {
	return ((uint64_t)0b00000000000000000000000001000001 << 32) | (0 << 24) | (0 << 16) | (0 << 8) | (condition_mask);
}
uint64_t mach::jump(int32_t offset) {
	return ((uint64_t)0b00000000000000000000000001000010 << 32) | (0 << 24) | (0 << 16) | (0 << 8) | (uint32_t)(offset);
}
uint64_t mach::conditional_jump(uint16_t condition_mask, int16_t offset) {
	return ((uint64_t)0b00000000000000000000000001000100 << 32) | (0 << 24) | ((uint64_t)(uint16_t)offset << 16) | (0 << 8) | (condition_mask);
}
uint64_t mach::stop() {
	return ((uint64_t)0b00000000000000000000000001001000 << 32) | (0 << 24) | (0 << 16) | (0 << 8) | (0);
}
uint64_t mach::conditional_stop(uint16_t condition_mask) {
	return ((uint64_t)0b00000000000000000000000001010000 << 32) | (0 << 24) | (0 << 16) | (0 << 8) | (condition_mask);
}
uint64_t mach::struct_load(uint8_t load_reg) {
	return ((uint64_t)0b00000000000000000000000010000000 << 32) | (0 << 24) | (0 << 16) | (0 << 8) | (load_reg);
}
uint64_t mach::hash_store(uint8_t store_reg) {
	return ((uint64_t)0b00000000000000000000000010000001 << 32) | (0 << 24) | (0 << 16) | (0 << 8) | (store_reg);
}
uint64_t mach::hash_load(uint8_t load_reg) {
	return ((uint64_t)0b00000000000000000000000010000010 << 32) | (0 << 24) | (0 << 16) | (0 << 8) | (load_reg);
}
uint64_t mach::gvar_store(uint8_t store_reg) {
	return ((uint64_t)0b00000000000000000000000010000100 << 32) | (0 << 24) | (0 << 16) | (0 << 8) | (store_reg);
}
uint64_t mach::gvar_load(uint8_t load_reg) {
	return ((uint64_t)0b00000000000000000000000010001000 << 32) | (0 << 24) | (0 << 16) | (0 << 8) | (load_reg);
}
uint64_t mach::set_hash_reg(uint8_t reg) {
	return ((uint64_t)0b00000000000000000000000010010000 << 32) | (0 << 24) | (0 << 16) | (0 << 8) | (reg);
}
uint64_t mach::set_struct_reg(uint8_t reg) {
	return ((uint64_t)0b00000000000000000000000010100000 << 32) | (0 << 24) | (0 << 16) | (0 << 8) | (reg);
}
uint64_t mach::set_gvar_reg(uint8_t reg) {
	return ((uint64_t)0b00000000000000000000000011000000 << 32) | (0 << 24) | (0 << 16) | (0 << 8) | (reg);
}
uint64_t mach::set_index_reg(uint8_t reg) {
	return ((uint64_t)0b00000000000000000000000011010000 << 32) | (0 << 24) | (0 << 16) | (0 << 8) | (reg);
}
uint64_t mach::set_hash_reg(uint32_t val) {
	return ((uint64_t)0b00000000000000000000001000000000 << 32) | (0 << 24) | (0 << 16) | (0 << 8) | (val);
}
uint64_t mach::set_struct_reg(uint32_t val) {
	return ((uint64_t)0b00000000000000000000001000000001 << 32) | (0 << 24) | (0 << 16) | (0 << 8) | (val);
}
uint64_t mach::set_gvar_reg(uint32_t val) {
	return ((uint64_t)0b00000000000000000000001000000010 << 32) | (0 << 24) | (0 << 16) | (0 << 8) | (val);
}
uint64_t mach::set_index_reg(uint32_t val) {
	return ((uint64_t)0b00000000000000000000001000000011 << 32) | (0 << 24) | (0 << 16) | (0 << 8) | (val);
}
uint64_t mach::set_short_immediate_target(uint8_t load_reg) { //This command clears the reg
	return ((uint64_t)0b00000000000000000000000100000000 << 32) | (0 << 24) | (0 << 16) | (0 << 8) | (load_reg);
}
uint64_t mach::load_immediate_L(uint32_t Lw) {
	return ((uint64_t)0b00000000000000000000000100000001 << 32) | (0 << 24) | (0 << 16) | (0 << 8) | (Lw);
}
uint64_t mach::load_immediate_U(uint32_t Uw) {
	return ((uint64_t)0b00000000000000000000000100000010 << 32) | (0 << 24) | (0 << 16) | (0 << 8) | (Uw);
}
uint64_t mach::set_long_immediate_target(uint8_t load_reg, uint16_t prim_count) {
	return ((uint64_t)0b00000000000000000000000100000100 << 32) | (0 << 24) | (0 << 16) | ((uint64_t)prim_count << 8) | (load_reg);
}
uint64_t mach::process_immediate(uint64_t Dw) {
	return Dw;
}
uint64_t mach::append(uint8_t lower_array, uint8_t upper_array, uint8_t out_reg, bool CMP) {
	return ((uint64_t)0b00000000000000000000000100001000 << 32) | (CMP ? 1 : 0 << 24) | ((uint64_t)out_reg << 16) | ((uint64_t)upper_array << 8) | (lower_array);
}
uint64_t mach::array_at_index(uint8_t array_reg, uint8_t out_reg, int16_t index_travel) {
	return ((uint64_t)0b00000000000000000000000100001001 << 32) | (0 << 24) | ((uint64_t)(uint16_t)index_travel << 16) | ((uint64_t)out_reg << 8) | (array_reg);
}
uint64_t mach::sub_array(uint8_t array_reg, uint8_t out_reg, uint16_t length) {
	return ((uint64_t)0b00000000000000000000000100001010 << 32) | (0 << 24) | ((uint64_t)length << 16) | ((uint64_t)out_reg << 8) | (array_reg);
}
uint64_t mach::sub_array_length_from_struct_reg(uint8_t array_reg, uint8_t out_reg) {
	return ((uint64_t)0b00000000000000000000000100001011 << 32) | (0 << 24) | (0 << 16) | ((uint64_t)out_reg << 8) | (array_reg);
}

uint64_t mach::add_immediate(uint8_t reg, int16_t immediate, bool CMP) {
	return ((uint64_t)0b00000000000000000000010000000000 << 32) | (CMP ? 1 : 0 << 24) | (0 << 16) | ((uint64_t)(uint16_t)immediate << 8) | (reg);
}