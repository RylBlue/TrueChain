#pragma once
#include "Database_Parse.h"

struct stack_instance {
public:
	uint8_t register_numb{0};
	uint32_t register_state{0};
	EasyArray<uint8_t> value;
	void operator=(const stack_instance& a);
};

struct machine_stack {
public:
	EasyArray<stack_instance> s;
	uint32_t stack_height{0};
	uint32_t stack_height_on_call{0};
	void operator=(const machine_stack& a);
	void function_jump();
	void function_return();
	void push_value(uint8_t reg, uint32_t reg_state, const EasyArray<uint8_t>& val);
	stack_instance* push_NULL();
	void pop_count(EasyArray<uint8_t>* registers, uint32_t* reg_states, uint256_t& hash_index_ref, uint32_t count, EasyArray<stack_instance>& sys_reg_return); //Also returns reg_state
};


struct all_instructions {
public:
	EasyArray<uint64_t> instructions;
};

struct Block_Machine {
public:
	Block_Machine();
	void operator=(const Block_Machine& a);

	void connect(database_state*);
	void disconnect();

	void clear_all();
	void clear_all_stack();
	void clear_all_registers();

	void run_till_stop(const EasyArray<uint64_t>&); //0xFFFF0000 in flag_reg, or out of code
	void run_next_line(uint64_t); 

	void clear(uint8_t clear_reg);
	void reinterpret(uint8_t reg, uint8_t prim_type);
	void set_type_and_clear(uint8_t clear_reg);
	void swap(uint8_t lhs, uint8_t rhs, bool CMP);
	void print(uint8_t reg, uint16_t print_flags);
	void add(uint8_t lhs, uint8_t rhs, uint8_t out_reg, bool CMP);
	void sub(uint8_t lhs, uint8_t rhs, uint8_t out_reg, bool CMP);
	void mul(uint8_t lhs, uint8_t rhs, uint8_t out_reg, bool CMP);
	void div(uint8_t lhs, uint8_t rhs, uint8_t div_reg, uint8_t mod_reg);
	void LSL(uint8_t lhs, uint8_t rhs, uint8_t out_reg, bool CMP);
	void LSR(uint8_t lhs, uint8_t rhs, uint8_t out_reg, bool CMP);
	void CMP(uint8_t lhs, uint8_t rhs);
	void parse_from_struct(uint8_t struct_reg, uint16_t var_index, uint8_t out_reg);
	void push_to_struct(uint8_t struct_reg, uint16_t var_index, uint8_t in_reg);
	void push(uint8_t reg1, uint8_t reg2, uint8_t reg3, uint8_t reg4);
	void pop(uint32_t count);
	void jump_fn(uint32_t new_program_count);
	void jump_return();
	void conditional_jump_return(uint16_t condition_mask);
	//Jump returns automatically pop the stack back to
	void jump(int32_t offset);
	void conditional_jump(uint16_t condition_mask, int16_t offset);
	void stop();
	void conditional_stop(uint16_t condition_mask);

	void struct_load(uint8_t load_reg);
	void hash_store(uint8_t store_reg);
	void hash_load(uint8_t load_reg);
	void gvar_store(uint8_t load_reg);
	void gvar_load(uint8_t load_reg);
	void set_hash_reg(uint8_t reg);
	void set_struct_reg(uint8_t reg);
	void set_gvar_reg(uint8_t reg);
	void set_index_reg(uint8_t reg);

	void set_hash_reg(uint32_t val);
	void set_struct_reg(uint32_t val);
	void set_gvar_reg(uint32_t val);
	void set_index_reg(uint32_t val);


	void set_short_immediate_target(uint8_t load_reg); //This command clears the reg
	void load_immediate_L(uint32_t Lw);
	void load_immediate_U(uint32_t Uw);

	void set_long_immediate_target(uint8_t load_reg, uint16_t prim_count);
	void process_immediate(uint64_t Dw);

	void append(uint8_t lower_array, uint8_t upper_array, uint8_t out_reg, bool CMP);
	void array_at_index(uint8_t array_reg, uint8_t out_reg, int16_t index_travel);
	void sub_array(uint8_t array_reg, uint8_t out_reg, uint16_t length);
	void sub_array_length_from_struct_reg(uint8_t array_reg, uint8_t out_reg);

	void add_immediate(uint8_t lhs, int16_t rhs, bool CMP);

	uint32_t get_program_counter() const;
private:
	void set_reg_mem(uint8_t regidx);
	uint32_t get_register_structure_type(uint8_t reg) const;
	std::string cmd_to_hex_string(uint32_t Uw, uint32_t Lw) const;
	std::string reg_to_hex_string(uint8_t regidx) const;
	all_instructions program;
	machine_stack stack;
	EasyArray<uint8_t> R[128]; //Normal registers
	uint32_t R_state[128];
	uint32_t hash_reg; //Hash map to access
	uint32_t struct_reg; //Structure index to access
	uint32_t gvar_reg; //Global variable to access
	uint32_t index_reg;
	uint32_t flag_reg; //Updated by comparisons
	uint32_t return_reg; //Where to jump to on function return
	uint256_t hash_index_reg; //Hash table access index
	uint32_t program_counter;
	bool immediate_load;
	uint8_t immediate_target;
	uint32_t immediate_byte_count;
	database_state* db;
};
/*

	case 0x80: return 0x13; //hash_reg
	case 0x81: return 0x13; //struct_reg
	case 0x82: return 0x13; //gvar_reg
	case 0x83: return 0x13; //index_reg
	case 0x84: return 0x13; //flag_reg
	case 0x85: return 0x13; //return_reg
	case 0x86: return 0x15; //hash_index_reg
	case 0x87: return 0x13; //program_counter
	case 0x88: return -1; //immediate_load bool
	case 0x89: return -1; //immediate_target

*/

//As the values inside of these registers can be any struct, array of primitives, or single primitive
//The operators that access these registers must specify the type of these values

