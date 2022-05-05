#include "Database_Parse.h"
#include "Logging.h"
#include <string>

hash_map::hash_map(uint64_t element_size, uint32_t partial_hash_size) : type_size(element_size), partial_hash_map(partial_hash_size) {}

hash_map::hash_map() {
	type_size = 0;
};

const EasyArray<uint8_t>* hash_map::get_at(const uint256_t& a) const{
	const index_and_map& temp = partial_hash_map.data[(a%partial_hash_map.length)];
	const EasyArray<uint8_t>* temp_ptr = temp.get_at(a);
	return temp_ptr;
}

void hash_map::set_at(const uint256_t& a, const EasyArray<uint8_t>& d) {
	partial_hash_map.data[(a % partial_hash_map.length)].set_at(a, d);
}

uint64_t hash_map::getSize() const{
	uint64_t to_return = 0;
	for (uint32_t i = 0; i < partial_hash_map.length; ++i) {
		to_return += partial_hash_map.data[i].getSize() + sizeof(index_and_map);
	}
	return to_return;
}

const EasyArray<uint8_t>* index_and_map::get_at(const uint256_t& a) const{
	uint32_t index;
	if (full_hash.contains(a, index)) {
		return &(data_array.data[index]);
	}
	return nullptr;
}

void index_and_map::set_at(const uint256_t& a, const EasyArray<uint8_t>& d) {
	uint32_t index;
	if (full_hash.contains(a, index)) {
		data_array.data[index] = d;
		return;
	}
	full_hash.new_length(full_hash.length + 1, a);
	data_array.new_length(data_array.length + 1, d);
	return;
}

uint64_t index_and_map::getSize() const {
	uint64_t to_return = full_hash.length * sizeof(uint256_t);
	for (uint32_t i = 0; i < data_array.length; ++i) {
		to_return += data_array.data[i].length * sizeof(uint8_t) + sizeof(EasyArray<uint8_t>);
	}
	return to_return;
}


uint64_t structure_definition::getSize() const {
	return (byte_offset.length * sizeof(uint32_t)) + (element_type.length * sizeof(type_definition));
}

uint64_t database_state::getSize() const {
	uint64_t to_return = hash_map_type_indexes.length * sizeof(uint32_t);
	for (uint32_t i = 0; i < structures.length; ++i) {
		to_return += structures.data[i].getSize() + sizeof(structure_definition);
	}
	for (uint32_t i = 0; i < hash_maps.length; ++i) {
		to_return += hash_maps.data[i].getSize() + sizeof(hash_map);
	}
	for (uint32_t i = 0; i < global_variables.length; ++i) {
		to_return += global_variables.data[i].getSize() + sizeof(index_and_map);
	}
	return to_return;
}


uint32_t get_default_primitive_byte_size(uint32_t prim_index) {
	if (prim_index & 0x20) {
		return 0;
	}
	switch (prim_index & 0xDF) {
	case 0x00: return sizeof(char);
	case 0x10: return sizeof(unsigned char);
	case 0x01: return sizeof(int8_t);
	case 0x11: return sizeof(uint8_t);
	case 0x02: return sizeof(int16_t);
	case 0x12: return sizeof(uint16_t);
	case 0x03: return sizeof(int32_t);
	case 0x13: return sizeof(uint32_t);
	case 0x04: return sizeof(int64_t);
	case 0x14: return sizeof(uint64_t);
	case 0x05: return sizeof(uint256_t); //TODO int256_t ???
	case 0x15: return sizeof(uint256_t);
	case 0x06: return sizeof(float);
	case 0x07: return sizeof(double);
	default: return 0;
	}
}

bool validate_primitive_length(uint32_t prim_index, uint32_t length) {
	if ((prim_index & 0x20)) {
		uint32_t base_leng = get_default_primitive_byte_size(prim_index ^ 0x20);
		Log::conditional_log_quit(base_leng == 0, __LINE__, __FILE__, "Inavlidate array primitive: "+std::to_string(prim_index));
		return !(length % base_leng);
	}
	return get_default_primitive_byte_size(prim_index) == length;
}

