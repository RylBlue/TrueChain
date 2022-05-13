#include <iomanip>
#include "Block_Machine.h"
#include "Logging.h"
#include "Keccak.h"

#define STACK_BUFFER_LENGTH 100000

void stack_instance::operator=(const stack_instance& a) {
	register_numb = a.register_numb;
	register_state = a.register_state;
	value = a.value;
}

void machine_stack::operator=(const machine_stack& a) {
	s = a.s;
	stack_height = a.stack_height;
	stack_height_on_call = a.stack_height_on_call;
}

void machine_stack::function_jump() {
	stack_instance old_return_height = { 0xFF, stack_height_on_call, {} };
	s.new_min_length(stack_height + 1 + ((stack_height % STACK_BUFFER_LENGTH) ? 0 : STACK_BUFFER_LENGTH), {});
	s.data[stack_height] = old_return_height;
	stack_height_on_call = stack_height;
	++stack_height;
}
void machine_stack::function_return() {
	Log::conditional_log_quit(stack_height == 0, __LINE__, __FILE__, "Stack return error#: 0");
	--stack_height;
	Log::conditional_log_quit(stack_height >= s.length, __LINE__, __FILE__, "Stack return error#: 1");
	stack_instance return_inst = s.data[stack_height];
	s.new_length(stack_height, {});
	Log::conditional_log_quit(return_inst.register_numb != 0xFF, __LINE__, __FILE__, "Stack return error#: 2");
	Log::conditional_log_quit(stack_height!=stack_height_on_call, __LINE__, __FILE__, "Stack return error#: 3");
	stack_height_on_call = return_inst.register_state;
}

void machine_stack::push_value(uint8_t reg, uint32_t reg_state, const EasyArray<uint8_t>& val) {
	s.new_min_length(stack_height + 1 + ((stack_height % STACK_BUFFER_LENGTH) ? 0 : STACK_BUFFER_LENGTH), {});
	++stack_height;
	s.data[stack_height - 1] = { reg, reg_state, {} };
	s.data[stack_height - 1].value = val;
}

stack_instance* machine_stack::push_NULL() {
	s.new_min_length(stack_height + 1 + ((stack_height % STACK_BUFFER_LENGTH) ? 0 : STACK_BUFFER_LENGTH), {});
	++stack_height;
	return &(s.data[stack_height -1]);
}

void machine_stack::pop_count(EasyArray<uint8_t>* registers, uint32_t* reg_states, uint256_t& hash_index_ref, uint32_t count, EasyArray<stack_instance>& sys_reg_return) {
	Log::conditional_log_quit(stack_height < count, __LINE__, __FILE__, "Stack over-pop error");
	Log::conditional_log_quit(stack_height_on_call > stack_height - count, __LINE__, __FILE__, "Stack pop-past-return error");
	for (uint32_t i = 0; i < count; ++i) {
		stack_instance* tmp_ptr = &s.data[stack_height - 1 - i];
		if (tmp_ptr->register_numb < 128) {
			//standard register (just swaps array ownership to prevent memory transfer overhead where possible)
			registers[tmp_ptr->register_numb].new_length(0, 0);
			registers[tmp_ptr->register_numb].data = tmp_ptr->value.data;
			registers[tmp_ptr->register_numb].length = tmp_ptr->value.length;
			tmp_ptr->value.data = nullptr;
			tmp_ptr->value.length = 0;
			reg_states[tmp_ptr->register_numb] = tmp_ptr->register_state;
			continue;
		}
		if (tmp_ptr->register_numb == 0x86) {
			Log::conditional_log_quit(tmp_ptr->value.length != 32, __LINE__, __FILE__, "Sysreg uint256_t error");
			hash_index_ref = (tmp_ptr->value).data;
			tmp_ptr->value = {};
			continue;
		}
		Log::conditional_log_quit(tmp_ptr->value.data != nullptr, __LINE__, __FILE__, "Sysreg stack state error");
		sys_reg_return.new_length(sys_reg_return.length + 1, *tmp_ptr);
	}
	stack_height -= count;
	if ((s.length - stack_height) > STACK_BUFFER_LENGTH + 1) {
		s.new_length(stack_height + 1 + ((stack_height % STACK_BUFFER_LENGTH) ? 0 : STACK_BUFFER_LENGTH), {});
	}
}


Block_Machine::Block_Machine() : hash_reg(0), struct_reg(0), gvar_reg(0), index_reg(0), flag_reg(0), return_reg(0), hash_index_reg((uint32_t)0), program_counter(0), immediate_load(false), immediate_target(0xFF), immediate_byte_count(0), db(nullptr) {
	for (unsigned int i = 0; i < 128; ++i) {
		R_state[i] = 0x10 | 0x03;
		set_reg_mem(i);
	}
}

void Block_Machine::operator=(const Block_Machine& a) {
	for (uint8_t i = 0; i < 128; ++i) {
		R[i] = a.R[i];
		R_state[i] = a.R_state[i];
	}
	hash_reg = a.hash_reg;
	struct_reg = a.struct_reg;
	gvar_reg = a.gvar_reg;
	index_reg = a.index_reg;
	flag_reg = a.flag_reg;
	return_reg = a.return_reg;
	hash_index_reg = a.hash_index_reg;
	program_counter = a.program_counter;
	immediate_load = a.immediate_load;
	immediate_target = a.immediate_target;
	immediate_byte_count = a.immediate_byte_count;
	db = a.db;

	stack = a.stack;
}

void Block_Machine::connect(database_state* a) {
	clear_all();
	db = a;
	Log::conditional_log_quit(db == nullptr, __LINE__, __FILE__, "nullptr passed into Block_Machine::connect");
}

void Block_Machine::disconnect() {
	clear_all();
	db = nullptr;
}

void Block_Machine::clear_all() {
	clear_all_stack();
	clear_all_registers();
}

void Block_Machine::clear_all_stack() {
	stack = machine_stack{};
}

void Block_Machine::clear_all_registers() {
	for (unsigned int i = 0; i < 128; ++i) {
		R_state[i] = 0x10 | 0x03;
		set_reg_mem(i);
	}
	hash_reg = 0;
	struct_reg = 0;
	gvar_reg = 0;
	index_reg = 0;
	flag_reg = 0;
	return_reg = 0;
	hash_index_reg = (uint32_t)0;
	program_counter = 0;
	immediate_load = false;
	immediate_target = 0xFF;
}

void Block_Machine::set_reg_mem(uint8_t reg_idx) {
	Log::conditional_log_quit(reg_idx >= 128, __LINE__, __FILE__, "Invalid reg index to set_reg_mem: "+std::to_string((unsigned int) reg_idx));

	uint32_t type = R_state[reg_idx];
	if (type< 0x1000000000000000) {
		R[reg_idx].new_length(0, 0);
		R[reg_idx].new_length(get_default_primitive_byte_size(R_state[reg_idx]), 0);
		return;
	}
	Log::conditional_log_quit(db == nullptr, __LINE__, __FILE__, "database_state has not yet been set!");
	const structure_definition* tmp = db->get_structure_def(R_state[reg_idx]);
	Log::conditional_log_quit(tmp == nullptr, __LINE__, __FILE__, "structure type not defined: " + std::to_string(R_state[reg_idx]));
	R[reg_idx].new_length(0, 0);
	R[reg_idx].new_length(tmp->struct_size, 0);
	return;
}

void Block_Machine::run_till_stop(const EasyArray<uint64_t>& a) {//0xFFFF0000 in flag_reg, or out of code
	while (true) {
		//std::cout<<program_counter<<std::endl;
		if (flag_reg >= 0xFFFF0000) {
			Log::log_info(__LINE__, __FILE__, "Stop instruction was executed: " + std::to_string(program_counter));
			return;
		}
		if (program_counter >= a.length) {
			Log::log_info(__LINE__, __FILE__, "End of instruction table reached: " + std::to_string(program_counter));
			return;
		}
		run_next_line(a.data[program_counter]);
	}
}

