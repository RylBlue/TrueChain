#pragma once
#include "Essential_Types.h"
#include "Easy_Array.h"
#include "Interfaces.h"




struct type_definition {
public:
	uint32_t type_index; //Primitive range is: [0x0000000000000000, 0x1000000000000000),     Structure range is: [0x1000000000000000, 0xFFFFFFFFFFFFFFFF]
	uint32_t length;
};

struct structure_definition : public sizeable {
public:
	unsigned short element_count{}; //Max number of structure elements = 2^16 - 1
	uint32_t struct_size{}; //Byte size of the structure
	EasyArray<uint32_t> byte_offset; //byte_offset for each element
	EasyArray<type_definition> element_type;
	uint64_t getSize() const;
};


//Primitive type declaration:
//0x00 = char (ascii print)
//0x01 = byte (int8)
//0x02 = int16
//0x03 = int32
//0x04 = int64
//0x05 = int256
//0x06 = float
//0x07 = double

//flags:
//0x10 = unsigned modifier (unsigned float and unsigned double are meaningless)
//0x20 = array modifier


struct index_and_map : public sizeable {
public:
	EasyArray<uint256_t> full_hash;
	EasyArray<EasyArray<uint8_t>> data_array;
	void set_at(const uint256_t& h, const EasyArray<uint8_t>& a);
	const EasyArray<uint8_t>* get_at(const uint256_t& h) const;
	uint64_t getSize() const;
};

struct hash_and_value{
public:
	uint256_t hash;
	uint32_t length;
	const uint8_t* value;
};

struct hash_map : public sizeable {
public:
	hash_map();
	hash_map(uint32_t element_size, uint32_t partial_hash_size);
	void set_at(const uint256_t& h, const EasyArray<uint8_t>& a);
	const EasyArray<uint8_t>* get_at(const uint256_t& h) const;
	uint64_t getSize() const;
//private:
	uint32_t type_size; //in bytes
	EasyArray<index_and_map> partial_hash_map;
};



struct database_state : public sizeable {
private:
	EasyArray<structure_definition> structures; //Indexed as (type_index - 0x1000000000000000)
	EasyArray<hash_map> hash_maps;
	EasyArray<uint32_t> hash_map_type_indexes; //Match the type index seen here to the structures array above (if value is >= 0x1000000000000000) 
	EasyArray<index_and_map> global_variables;
public:
	database_state();
	database_state(const EasyArray<structure_definition>& structs, const EasyArray<uint32_t>& hash_map_indicies, const EasyArray<index_and_map>& global_vars, uint32_t hash_base_leng);
	uint64_t getSize() const;
	~database_state();

	const structure_definition* get_structure_def(uint32_t structure_index) const;
	uint32_t get_hash_structure_index(uint32_t hash_index) const;
	void hash_unmap(uint32_t hash_index, const uint256_t& h, EasyArray<uint8_t>& out) const;
	void set_map_data(uint32_t hash_index, const uint256_t& h, const EasyArray<uint8_t>& data) const;
	
	void state_to_file(std::string pathname) const;
	void file_to_state(std::string pathname, uint32_t hash_base_leng);
};

uint32_t get_default_primitive_byte_size(uint32_t prim_index);
bool validate_primitive_length(uint32_t prim_index, uint32_t length);