const structure_definition* database_state::get_structure_def(uint32_t structure_index) const {
	Log::conditional_log_quit(structure_index < 0x1000000000000000, __LINE__, __FILE__, "Invalid structure index: "+ std::to_string(structure_index));
	Log::conditional_log_quit(structure_index >= structures.length, __LINE__, __FILE__, "Structure index: "+ std::to_string(structure_index) + " Structure length: "+std::to_string(structures.length));
	return &(structures.data[structure_index - 0x1000000000000000]);
}

uint32_t database_state::get_hash_structure_index(uint32_t hash_index) const {
	Log::conditional_log_quit(hash_index >= hash_maps.length, __LINE__, __FILE__, "Hash map index: " + std::to_string(hash_index) + " Hash map length: " + std::to_string(hash_maps.length));
	Log::conditional_log_quit(hash_index >= hash_map_type_indexes.length, __LINE__, __FILE__, "Hash map type index: " + std::to_string(hash_index) + " Hash map type length: " + std::to_string(hash_map_type_indexes.length));
	return hash_map_type_indexes.data[hash_index];
}

void database_state::hash_unmap(uint32_t hash_index, const uint256_t& h, EasyArray<uint8_t>& out) const {
	uint32_t struct_index = get_hash_structure_index(hash_index);
	const structure_definition* struct_def = nullptr;
	if (struct_index >= 0x1000000000000000) {
		struct_def = get_structure_def(struct_index);
		Log::conditional_log_quit(struct_def == nullptr, __LINE__, __FILE__, "Nullptr struct def? How?");
	}
	//If here, all Log checks have passed

	const EasyArray<uint8_t>* temp_ptr = hash_maps.data[hash_index].get_at(h);

	if (temp_ptr == nullptr) {
		Log::log_info(__LINE__, __FILE__, "Default constructor called for struct_index: "+std::to_string(struct_index));
		if (struct_def == nullptr) {
			out = EasyArray<uint8_t> (get_default_primitive_byte_size(struct_index));
			out.set_all(0);
			return;
		}
		out = EasyArray<uint8_t>(struct_def->struct_size);
		return;
	}
	Log::log_info(__LINE__, __FILE__, "Found stored value from hash: " + std::to_string(h));
	out = *temp_ptr;
	return;
}

void database_state::set_map_data(uint32_t hash_index, const uint256_t& h, const EasyArray<uint8_t>& data) const {
	uint32_t struct_index = get_hash_structure_index(hash_index);
	const structure_definition* struct_def;
	if (struct_index >= 0x1000000000000000) {
		struct_def = get_structure_def(struct_index);
		Log::conditional_log_quit(struct_def == nullptr, __LINE__, __FILE__, "Nullptr struct def? How?");
		Log::conditional_log_quit(struct_def->struct_size != data.length, __LINE__, __FILE__, "Invalid struct data length passed to set_map_data. Expected: "+std::to_string(struct_def->struct_size) + " Received: " + std::to_string(data.length));
		hash_maps.data[hash_index].set_at(h, data);
		return;
	}

	Log::conditional_log_quit(!validate_primitive_length(struct_index, data.length), __LINE__, __FILE__, "Invalid primitive length passed to set_map_data. Received: " + std::to_string(data.length));
	hash_maps.data[hash_index].set_at(h, data);
}

database_state::database_state() {};
database_state::database_state(const EasyArray<structure_definition>& structs, const EasyArray<uint32_t>& hash_map_indicies, const EasyArray<index_and_map>& global_vars, uint32_t hash_base_leng) {
	structures = structs;
	hash_maps = EasyArray<hash_map>(hash_map_indicies.length);
	hash_map_type_indexes = hash_map_indicies;
	global_variables = global_vars;


	for (uint32_t i = 0; i < hash_maps.length; ++i) {
		uint32_t index = get_hash_structure_index(i);
		if (index >= 0x1000000000000000) {
			const structure_definition* def = get_structure_def(index);
			hash_maps.data[i] = hash_map(def->struct_size, hash_base_leng);
			continue;
		}
		hash_maps.data[i] = hash_map(get_default_primitive_byte_size(index), hash_base_leng);
	}

}

database_state::~database_state() {

}