void Block_Machine::run_next_line(uint64_t a) {

	if (flag_reg >= 0xFFFF000) {
		Log::log_info(__LINE__, __FILE__, "Stop instruction was executed: " + std::to_string(program_counter));
		return;
	}

	if (immediate_load) {
		process_immediate(a);
		++program_counter;
		return;
	}
	
	uint32_t Uw = a>>32;
	uint32_t Lw = a & 0xFFFFFFFF;

	uint8_t reg1 = Lw & 0xFF;
	uint8_t reg2 = (Lw >> 8) & 0xFF;
	uint8_t reg3 = (Lw >> 16) & 0xFF;
	uint8_t reg4 = (Lw >> 24);

	uint16_t uint16_r12 = reg1 | (reg2 << 8);
	uint16_t uint16_r23 = reg2 | (reg3 << 8);
	uint16_t uint16_r34 = reg3 | (reg4 << 8);

	switch (Uw) {
	case 0b00000000000000000000000000000000: //nop
		++program_counter;
		return;
	case 0b00000000000000000000000000000001: //clear
		clear(reg1);
		++program_counter;
		return;
	case 0b00000000000000000000000000000010: //reinterpret
		reinterpret(reg1, reg2);
		++program_counter;
		return;
	case 0b00000000000000000000000000000011: //set_type_and_clear
		set_type_and_clear(reg1);
		++program_counter;
		return;
	case 0b00000000000000000000000000000100: //swap
		swap(reg1, reg2, bool(reg3) || bool(reg4));
		++program_counter;
		return;
	case 0b00000000000000000000000000000101: //set
		set(reg1, reg2, bool(reg3) || bool(reg4));
		++program_counter;
		return;
	case 0b00000000000000000000000000001000: //print
		print(reg1, uint16_r23);
		++program_counter;
		return;
	case 0b00000000000000000000000000010000: //add
		add(reg1, reg2, reg3, bool(reg4));
		++program_counter;
		return;
	case 0b00000000000000000000000000010001: //sub
		sub(reg1, reg2, reg3, bool(reg4));
		++program_counter;
		return;
	case 0b00000000000000000000000000010010: //mul
		mul(reg1, reg2, reg3, bool(reg4));
		++program_counter;
		return;
	case 0b00000000000000000000000000010011: //div
		div(reg1, reg2, reg3, reg4);
		++program_counter;
		return;
	case 0b00000000000000000000000000010100: //LSL
		LSL(reg1, reg2, reg3, bool(reg4));
		++program_counter;
		return;
	case 0b00000000000000000000000000010101: //LSR
		LSR(reg1, reg2, reg3, bool(reg4));
		++program_counter;
		return;
	case 0b00000000000000000000000000011000: //CMP
		CMP(reg1, reg2);
		++program_counter;
		return;
	case 0b00000000000000000000000000100000: //parse_from_struct
		parse_from_struct(reg1, uint16_r23, reg4);
		++program_counter;
		return;
	case 0b00000000000000000000000000100001: //push_to_struct
		push_to_struct(reg1, uint16_r23, reg4);
		++program_counter;
		return;
	case 0b00000000000000000000000000100010: //push
		push(reg1, reg2, reg3, reg4); //pushed in the order push(reg1), push(reg2), push(reg3), push(reg4)
		++program_counter;
		return;
	case 0b00000000000000000000000000100100: //pop
		pop(Lw);
		++program_counter;
		return;
	case 0b00000000000000000000000001000000: //jump_return
		jump_return();
		return;
	case 0b00000000000000000000000001000001: //conditional_jump_return
		conditional_jump_return(uint16_r12);
		return;
	case 0b00000000000000000000000001000010: //jump
		jump(Lw);
		return;
	case 0b00000000000000000000000001000100: //conditional_jump
		conditional_jump(uint16_r12, uint16_r34);
		return;
	case 0b00000000000000000000000001001000: //stop
		stop();
		++program_counter;
		return;
	case 0b00000000000000000000000001010000: //conditional_stop
		conditional_stop(uint16_r12);
		++program_counter;
		return;
	case 0b00000000000000000000000001111111: //jump_fn
		jump_fn(Lw);
		return;
	case 0b00000000000000000000000010000000: //struct_load
		struct_load(reg1);
		++program_counter;
		return;
	case 0b00000000000000000000000010000001: //hash_store
		hash_store(reg1);
		++program_counter;
		return;
	case 0b00000000000000000000000010000010: //hash_load
		hash_load(reg1);
		++program_counter;
		return;
	case 0b00000000000000000000000010000100: //gvar_store
		gvar_store(reg1);
		++program_counter;
		return;
	case 0b00000000000000000000000010001000: //gvar_load
		gvar_load(reg1);
		++program_counter;
		return;
	case 0b00000000000000000000000010010000: //set_hash_reg
		set_hash_reg(reg1);
		++program_counter;
		return;
	case 0b00000000000000000000000010100000: //set_struct_reg
		set_struct_reg(reg1);
		++program_counter;
		return;
	case 0b00000000000000000000000011000000: //set_gvar_reg
		set_gvar_reg(reg1);
		++program_counter;
		return;
	case 0b00000000000000000000000011010000: //set_gvar_reg
		set_index_reg(reg1);
		++program_counter;
		return;
	case 0b00000000000000000000000100000000: //set_short_immediate_target
		set_short_immediate_target(reg1);
		++program_counter;
		return;
	case 0b00000000000000000000000100000001: //load_immediate_L
		load_immediate_L(Lw);
		++program_counter;
		return;
	case 0b00000000000000000000000100000010: //load_immediate_U
		load_immediate_U(Uw);
		++program_counter;
		return;
	case 0b00000000000000000000000100000100: //set_long_immediate_target
		set_long_immediate_target(reg1, uint16_r23);
		++program_counter;
		return;
	case 0b00000000000000000000000100001000://append
		append(reg1, reg2, reg3, bool(reg4));
		++program_counter;
		return;
	case 0b00000000000000000000000100001001://array_at_index
		array_at_index(reg1, reg2, (int16_t)uint16_r34);
		++program_counter;
		return;
	case 0b00000000000000000000000100001010://sub_array
		sub_array(reg1, reg2, uint16_r34);
		++program_counter;
		return;
	case 0b00000000000000000000000100001011://sub_array_length_from_struct_reg
		sub_array_length_from_struct_reg(reg1, reg2);
		++program_counter;
		return;
	case 0b00000000000000000000001000000000: //set_hash_reg (immediate)
		set_hash_reg(Lw);
		++program_counter;
		return;
	case 0b00000000000000000000001000000001: //set_struct_reg (immediate)
		set_struct_reg(Lw);
		++program_counter;
		return;
	case 0b00000000000000000000001000000010: //set_gvar_reg (immediate)
		set_gvar_reg(Lw);
		++program_counter;
		return;
	case 0b00000000000000000000001000000011: //set_index_reg (immediate)
		set_index_reg(Lw);
		++program_counter;
		return;
	case 0b00000000000000000000010000000000:
		add_immediate(reg1, uint16_r23, (bool)reg4);
		++program_counter;
		return;
	case 0b00000000000000000000100000000000:
		keccak256(reg1);
		++program_counter;
		return;
	default:
		Log::log_error_quit(__LINE__, __FILE__, "Invalid command parsed: " + cmd_to_hex_string(Uw, Lw));
		return;
	}
}


void Block_Machine::clear(uint8_t clear_reg) {
	set_reg_mem(clear_reg);
}

void Block_Machine::reinterpret(uint8_t reg, uint8_t prim_type) {
	Log::conditional_log_quit(reg >= 128, __LINE__, __FILE__, "Invalid register in reinterpret: "+std::to_string(reg));
	Log::conditional_log_quit(validate_primitive_length(prim_type, R[reg].length), __LINE__, __FILE__, "Invalid main register cast: " + std::to_string(reg));
	R_state[reg] = prim_type;
}

void Block_Machine::set_type_and_clear(uint8_t clear_reg) {
	Log::conditional_log_quit(clear_reg >= 128, __LINE__, __FILE__, "Invalid register in set_type_and_clear: " + std::to_string(clear_reg));
	R_state[clear_reg] = struct_reg;
	set_reg_mem(clear_reg);
}

