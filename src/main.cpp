#include <iostream>
#include <chrono>
#include "Dual_Tree.h"
#include "Database_Parse.h"
#include "Logging.h"
#include "Block_Machine.h"
#include "Block_Machine_Assembler.h"
#include "Keccak.h"

#define PREHASH_MAP_LENG 100000

int main() {
	Log::set_log_file("Log.txt");
	Log::set_console_output(true);
	Log::set_debug_state(true);
	Log::set_info_state(true);

	Log::log_info(__LINE__, __FILE__, "Logging initialized---");

	database_state dbs = database_state({}, {0 | 0x20, 1, 2, 0x13}, {}, PREHASH_MAP_LENG);
	std::cout << dbs.getSize() << std::endl;


	Block_Machine m;
	m.connect(&dbs);

	std::string test = "Hello world!";

	EasyArray<uint64_t> instructions = {
		mach::jump_fn(2), //function call two instructions ahead
		mach::stop(),
		mach::set_struct_reg((uint32_t)0x20),
		mach::set_long_immediate_target(0, (uint16_t)test.length()),
		mach::process_immediate(*((const uint64_t*)(test.c_str()) + 0)), //The number of these process_immediate lines depend on the length of the string: floor((length + 7) / 8)
		mach::process_immediate(*((const uint64_t*)(test.c_str()) + 1)),
		mach::print(0, 0),
		mach::set_hash_reg((uint32_t)0),
		mach::set_struct_reg((uint32_t)0x15),
		mach::set_long_immediate_target(63, 1),
		mach::process_immediate(0x00000000000000FF), //uint256 needs 4 * 64 bits
		mach::process_immediate(0x0000000000000000),
		mach::process_immediate(0x0000000000000000),
		mach::process_immediate(0xF000000000000000),
		mach::swap(63, 0x86, false),
		mach::hash_store(0),
		mach::hash_load(1),
		mach::print(1, 0),
		mach::clear(0),
		mach::swap(1, 8, false),
		mach::print(8, 0),
		mach::set_struct_reg((uint32_t)0x13),
		mach::set_short_immediate_target(0),
		mach::load_immediate_L(0),
		mach::set_short_immediate_target(1),
		mach::load_immediate_L((uint32_t)test.length()),
		mach::set_index_reg((uint32_t)0),
		mach::NOP(), //LOOP: @(56 - line_base)
		mach::array_at_index(8, 7, 1),
		mach::set(0x83, 0, false),
		mach::CMP(0, 1),
		mach::print(0x07, 0),
		mach::conditional_jump(0x04 << 8, -5), //jump not equal //LOOP: 5 lines up
		mach::hash_store(8),
		mach::hash_load(8),
		mach::keccak256(8),
		mach::print(0x86, 0),
		mach::hash_store(8),
		mach::set_index_reg((uint32_t)0),
		mach::jump_return(),
	};

	m.run_till_stop(instructions);

	m.disconnect();
	std::cout << dbs.getSize() << std::endl;

	//std::chrono::time_point t0 = std::chrono::high_resolution_clock::now();
	//for(uint32_t i = 0; i<100000; ++i){
	//	m.keccak256(8);
	//}
	//std::chrono::time_point t1 = std::chrono::high_resolution_clock::now();
	//std::cout << std::chrono::duration_cast<std::chrono::microseconds>(t1-t0).count() << std::endl;

	dbs.state_to_file("Database.blkdb");
	dbs.file_to_state("Database.blkdb", PREHASH_MAP_LENG);
	dbs.state_to_file("ToCompare.blkdb");

	//double test_d = 0.0;
	//uint256_t temp256_0 = custom_keccak256(sizeof(double), (const uint8_t*) &test_d, (uint32_t)0);
	//uint256_t temp256_1 = custom_keccak256(sizeof(double), (const uint8_t*) &test_d, (uint32_t)1); 
	//uint256_t temp256_2 = custom_keccak256(0, nullptr, (uint32_t)0);
	//std::cout<<temp256_0<<std::endl;
	//std::cout<<temp256_1<<std::endl;
	//std::cout<<temp256_2<<std::endl;
}