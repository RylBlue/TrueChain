#pragma once
#include "Database_Parse.h"

namespace mach {
	uint64_t NOP();
	uint64_t clear(uint8_t clear_reg);
	uint64_t reinterpret(uint8_t reg, uint8_t prim_type);
	uint64_t set_type_and_clear(uint8_t clear_reg);
	uint64_t swap(uint8_t lhs, uint8_t rhs, bool CMP);
	uint64_t set(uint8_t lhs, uint8_t rhs, bool CMP);
	uint64_t print(uint8_t reg, uint16_t print_flags);
	uint64_t add(uint8_t lhs, uint8_t rhs, uint8_t out_reg, bool CMP);
	uint64_t sub(uint8_t lhs, uint8_t rhs, uint8_t out_reg, bool CMP);
	uint64_t mul(uint8_t lhs, uint8_t rhs, uint8_t out_reg, bool CMP);
	uint64_t div(uint8_t lhs, uint8_t rhs, uint8_t div_reg, uint8_t mod_reg);
	uint64_t LSL(uint8_t lhs, uint8_t rhs, uint8_t out_reg, bool CMP);
	uint64_t LSR(uint8_t lhs, uint8_t rhs, uint8_t out_reg, bool CMP);
	uint64_t CMP(uint8_t lhs, uint8_t rhs);
	uint64_t parse_from_struct(uint8_t struct_reg, uint16_t var_index, uint8_t out_reg);
	uint64_t push_to_struct(uint8_t struct_reg, uint16_t var_index, uint8_t in_reg);
	uint64_t push(uint8_t reg1, uint8_t reg2, uint8_t reg3, uint8_t reg4);
	uint64_t pop(uint32_t count);
	uint64_t jump_fn(uint32_t new_program_count);
	uint64_t jump_return();
	uint64_t conditional_jump_return(uint16_t condition_mask);
	uint64_t jump(int32_t offset);
	uint64_t conditional_jump(uint16_t condition_mask, int16_t offset);
	uint64_t stop();
	uint64_t conditional_stop(uint16_t condition_mask);
	uint64_t struct_load(uint8_t load_reg);
	uint64_t hash_store(uint8_t store_reg);
	uint64_t hash_load(uint8_t load_reg);
	uint64_t gvar_store(uint8_t store_reg);
	uint64_t gvar_load(uint8_t load_reg);
	uint64_t set_hash_reg(uint8_t reg);
	uint64_t set_struct_reg(uint8_t reg);
	uint64_t set_gvar_reg(uint8_t reg);
	uint64_t set_index_reg(uint8_t reg);
	uint64_t set_hash_reg(uint32_t val);
	uint64_t set_struct_reg(uint32_t val);
	uint64_t set_gvar_reg(uint32_t val);
	uint64_t set_index_reg(uint32_t val);
	uint64_t set_short_immediate_target(uint8_t load_reg); //This command clears the reg
	uint64_t load_immediate_L(uint32_t Lw);
	uint64_t load_immediate_U(uint32_t Uw);
	uint64_t set_long_immediate_target(uint8_t load_reg, uint16_t prim_count);
	uint64_t process_immediate(uint64_t Dw);
	uint64_t append(uint8_t lower_array, uint8_t upper_array, uint8_t out_reg, bool CMP);
	uint64_t array_at_index(uint8_t array_reg, uint8_t out_reg, int16_t index_travel);
	uint64_t sub_array(uint8_t array_reg, uint8_t out_reg, uint16_t length);
	uint64_t sub_array_length_from_struct_reg(uint8_t array_reg, uint8_t out_reg);
	uint64_t add_immediate(uint8_t reg, int16_t immediate, bool CMP);
	uint64_t keccak256(uint8_t reg);
}