void Block_Machine::swap(uint8_t lhs, uint8_t rhs, bool CMP) {
	uint32_t temp_32;
	uint256_t temp_256;
	uint8_t* data;
	if(CMP){
		Block_Machine::CMP(lhs, rhs);
	}
	if (lhs < 128) {
		if (rhs < 128) {
			R[lhs].swap(R[rhs]);

			temp_32 = R_state[lhs];
			R_state[lhs] = R_state[rhs];
			R_state[rhs] = temp_32;
			return;
		}
		Log::conditional_log_quit(R_state[lhs]!=get_register_structure_type(rhs), __LINE__, __FILE__, "Invalid swap LHS");

		data = R[lhs].data;

		switch (rhs) {
		case 0x80:  //hash_reg
			temp_32 = hash_reg;
			hash_reg = *((uint32_t*)data);
			*((uint32_t*)data) = temp_32;
			return;
		case 0x81:  //struct_reg
			temp_32 = struct_reg;
			struct_reg = *((uint32_t*)data);
			*((uint32_t*)data) = temp_32;
			return;
		case 0x82:  //gvar_reg
			temp_32 = gvar_reg;
			gvar_reg = *((uint32_t*)data);
			*((uint32_t*)data) = temp_32;
			return;
		case 0x83:  //index_reg
			temp_32 = index_reg;
			index_reg = *((uint32_t*)data);
			*((uint32_t*)data) = temp_32;
			return;
		case 0x84:  //flag_reg
			temp_32 = flag_reg;
			flag_reg = *((uint32_t*)data);
			*((uint32_t*)data) = temp_32;
			return;
		case 0x85:  //return_reg
			temp_32 = return_reg;
			return_reg = *((uint32_t*)data);
			*((uint32_t*)data) = temp_32;
			return;
		case 0x86:  //hash_index_reg
			temp_256 = hash_index_reg;
			hash_index_reg = data;
			*((uint256_t*)data) = temp_256;
			return;
		case 0x87:  //program_counter
			temp_32 = program_counter;
			program_counter = *((uint32_t*)data);
			*((uint32_t*)data) = temp_32;
			return;
		default:
			Log::log_error_quit(__LINE__, __FILE__, "Invalid swap rhs register: "+ reg_to_hex_string(rhs));
			return;
		}
		return;
	}
	if (rhs < 128) {
		Log::conditional_log_quit(R_state[rhs] != get_register_structure_type(lhs), __LINE__, __FILE__, "Invalid swap RHS");
		data = R[rhs].data;
		switch (lhs) {
		case 0x80:  //hash_reg
			temp_32 = hash_reg;
			hash_reg = *((uint32_t*)data);
			*((uint32_t*)data) = temp_32;
			return;
		case 0x81:  //struct_reg
			temp_32 = struct_reg;
			struct_reg = *((uint32_t*)data);
			*((uint32_t*)data) = temp_32;
			return;
		case 0x82:  //gvar_reg
			temp_32 = gvar_reg;
			gvar_reg = *((uint32_t*)data);
			*((uint32_t*)data) = temp_32;
			return;
		case 0x83:  //index_reg
			temp_32 = index_reg;
			index_reg = *((uint32_t*)data);
			*((uint32_t*)data) = temp_32;
			return;
		case 0x84:  //flag_reg
			temp_32 = flag_reg;
			flag_reg = *((uint32_t*)data);
			*((uint32_t*)data) = temp_32;
			return;
		case 0x85:  //return_reg
			temp_32 = return_reg;
			return_reg = *((uint32_t*)data);
			*((uint32_t*)data) = temp_32;
			return;
		case 0x86:  //hash_index_reg
			temp_256 = hash_index_reg;
			hash_index_reg = data;
			*((uint256_t*)data) = temp_256;
			return;
		case 0x87:  //program_counter
			temp_32 = program_counter;
			program_counter = *((uint32_t*)data);
			*((uint32_t*)data) = temp_32;
			return;
		default:
			Log::log_error_quit(__LINE__, __FILE__, "Invalid swap rhs register: " + reg_to_hex_string(rhs));
			return;
		}
	}

	Log::log_warning(__LINE__, __FILE__, "Cannot swap two non base registers: lhs: " + reg_to_hex_string(lhs) + " rhs: " + reg_to_hex_string(rhs));
}

void Block_Machine::set(uint8_t lhs, uint8_t rhs, bool CMP){
	//lhs -> rhs
	uint8_t* data;
	if(CMP){
		Block_Machine::CMP(lhs, rhs);
	}
	if (lhs < 128) {
		if (rhs < 128) {
			R[rhs] = R[lhs];
			R_state[rhs] = R_state[lhs];
			return;
		}
		Log::conditional_log_quit(R_state[lhs]!=get_register_structure_type(rhs), __LINE__, __FILE__, "Invalid set LHS");

		data = R[lhs].data;

		switch (rhs) {
		case 0x80:  //hash_reg
			hash_reg = *((uint32_t*)data);
			return;
		case 0x81:  //struct_reg
			struct_reg = *((uint32_t*)data);
			return;
		case 0x82:  //gvar_reg
			gvar_reg = *((uint32_t*)data);
			return;
		case 0x83:  //index_reg
			index_reg = *((uint32_t*)data);
			return;
		case 0x84:  //flag_reg
			flag_reg = *((uint32_t*)data);
			return;
		case 0x85:  //return_reg
			return_reg = *((uint32_t*)data);
			return;
		case 0x86:  //hash_index_reg
			hash_index_reg = data;
			return;
		case 0x87:  //program_counter
			program_counter = *((uint32_t*)data);
			return;
		default:
			Log::log_error_quit(__LINE__, __FILE__, "Invalid set rhs register: "+ reg_to_hex_string(rhs));
			return;
		}
		return;
	}
	if (rhs < 128) {
		Log::conditional_log_quit(R_state[rhs] != get_register_structure_type(lhs), __LINE__, __FILE__, "Invalid set RHS");
		data = R[rhs].data;
		switch (lhs) {
		case 0x80:  //hash_reg
			*((uint32_t*)data) = hash_reg;
			return;
		case 0x81:  //struct_reg
			*((uint32_t*)data) = struct_reg;
			return;
		case 0x82:  //gvar_reg
			*((uint32_t*)data) = gvar_reg;
			return;
		case 0x83:  //index_reg
			*((uint32_t*)data) = index_reg;
			return;
		case 0x84:  //flag_reg
			*((uint32_t*)data) = flag_reg;
			return;
		case 0x85:  //return_reg
			*((uint32_t*)data) = return_reg;
			return;
		case 0x86:  //hash_index_reg
			*((uint256_t*)data) = hash_index_reg;
			return;
		case 0x87:  //program_counter
			*((uint32_t*)data) = program_counter;
			return;
		default:
			Log::log_error_quit(__LINE__, __FILE__, "Invalid set lhs register: " + reg_to_hex_string(lhs));
			return;
		}
	}

	Log::log_warning(__LINE__, __FILE__, "Cannot set two non base registers: lhs: " + reg_to_hex_string(lhs) + " rhs: " + reg_to_hex_string(rhs));
}

void Block_Machine::print(uint8_t reg, uint16_t print_flags) {
	if (reg<128) {
		if (R_state[reg]==0 || R_state[reg]==0x20) {
			for (uint32_t i = 0; i < R[reg].length; ++i) {
				std::cout << R[reg].data[i];
			}
			std::cout << std::endl;
			return;
		}
		if (R_state[reg] == 0x06) {
			std::cout << *(float*)R[reg].data << std::endl;
			return;
		}
		if (R_state[reg] == 0x07) {
			std::cout << *(double*)R[reg].data << std::endl;
			return;
		}
		std::cout<<"R#0x"<<reg_to_hex_string(reg)<<": L#"<<R[reg].length<<": V#0x";
		for (uint32_t i = 0; i < R[reg].length; ++i) {
			std::cout << reg_to_hex_string(R[reg].data[R[reg].length - 1 - i]);
		}
		std::cout << std::endl;
		return;
	}
	std::cout << "R#0x" << reg_to_hex_string(reg) << ": V#";
	switch (reg) {
	case 0x80:  //hash_reg
		std::cout << "0x" << std::setfill('0') << std::setw(8) << std::right << std::hex << hash_reg << std::dec << std::endl;
		return;
	case 0x81:  //struct_reg
		std::cout << "0x" << std::setfill('0') << std::setw(8) << std::right << std::hex << struct_reg << std::dec << std::endl;
		return;
	case 0x82:  //gvar_reg
		std::cout << "0x" << std::setfill('0') << std::setw(8) << std::right << std::hex << gvar_reg << std::dec << std::endl;
		return;
	case 0x83:  //index_reg
		std::cout << "0x" << std::setfill('0') << std::setw(8) << std::right << std::hex << index_reg << std::dec << std::endl;
		return;
	case 0x84:  //flag_reg
		std::cout << "0x" << std::setfill('0') << std::setw(8) << std::right << std::hex << flag_reg << std::dec << std::endl;
		return;
	case 0x85:  //return_reg
		std::cout << "0x" << std::setfill('0') << std::setw(8) << std::right << std::hex << return_reg << std::dec << std::endl;
		return;
	case 0x86:  //hash_index_reg
		std::cout << std::hex << hash_index_reg << std::dec << std::endl;
		return;
	case 0x87:  //program_counter
		std::cout << "0x" << std::setfill('0') << std::setw(8) << std::right << std::hex << program_counter << std::dec << std::endl;
		return;
	case 0x88:  //immediate_load bool
		std::cout << " BOOL: " << std::hex << immediate_load << std::dec << std::endl;
		return;
	case 0x89:  //immediate_target
		std::cout << "0x" << std::setfill('0') << std::setw(8) << std::right << std::hex << immediate_target << std::dec << std::endl;
		return;
	default:
		Log::log_warning(__LINE__, __FILE__, "Invalid register print request: "+reg_to_hex_string(reg));
	}
}

void Block_Machine::add(uint8_t lhs, uint8_t rhs, uint8_t out_reg, bool CMP) {
	Log::conditional_log_quit(lhs >= 128 || rhs >= 128 || out_reg>=128, __LINE__, __FILE__, "add register index error");
	if (CMP) {
		Block_Machine::CMP(lhs, rhs);
	}
	if ((R_state[lhs] == 0x07 || R_state[lhs] == 0x06) || (R_state[rhs] == 0x07 || R_state[rhs] == 0x06)) {
		double left = 0;
		double right = 0;

		switch (R_state[lhs]) {
		case 0x01: left = (double) *(  int8_t*)(R[lhs].data); break;
		case 0x11: left = (double) *( uint8_t*)(R[lhs].data); break;
		case 0x02: left = (double) *( int16_t*)(R[lhs].data); break;
		case 0x12: left = (double) *(uint16_t*)(R[lhs].data); break;
		case 0x03: left = (double) *( int32_t*)(R[lhs].data); break;
		case 0x13: left = (double) *(uint32_t*)(R[lhs].data); break;
		case 0x04: left = (double) *( int64_t*)(R[lhs].data); break;
		case 0x14: left = (double) *(uint64_t*)(R[lhs].data); break;
		case 0x06: left = (double) *(   float*)(R[lhs].data); break;
		case 0x07: left = (double) *(  double*)(R[lhs].data); break;
		default:
			Log::log_error_quit(__LINE__, __FILE__, "Add floating point type missmatch on lhs");
			return;
		}
		switch (R_state[rhs]) {
		case 0x01: right = (double) *(  int8_t*)(R[rhs].data); break;
		case 0x11: right = (double) *( uint8_t*)(R[rhs].data); break;
		case 0x02: right = (double) *( int16_t*)(R[rhs].data); break;
		case 0x12: right = (double) *(uint16_t*)(R[rhs].data); break;
		case 0x03: right = (double) *( int32_t*)(R[rhs].data); break;
		case 0x13: right = (double) *(uint32_t*)(R[rhs].data); break;
		case 0x04: right = (double) *( int64_t*)(R[rhs].data); break;
		case 0x14: right = (double) *(uint64_t*)(R[rhs].data); break;
		case 0x06: right = (double) *(   float*)(R[rhs].data); break;
		case 0x07: right = (double) *(  double*)(R[rhs].data); break;
		default:
			Log::log_error_quit(__LINE__, __FILE__, "Add floating point type missmatch on rhs");
			return;
		}
		if ((R_state[lhs]&0x06 + R_state[rhs] & 0x06) > (R_state[lhs] & 0x07 + R_state[rhs] & 0x07)) {
			R_state[out_reg] = 0x06;
			set_reg_mem(out_reg);
			*(float*)R[out_reg].data = (float)(left + right);
			return;
		}
		R_state[out_reg] = 0x07;
		set_reg_mem(out_reg);
		*(double*)R[out_reg].data = left + right;
		return;
	}
	bool extend_sign_L = (R_state[lhs] < 0x1000000000000000) && ((R_state[lhs] & 0x10) == 0);
	bool extend_sign_R = (R_state[rhs] < 0x1000000000000000) && ((R_state[rhs] & 0x10) == 0);

	uint16_t carry = 0;
	uint16_t l = 0;
	uint16_t r = 0;

	EasyArray<uint8_t> temp_array;
	temp_array.new_length(R[lhs].length>=R[rhs].length ? R[lhs].length : R[rhs].length, 0);
	
	uint32_t i = 0;

	for (; i < R[lhs].length && i < R[rhs].length; ++i) {
		l = R[lhs].data[i];
		r = R[rhs].data[i];
		carry += l + r;
		temp_array.data[i] = carry & 0xFF;
		carry >>= 8;
	}

	extend_sign_L = extend_sign_L && (l>>7);
	extend_sign_R = extend_sign_R && (r>>7);

	for (; i < R[lhs].length; ++i) {
		l = R[lhs].data[i];
		carry += l;
		if (extend_sign_R) {
			carry += 0xFF;
		}
		temp_array.data[i] = carry & 0xFF;
		carry >>= 8;
	}
	for (; i < R[rhs].length; ++i) {
		r = R[rhs].data[i];
		carry += r;
		if (extend_sign_L) {
			carry += 0xFF;
		}
		temp_array.data[i] = carry & 0xFF;
		carry >>= 8;
	}



	R_state[out_reg] = R[lhs].length >= R[rhs].length ? R_state[lhs] : R_state[rhs];
	R[out_reg].swap(temp_array);
}

void Block_Machine::sub(uint8_t lhs, uint8_t rhs, uint8_t out_reg, bool CMP) {
	Log::conditional_log_quit(lhs >= 128 || rhs >= 128 || out_reg >= 128, __LINE__, __FILE__, "sub register index error");
	if (CMP) {
		Block_Machine::CMP(lhs, rhs);
	}
	if ((R_state[lhs] == 0x07 || R_state[lhs] == 0x06) || (R_state[rhs] == 0x07 || R_state[rhs] == 0x06)) {
		double left = 0;
		double right = 0;

		switch (R_state[lhs]) {
		case 0x01: left = (double)*(int8_t*)(R[lhs].data); break;
		case 0x11: left = (double)*(uint8_t*)(R[lhs].data); break;
		case 0x02: left = (double)*(int16_t*)(R[lhs].data); break;
		case 0x12: left = (double)*(uint16_t*)(R[lhs].data); break;
		case 0x03: left = (double)*(int32_t*)(R[lhs].data); break;
		case 0x13: left = (double)*(uint32_t*)(R[lhs].data); break;
		case 0x04: left = (double)*(int64_t*)(R[lhs].data); break;
		case 0x14: left = (double)*(uint64_t*)(R[lhs].data); break;
		case 0x06: left = (double)*(float*)(R[lhs].data); break;
		case 0x07: left = (double)*(double*)(R[lhs].data); break;
		default:
			Log::log_error_quit(__LINE__, __FILE__, "Sub floating point type missmatch on lhs");
			return;
		}
		switch (R_state[rhs]) {
		case 0x01: right = (double)*(int8_t*)(R[rhs].data); break;
		case 0x11: right = (double)*(uint8_t*)(R[rhs].data); break;
		case 0x02: right = (double)*(int16_t*)(R[rhs].data); break;
		case 0x12: right = (double)*(uint16_t*)(R[rhs].data); break;
		case 0x03: right = (double)*(int32_t*)(R[rhs].data); break;
		case 0x13: right = (double)*(uint32_t*)(R[rhs].data); break;
		case 0x04: right = (double)*(int64_t*)(R[rhs].data); break;
		case 0x14: right = (double)*(uint64_t*)(R[rhs].data); break;
		case 0x06: right = (double)*(float*)(R[rhs].data); break;
		case 0x07: right = (double)*(double*)(R[rhs].data); break;
		default:
			Log::log_error_quit(__LINE__, __FILE__, "Sub floating point type missmatch on rhs");
			return;
		}
		if ((R_state[lhs] & 0x06 + R_state[rhs] & 0x06) > (R_state[lhs] & 0x07 + R_state[rhs] & 0x07)) {
			R_state[out_reg] = 0x06;
			set_reg_mem(out_reg);
			*(float*)R[out_reg].data = (float)(left + right);
			return;
		}
		R_state[out_reg] = 0x07;
		set_reg_mem(out_reg);
		*(double*)R[out_reg].data = left + right;
		return;
	}
	bool extend_sign_L = (R_state[lhs] < 0x1000000000000000) && ((R_state[lhs] & 0x10) == 0);
	bool extend_sign_R = true;

	uint16_t carry = 0;
	uint16_t l = 0;
	uint16_t r = 0;

	EasyArray<uint8_t> temp_array;
	temp_array.new_length(R[lhs].length >= R[rhs].length ? R[lhs].length : R[rhs].length, 0);

	uint32_t i = 0;

	for (; i < R[lhs].length && i < R[rhs].length; ++i) {
		l = R[lhs].data[i];
		if (i == 0) {
			r = (~(R[rhs].data[i]) + 1) & 0xFF;
		}
		else {
			r = (~(R[rhs].data[i])) & 0xFF;
		}
		carry += l + r;
		temp_array.data[i] = carry & 0xFF;
		carry >>= 8;
	}

	extend_sign_L = extend_sign_L && (l >> 7);
	extend_sign_R = extend_sign_R && (r >> 7);

	for (; i < R[lhs].length; ++i) {
		l = R[lhs].data[i];
		carry += l;
		if (extend_sign_R) {
			carry += 0xFF;
		}
		temp_array.data[i] = carry & 0xFF;
		carry >>= 8;
	}
	for (; i < R[rhs].length; ++i) {
		if (i == 0) {
			r = (~(R[rhs].data[i]) + 1) & 0xFF;
		}
		else {
			r = (~(R[rhs].data[i])) & 0xFF;
		}
		carry += r;
		if (extend_sign_L) {
			carry += 0xFF;
		}
		temp_array.data[i] = carry & 0xFF;
		carry >>= 8;
	}

	R_state[out_reg] = R[lhs].length >= R[rhs].length ? R_state[lhs] : R_state[rhs];
	R[out_reg].swap(temp_array);
}

void Block_Machine::mul(uint8_t lhs, uint8_t rhs, uint8_t out_reg, bool CMP) {

}

void Block_Machine::div(uint8_t lhs, uint8_t rhs, uint8_t div_reg, uint8_t mod_reg) {

}

void Block_Machine::LSL(uint8_t lhs, uint8_t rhs, uint8_t out_reg, bool CMP) {
	//rhs is treated as an immediate
	Log::conditional_log_quit(lhs >= 128 || out_reg>=128, __LINE__, __FILE__, "add register index error");

	EasyArray<uint8_t> tmp_arr;
	tmp_arr.new_length(R[lhs].length, 0);
	if ((rhs / 8u) >= R[lhs].length) {
		R[out_reg].swap(tmp_arr);
		R_state[out_reg] = R_state[lhs];
		return;
	}

	uint8_t temp_byte = 0;
	for (uint32_t i = 0; i < tmp_arr.length - rhs/8; ++i) {
		tmp_arr.data[i + rhs / 8] = temp_byte | (R[lhs].data[i] << (rhs % 8));
		temp_byte = R[lhs].data[i] >> (8 - (rhs % 8));
	}
	R[out_reg].swap(tmp_arr);
	R_state[out_reg] = R_state[lhs];
}

void Block_Machine::LSR(uint8_t lhs, uint8_t rhs, uint8_t out_reg, bool CMP) {
	//rhs is treated as an immediate
	Log::conditional_log_quit(lhs >= 128 || out_reg>=128, __LINE__, __FILE__, "add register index error");
	EasyArray<uint8_t> tmp_arr;
	tmp_arr.new_length(R[lhs].length, 0);
	if ((rhs / 8u) >= R[lhs].length) {
		R[out_reg].swap(tmp_arr);
		R_state[out_reg] = R_state[lhs];
		return;
	}

	uint8_t temp_byte = 0;
	for (uint32_t i = 0; i < tmp_arr.length - rhs / 8; ++i) {
		tmp_arr.data[tmp_arr.length - 1 - i - rhs / 8] = temp_byte | (R[lhs].data[tmp_arr.length - 1 - i] >> (rhs % 8));
		temp_byte = R[lhs].data[tmp_arr.length - 1 - i] << (8 - (rhs % 8));
	}
	R[out_reg].swap(tmp_arr);
	R_state[out_reg] = R_state[lhs];
}

void Block_Machine::CMP(uint8_t lhs, uint8_t rhs) {
	Log::conditional_log_quit(lhs>=128 || rhs>=128, __LINE__, __FILE__, "CMP register index error");
	if ((R_state[lhs] == 0x07 || R_state[lhs] == 0x06) || (R_state[rhs] == 0x07 || R_state[rhs] == 0x06)) {
		double left = 0;
		double right = 0;

		switch (R_state[lhs]) {
		case 0x01: left = (double) *(int8_t*)(R[lhs].data); break;
		case 0x11: left = (double) *(uint8_t*)(R[lhs].data); break;
		case 0x02: left = (double) *(int16_t*)(R[lhs].data); break;
		case 0x12: left = (double) *(uint16_t*)(R[lhs].data); break;
		case 0x03: left = (double) *(int32_t*)(R[lhs].data); break;
		case 0x13: left = (double) *(uint32_t*)(R[lhs].data); break;
		case 0x04: left = (double) *(int64_t*)(R[lhs].data); break;
		case 0x14: left = (double) *(uint64_t*)(R[lhs].data); break;
		case 0x06: left = (double) *(float*)(R[lhs].data); break;
		case 0x07: left = (double) *(double*)(R[lhs].data); break;
		default:
			Log::log_error_quit(__LINE__, __FILE__, "CMP floating point type missmatch on lhs");
			return;
		}
		switch (R_state[rhs]) {
		case 0x01: right = (double) *(int8_t*)(R[rhs].data); break;
		case 0x11: right = (double) *(uint8_t*)(R[rhs].data); break;
		case 0x02: right = (double) *(int16_t*)(R[rhs].data); break;
		case 0x12: right = (double) *(uint16_t*)(R[rhs].data); break;
		case 0x03: right = (double) *(int32_t*)(R[rhs].data); break;
		case 0x13: right = (double) *(uint32_t*)(R[rhs].data); break;
		case 0x04: right = (double) *(int64_t*)(R[rhs].data); break;
		case 0x14: right = (double) *(uint64_t*)(R[rhs].data); break;
		case 0x06: right = (double) *(float*)(R[rhs].data); break;
		case 0x07: right = (double) *(double*)(R[rhs].data); break;
		default:
			Log::log_error_quit(__LINE__, __FILE__, "CMP floating point type missmatch on lhs");
			return;
		}

		flag_reg &= 0xFF00;
		flag_reg |= left <  right ? 0x01 : 0;
		flag_reg |= left <= right ? 0x02 : 0;
		flag_reg |= left == right ? 0x04 : 0;
		flag_reg |= left >= right ? 0x08 : 0;
		flag_reg |= left >  right ? 0x10 : 0;
		flag_reg |= left == 0     ? 0x20 : 0;
		flag_reg |= right == 0    ? 0x40 : 0;
		flag_reg |= R[lhs].length == R[rhs].length ? 0x80 : 0;
		return;
	}
	flag_reg &= 0xFF00;
	flag_reg |= R[lhs].length == R[rhs].length ? 0x80 : 0;
	uint32_t i = 0;
	uint32_t true_mask = 0x00FF;
	uint8_t greater_state = 0;
	for (; i < R[lhs].length && i < R[rhs].length; ++i) {
		uint8_t lbyte = R[lhs].data[i];
		uint8_t rbyte = R[rhs].data[i];
		if (lbyte != rbyte) {
			true_mask &= ~0x04;
		}
		if (lbyte) {
			true_mask &= ~0x20;
		}
		if (rbyte) {
			true_mask &= ~0x40;
		}
		if (lbyte>rbyte) {
			greater_state = 0x01;
		}
		if (rbyte > lbyte) {
			greater_state = 0x02;
		}
	}
	for (; i < R[lhs].length; ++i) {
		if (R[lhs].data[i]) {
			true_mask &= ~0x04;
			true_mask &= ~0x20;
			greater_state = 0x01;
			break;
		}
	}
	for (; i < R[rhs].length; ++i) {
		if (R[rhs].data[i]) {
			true_mask &= ~0x04;
			true_mask &= ~0x40;
			greater_state = 0x02;
			break;
		}
	}
	switch (greater_state) {
	case 0x00: true_mask &= ~0x01; true_mask &= ~0x10; break;
	case 0x01: true_mask &= ~0x01; true_mask &= ~0x02; break;
	case 0x02: true_mask &= ~0x10; true_mask &= ~0x08; break;
	}

	if (R_state[lhs] >= 0x1000000000000000 || R_state[rhs] >= 0x1000000000000000 || ((R_state[lhs]&0x10) && (R_state[rhs]&0x10)) || R_state[lhs]&0x20 || R_state[rhs]&0x20) {
		flag_reg |= true_mask;
		return;
	}

	bool left_sign = true;
	bool right_sign = true;

	if (!(R_state[lhs] & 0x10) && R[lhs].length > 0) {
		left_sign = !(R[lhs].data[R[lhs].length - 1] & 0x80);
	}
	if (!(R_state[rhs] & 0x10) && R[rhs].length > 0) {
		right_sign = !(R[rhs].data[R[rhs].length - 1] & 0x80);
	}

	if (left_sign==right_sign) {
		flag_reg |= true_mask;
		return;
	}
	if (!left_sign) {
		true_mask &= 0xC0;
		true_mask |= 0x03;
	}
	if (!right_sign) {
		true_mask &= 0xA0;
		true_mask |= 0x18;
	}
	flag_reg |= true_mask;
}

void Block_Machine::parse_from_struct(uint8_t struct_reg, uint16_t var_index, uint8_t out_reg) {
	Log::conditional_log_quit(db == nullptr, __LINE__, __FILE__, "parse_from_struct no database connection!");
	Log::conditional_log_quit(struct_reg >= 128 || out_reg >= 128, __LINE__, __FILE__, "parse_from_struct invalid register indexes");
	Log::conditional_log_quit(R_state[struct_reg] < 0x1000000000000000, __LINE__, __FILE__, "parse_from_struct a primitive is not a struct!");
	const structure_definition* struct_def = (db->get_structure_def(R_state[struct_reg]));
	Log::conditional_log_quit(struct_def == nullptr, __LINE__, __FILE__, "parse_from_struct nullptr for struct_def");
	Log::conditional_log_quit(struct_def->element_count <= var_index, __LINE__, __FILE__, "parse_from_struct invalid struct var index");
	R_state[out_reg] = struct_def->element_type.data[var_index].type_index;
	EasyArray<uint8_t> temp_arr;
	temp_arr.clear_copy_from(struct_def->element_type.data[var_index].length, &(R[struct_reg].data[struct_def->byte_offset.data[var_index]]));
	R[out_reg].swap(temp_arr);
}

void Block_Machine::push_to_struct(uint8_t struct_reg, uint16_t var_index, uint8_t in_reg) {
	Log::conditional_log_quit(db == nullptr, __LINE__, __FILE__, "push_to_struct no database connection!");
	Log::conditional_log_quit(struct_reg >= 128 || in_reg >= 128, __LINE__, __FILE__, "push_to_struct invalid register indexes");
	Log::conditional_log_quit(R_state[struct_reg] < 0x1000000000000000, __LINE__, __FILE__, "push_to_struct a primitive is not a struct!");
	const structure_definition* struct_def = (db->get_structure_def(R_state[struct_reg]));
	Log::conditional_log_quit(struct_def == nullptr, __LINE__, __FILE__, "push_to_struct nullptr for struct_def");
	Log::conditional_log_quit(struct_def->element_count <= var_index, __LINE__, __FILE__, "push_to_struct invalid struct var index");
	Log::conditional_log_quit(struct_def->element_type.data[var_index].type_index != R_state[in_reg], __LINE__, __FILE__, "push_to_struct inavlid in_reg data type!");
	Log::conditional_log_quit(struct_def->element_type.data[var_index].length < R[in_reg].length, __LINE__, __FILE__, "push_to_struct inavlid in_reg data length!");

	EasyArray<uint8_t> temp_arr;
	temp_arr.data = &(R[struct_reg].data[struct_def->byte_offset.data[var_index]]);
	temp_arr.length = R[in_reg].length;

	temp_arr.copy_from(R[in_reg].data);

	temp_arr.data = nullptr; //Stops the scope exit from causing a segfault :)
	temp_arr.length = 0;
}

void Block_Machine::push(uint8_t reg1, uint8_t reg2, uint8_t reg3, uint8_t reg4) {
	for (int i = 0; i < 4; ++i) {
		uint8_t temp_reg = 0xFF;
		switch (i) {
		case 0: temp_reg = reg1; break;
		case 1: temp_reg = reg2; break;
		case 2: temp_reg = reg3; break;
		case 3: temp_reg = reg4; break;
		}
		if (temp_reg == 0xFF) {
			continue;
		}

		if (temp_reg<128) {
			stack_instance* tmp_ptr = stack.push_NULL();
			tmp_ptr->register_numb = temp_reg;
			tmp_ptr->register_state = R_state[temp_reg];
			tmp_ptr->value.data = R[temp_reg].data;
			tmp_ptr->value.length = R[temp_reg].length;
			R[temp_reg].data = nullptr;
			R[temp_reg].length = 0;
			R_state[temp_reg] = 0x13;
			set_reg_mem(temp_reg);
			continue;
		}
		if (temp_reg==0x86) {
			EasyArray<uint8_t> temp_arr = EasyArray<uint8_t>(32);
			*((uint256_t*)temp_arr.data) = hash_index_reg;
			stack.push_value(temp_reg, 0x15, temp_arr);
			hash_index_reg = (uint32_t)0;
			continue;
		}
		uint32_t to_push = 0;
		switch (temp_reg) {
		case 0x80:stack.push_value(temp_reg, hash_reg, {});  hash_reg = 0; continue;  //hash_reg
		case 0x81:stack.push_value(temp_reg, struct_reg, {}); struct_reg = 0; continue;  //struct_reg
		case 0x82:stack.push_value(temp_reg, gvar_reg, {}); gvar_reg = 0;  continue;  //gvar_reg
		case 0x83:stack.push_value(temp_reg, index_reg, {}); index_reg = 0; continue;  //index_reg
		case 0x84:stack.push_value(temp_reg, flag_reg, {}); flag_reg = 0; continue;  //flag_reg
		case 0x85:stack.push_value(temp_reg, return_reg, {}); return_reg = 0; continue;  //return_reg
		case 0x87:stack.push_value(temp_reg, program_counter, {}); program_counter = 0; continue;  //program_counter
		}
		Log::log_error_quit(__LINE__, __FILE__, "Invalid push register request: "+reg_to_hex_string(temp_reg));
	}
}

void Block_Machine::pop(uint32_t count) {
	EasyArray<stack_instance> system_reg_stack;
	stack.pop_count(R, R_state, hash_index_reg, count, system_reg_stack);
	for (uint32_t i = 0; i < system_reg_stack.length; ++i) {
		switch (system_reg_stack.data[i].register_numb) {
		case 0x80: hash_reg = system_reg_stack.data[i].register_state; continue;//hash_reg
		case 0x81: struct_reg = system_reg_stack.data[i].register_state; continue;//struct_reg
		case 0x82: gvar_reg = system_reg_stack.data[i].register_state; continue;//gvar_reg
		case 0x83: index_reg = system_reg_stack.data[i].register_state; continue;//gvar_reg
		case 0x84: flag_reg = system_reg_stack.data[i].register_state; continue;//flag_reg
		case 0x85: return_reg = system_reg_stack.data[i].register_state; continue;//return_reg
		case 0x87: program_counter = system_reg_stack.data[i].register_state; continue;//program_counter
		}
		Log::log_error_quit(__LINE__, __FILE__, "Invalid pop register request: " + reg_to_hex_string(system_reg_stack.data[i].register_numb));
	}
}

void Block_Machine::jump_fn(uint32_t new_program_count) {
	for (uint32_t i = 0; i < 64 / 4; ++i) { //Only push R[0-64], R[65-127] are function input registers
		push(i*4, i*4 + 1, i*4 + 2, i*4 + 3);
	}
	push(0x80, 0x81, 0x82, 0x83);
	push(0x84, 0x85, 0x86, 0x87);
	//push(0x88, 0x89, 0xFF, 0xFF);
	stack.function_jump();
	program_counter = new_program_count;
}

void Block_Machine::jump_return() {
	stack.function_return();
	pop(64 + 8);
	++program_counter;
}

void Block_Machine::conditional_jump_return(uint16_t condition_mask) {
	if (((condition_mask & flag_reg & 0x00FF) == (condition_mask & 0x00FF)) && ( ((((~flag_reg) & 0x00FF) << 8)) & 0xFF00 & condition_mask) ) {
		stack.function_return();
		pop(64 + 8);
	}
	++program_counter;
}

void Block_Machine::jump(int32_t offset) {
	program_counter += offset;
}

void Block_Machine::conditional_jump(uint16_t condition_mask, int16_t offset) {
	if (((condition_mask & flag_reg & 0x00FF) == (condition_mask & 0x00FF)) && ((((((~flag_reg) & 0x00FF) << 8)) & 0xFF00 & condition_mask))) {
		program_counter += offset;
		return;
	}
	++program_counter;
}

void Block_Machine::stop() {
	flag_reg |= 0xFFFF0000;
}

void Block_Machine::conditional_stop(uint16_t condition_mask) {
	if (((condition_mask & flag_reg & 0x00FF) == (condition_mask & 0x00FF)) && (((((~flag_reg) & 0x00FF) << 8)) & 0xFF00 & condition_mask)) {
		flag_reg |= 0xFFFF0000;
		return;
	}
}

void Block_Machine::struct_load(uint8_t load_reg) {
	Log::conditional_log_quit(!(load_reg < 128), __LINE__, __FILE__, "struct_load invalid reg index: 0x" + reg_to_hex_string(load_reg));
	R_state[load_reg] = struct_reg;
	set_reg_mem(load_reg);
}

void Block_Machine::hash_store(uint8_t store_reg) {
	Log::conditional_log_quit(!(store_reg < 128), __LINE__, __FILE__, "hash_store invalid reg index: 0x" + reg_to_hex_string(store_reg));
	Log::conditional_log_quit(db == nullptr, __LINE__, __FILE__, "hash_store no database connection!");
	Log::conditional_log_quit(R_state[store_reg]!= db->get_hash_structure_index(hash_reg), __LINE__, __FILE__, "hash_store structure type mismatch");
	db->set_map_data(hash_reg, hash_index_reg, R[store_reg]);
}

void Block_Machine::hash_load(uint8_t load_reg) {
	Log::conditional_log_quit(!(load_reg < 128), __LINE__, __FILE__, "hash_load invalid reg index: 0x" + reg_to_hex_string(load_reg));
	Log::conditional_log_quit(db==nullptr, __LINE__, __FILE__, "hash_load no database connection!");
	R_state[load_reg] = db->get_hash_structure_index(hash_reg);
	db->hash_unmap(hash_reg, hash_index_reg, R[load_reg]);
}

void Block_Machine::gvar_store(uint8_t load_reg) {
	//TODO in database
}

void Block_Machine::gvar_load(uint8_t load_reg) {
	//TODO in database
}

void Block_Machine::set_hash_reg(uint8_t reg) {
	Log::conditional_log_quit(!(reg < 128), __LINE__, __FILE__, "set_hash_reg invalid reg index: 0x" + reg_to_hex_string(reg));
	Log::conditional_log_quit(R_state[reg] != 0x13, __LINE__, __FILE__, "set_hash_reg invalid reg_state: 0x" + reg_to_hex_string(reg));
	hash_reg = *((uint32_t*)R[reg].data);
}

void Block_Machine::set_struct_reg(uint8_t reg) {
	Log::conditional_log_quit(!(reg < 128), __LINE__, __FILE__, "set_struct_reg invalid reg index: 0x" + reg_to_hex_string(reg));
	Log::conditional_log_quit(R_state[reg] != 0x13, __LINE__, __FILE__, "set_struct_reg invalid reg_state: 0x" + reg_to_hex_string(reg));
	struct_reg = *((uint32_t*)R[reg].data);
}

void Block_Machine::set_gvar_reg(uint8_t reg) {
	Log::conditional_log_quit(!(reg < 128), __LINE__, __FILE__, "set_gvar_reg invalid reg index: 0x" + reg_to_hex_string(reg));
	Log::conditional_log_quit(R_state[reg] != 0x13, __LINE__, __FILE__, "set_gvar_reg invalid reg_state: 0x" + reg_to_hex_string(reg));
	gvar_reg = *((uint32_t*)R[reg].data);
}

void Block_Machine::set_index_reg(uint8_t reg) {
	Log::conditional_log_quit(!(reg < 128), __LINE__, __FILE__, "set_index_reg invalid reg index: 0x" + reg_to_hex_string(reg));
	Log::conditional_log_quit(R_state[reg] != 0x13, __LINE__, __FILE__, "set_index_reg invalid reg_state: 0x" + reg_to_hex_string(reg));
	index_reg = *((uint32_t*)R[reg].data);
}

void Block_Machine::set_hash_reg(uint32_t val) {
	hash_reg = val;
}
void Block_Machine::set_struct_reg(uint32_t val) {
	struct_reg = val;
}
void Block_Machine::set_gvar_reg(uint32_t val) {
	gvar_reg = val;
}
void Block_Machine::set_index_reg(uint32_t val) {
	index_reg = val;
}

void Block_Machine::set_short_immediate_target(uint8_t load_reg) {
	//Uses struct_reg
	Log::conditional_log_quit(!(load_reg<128), __LINE__, __FILE__, "Inavlid short immediate target: 0x"+reg_to_hex_string(load_reg));
	immediate_target = load_reg;
	R_state[load_reg] = struct_reg;
	set_reg_mem(load_reg);
}

void Block_Machine::load_immediate_L(uint32_t Lw) {
	for (uint32_t i = 0; i < 4 && i<R[immediate_target].length; ++i) {
		R[immediate_target].data[i] = 0xFF & (Lw >> (8*i));
	}
}

void Block_Machine::load_immediate_U(uint32_t Uw) {
	for (uint32_t i = 0; i < 4 && (i+4) < R[immediate_target].length; ++i) {
		R[immediate_target].data[i+4] = 0xFF & (Uw >> (8 * i));
	}
}

void Block_Machine::set_long_immediate_target(uint8_t load_reg, uint16_t prim_count) {
	Log::conditional_log_quit(!(load_reg < 128), __LINE__, __FILE__, "Inavlid short immediate target: 0x" + reg_to_hex_string(load_reg));
	immediate_target = load_reg;
	R_state[load_reg] = struct_reg;
	if (struct_reg < 0x1000000000000000 && (struct_reg&0x20)) {
		uint32_t byte_count = get_default_primitive_byte_size(struct_reg ^ 0x20) * prim_count;
		R[load_reg].new_length(0, 0);
		R[load_reg].new_length(byte_count, 0);
	}
	else {
		set_reg_mem(load_reg);
	}
	immediate_byte_count = 0;
	immediate_load = immediate_byte_count != R[immediate_target].length;
}

void Block_Machine::process_immediate(uint64_t Dw) {
	Log::conditional_log_warn(!immediate_load, __LINE__, __FILE__, "Direct call to process_immediate was in error");
	Log::conditional_log_quit(immediate_byte_count >= R[immediate_target].length, __LINE__, __FILE__, "Process immediate NOP behavior: "+std::to_string(Dw));
	for (uint32_t i = 0; i < 8 && immediate_byte_count < R[immediate_target].length; ++i) {
		R[immediate_target].data[immediate_byte_count] = 0xFF & (Dw >> (8 * i));
		++immediate_byte_count;
	}
	immediate_load = immediate_byte_count != R[immediate_target].length;
}

void Block_Machine::append(uint8_t lower_array, uint8_t upper_array, uint8_t out_reg, bool CMP) {
	Log::conditional_log_quit(lower_array>=128 || upper_array>=128 || out_reg>=128, __LINE__, __FILE__, "append invalid register indexes");
	Log::conditional_log_quit(R_state[lower_array] >= 0x1000000000000000, __LINE__, __FILE__, "append non-primitive type");
	Log::conditional_log_quit((R_state[lower_array] & ((~0) ^ 0x20)) != (R_state[upper_array] & ((~0) ^ 0x20)), __LINE__, __FILE__, "append unequal register types");
	if (CMP) {
		Block_Machine::CMP(lower_array, upper_array);
	}
	R_state[out_reg] = R_state[lower_array] & 0x20;
	EasyArray<uint8_t> temp_arr(R[lower_array].length + R[upper_array].length);
	uint32_t i1 = 0;
	uint32_t i2 = 0;
	for (;i1< R[lower_array].length; ++i1) {
		temp_arr = R[lower_array].data[i1];
	}
	for (; i2 < R[upper_array].length; ++i2) {
		temp_arr = R[upper_array].data[i2];
	}
	temp_arr.swap(R[out_reg]);
	//free is called on temp_arr after this scope is exited
}

void Block_Machine::array_at_index(uint8_t array_reg, uint8_t out_reg, int16_t index_travel) {
	Log::conditional_log_quit(array_reg >= 128  || out_reg >= 128, __LINE__, __FILE__, "array_at_index invalid register indexes");
	Log::conditional_log_quit(R_state[array_reg] >= 0x1000000000000000 || !(R_state[array_reg] & 0x20), __LINE__, __FILE__, "array_at_index non-primitive type or non array");

	uint32_t type_leng = get_default_primitive_byte_size(R_state[array_reg] ^ 0x20);

	Log::conditional_log_quit(((uint64_t)index_reg + 1) * (uint64_t)type_leng > R[array_reg].length, __LINE__, __FILE__, "array_at_index invalid index request");
	EasyArray<uint8_t> temp_arr;
	temp_arr.clear_copy_from(type_leng, &(R[array_reg].data[index_reg * type_leng]));
	R[out_reg].swap(temp_arr);
	R_state[out_reg] = R_state[array_reg] ^ 0x20;

	index_reg += index_travel;
}

void Block_Machine::sub_array(uint8_t array_reg, uint8_t out_reg, uint16_t length) {
	Log::conditional_log_quit(array_reg >= 128 || out_reg >= 128, __LINE__, __FILE__, "sub_array invalid register indexes");
	Log::conditional_log_quit(R_state[array_reg] >= 0x1000000000000000 || !(R_state[array_reg] & 0x20), __LINE__, __FILE__, "sub_array non-primitive type or non array");

	uint32_t type_leng = get_default_primitive_byte_size(R_state[array_reg] ^ 0x20);

	Log::conditional_log_quit((index_reg + length) * (uint64_t)type_leng > R[array_reg].length, __LINE__, __FILE__, "sub_array invalid sub_length request");
	EasyArray<uint8_t> temp_arr;
	temp_arr.clear_copy_from(type_leng*length, &(R[array_reg].data[index_reg * type_leng]));
	R[out_reg].swap(temp_arr);
	R_state[out_reg] = R_state[array_reg];
}

void Block_Machine::sub_array_length_from_struct_reg(uint8_t array_reg, uint8_t out_reg) {
	Log::conditional_log_quit(array_reg >= 128 || out_reg >= 128, __LINE__, __FILE__, "sub_array invalid register indexes");
	Log::conditional_log_quit(R_state[array_reg] >= 0x1000000000000000 || !(R_state[array_reg] & 0x20), __LINE__, __FILE__, "sub_array non-primitive type or non array");

	uint32_t type_leng = get_default_primitive_byte_size(R_state[array_reg] ^ 0x20);

	Log::conditional_log_quit((index_reg + struct_reg) * (uint64_t)type_leng > R[array_reg].length, __LINE__, __FILE__, "sub_array invalid sub_length request");
	EasyArray<uint8_t> temp_arr;
	temp_arr.clear_copy_from(type_leng*struct_reg, &(R[array_reg].data[index_reg * type_leng]));
	R[out_reg].swap(temp_arr);
	R_state[out_reg] = R_state[array_reg];
}


std::string Block_Machine::cmd_to_hex_string(uint32_t Uw, uint32_t Lw) const{
	std::string L = "";
	std::string U = "";
	for (unsigned int i = 0; i < 8; ++i) {
		switch ((Lw>>(24 - 8*i)) & 0xF) {
		case 0x0: L += '0'; break;
		case 0x1: L += '1'; break;
		case 0x2: L += '2'; break;
		case 0x3: L += '3'; break;
		case 0x4: L += '4'; break;
		case 0x5: L += '5'; break;
		case 0x6: L += '6'; break;
		case 0x7: L += '7'; break;
		case 0x8: L += '8'; break;
		case 0x9: L += '9'; break;
		case 0xA: L += 'A'; break;
		case 0xB: L += 'B'; break;
		case 0xC: L += 'C'; break;
		case 0xD: L += 'D'; break;
		case 0xE: L += 'E'; break;
		case 0xF: L += 'F'; break;
		}
		switch ((Uw >> (24 - 8 * i)) & 0xF) {
		case 0x0: U += '0'; break;
		case 0x1: U += '1'; break;
		case 0x2: U += '2'; break;
		case 0x3: U += '3'; break;
		case 0x4: U += '4'; break;
		case 0x5: U += '5'; break;
		case 0x6: U += '6'; break;
		case 0x7: U += '7'; break;
		case 0x8: U += '8'; break;
		case 0x9: U += '9'; break;
		case 0xA: U += 'A'; break;
		case 0xB: U += 'B'; break;
		case 0xC: U += 'C'; break;
		case 0xD: U += 'D'; break;
		case 0xE: U += 'E'; break;
		case 0xF: U += 'F'; break;
		}
	}
	return U+L;
}

std::string Block_Machine::reg_to_hex_string(uint8_t regidx) const{
	std::string to_return = "";
	switch ((regidx >> (4)) & 0xF) {
	case 0x0: to_return += '0'; break;
	case 0x1: to_return += '1'; break;
	case 0x2: to_return += '2'; break;
	case 0x3: to_return += '3'; break;
	case 0x4: to_return += '4'; break;
	case 0x5: to_return += '5'; break;
	case 0x6: to_return += '6'; break;
	case 0x7: to_return += '7'; break;
	case 0x8: to_return += '8'; break;
	case 0x9: to_return += '9'; break;
	case 0xA: to_return += 'A'; break;
	case 0xB: to_return += 'B'; break;
	case 0xC: to_return += 'C'; break;
	case 0xD: to_return += 'D'; break;
	case 0xE: to_return += 'E'; break;
	case 0xF: to_return += 'F'; break;
	}
	switch (regidx & 0xF) {
	case 0x0: to_return += '0'; break;
	case 0x1: to_return += '1'; break;
	case 0x2: to_return += '2'; break;
	case 0x3: to_return += '3'; break;
	case 0x4: to_return += '4'; break;
	case 0x5: to_return += '5'; break;
	case 0x6: to_return += '6'; break;
	case 0x7: to_return += '7'; break;
	case 0x8: to_return += '8'; break;
	case 0x9: to_return += '9'; break;
	case 0xA: to_return += 'A'; break;
	case 0xB: to_return += 'B'; break;
	case 0xC: to_return += 'C'; break;
	case 0xD: to_return += 'D'; break;
	case 0xE: to_return += 'E'; break;
	case 0xF: to_return += 'F'; break;
	}
	return to_return;
}

uint32_t Block_Machine::get_register_structure_type(uint8_t reg) const{
	if (reg<128) {
		return R_state[reg];
	}
	switch (reg) {
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
	default:
		Log::log_error_quit(__LINE__, __FILE__, "Invalid register received: "+std::to_string((unsigned int)reg));
		return -1;
	}
}

void Block_Machine::add_immediate(uint8_t lhs, int16_t rhs, bool CMP) {
	Log::conditional_log_quit(lhs >= 128, __LINE__, __FILE__, "add_immediate invalid lhs reg");
	Log::conditional_log_quit(R_state[lhs] >= 0x1000000000000000, __LINE__, __FILE__, "add_immediate invalid lhs type");
	if (rhs == 0) {
		return;
	}

	uint64_t left = (uint32_t)0;
	int64_t right = rhs;

	switch (R_state[lhs]) {
	case 0x01: left = (uint64_t)*(int8_t*)(R[lhs].data)  ;*(int8_t*)(R[lhs].data)   += ( int8_t)right; break;
	case 0x11: left = (uint64_t)*(uint8_t*)(R[lhs].data) ;*(uint8_t*)(R[lhs].data)  += (uint8_t)right; break;
	case 0x02: left = (uint64_t)*(int16_t*)(R[lhs].data) ;*(int16_t*)(R[lhs].data)  += ( int16_t)right; break;
	case 0x12: left = (uint64_t)*(uint16_t*)(R[lhs].data);*(uint16_t*)(R[lhs].data) += (uint16_t)right; break;
	case 0x03: left = (uint64_t)*(int32_t*)(R[lhs].data) ;*(int32_t*)(R[lhs].data)  += ( int32_t)right; break;
	case 0x13: left = (uint64_t)*(uint32_t*)(R[lhs].data);*(uint32_t*)(R[lhs].data) += (uint32_t)right; break;
	case 0x04: left = (uint64_t)*(int64_t*)(R[lhs].data) ;*(int64_t*)(R[lhs].data)  += ( int64_t)right; break;
	case 0x14: left = (uint64_t)*(uint64_t*)(R[lhs].data);*(uint64_t*)(R[lhs].data) += (uint64_t)right; break;
	default:
		Log::log_error_quit(__LINE__, __FILE__, "add_immediate type missmatch on lhs");
		return;
	}

	right = left + right;

	if (CMP) {
		flag_reg &= 0xFF00;
		flag_reg |= (int64_t) left < right ? 0x01 : 0;
		flag_reg |= (int64_t) left <= right ? 0x02 : 0;
		flag_reg |= (int64_t) left == right ? 0x04 : 0;
		flag_reg |= (int64_t) left >= right ? 0x08 : 0;
		flag_reg |= (int64_t) left > right ? 0x10 : 0;
		flag_reg |= (int64_t) left == 0 ? 0x20 : 0;
		flag_reg |= right == 0 ? 0x40 : 0;
		flag_reg |= R[lhs].length == sizeof(int16_t) ? 0x80 : 0;
	}
	return;

}

uint32_t Block_Machine::get_program_counter() const{
	return program_counter;
}
void Block_Machine::keccak256(uint8_t reg){
	Log::conditional_log_quit(reg>=128, __LINE__, __FILE__, "keccak256 invalid target reg");
	//prehash = R_state[reg]
	hash_index_reg = custom_keccak256(R[reg].length, R[reg].data, R_state[reg]);
}