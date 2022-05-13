#include "Database_Parse.h"
#include "Logging.h"
#include <string>

hash_map::hash_map(uint32_t element_size, uint32_t partial_hash_size) : type_size(element_size), partial_hash_map(partial_hash_size) {}

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


#include "File_Reader.h"
#include "File_Writer.h"
#include "Easy_Array64.h"
void database_state::state_to_file(std::string pathname) const{
	Log::conditional_log_quit(!writer::initialize_writer(pathname), __LINE__, __FILE__, "Failed to open output file: ./"+pathname);
	//Basic length header
	writer::wui32(structures.length);
	writer::wui32(hash_maps.length);
	writer::wui32(global_variables.length);
	//Structure definitions
	for(uint32_t i = 0; i<structures.length; ++i){
		writer::wui16(structures.data[i].element_count);
		writer::wui32(structures.data[i].struct_size);
		for(uint32_t ii = 0; ii< structures.data[i].element_count; ++i){
			writer::wui32(structures.data[i].byte_offset.data[ii]);
			writer::wui32(structures.data[i].element_type.data[ii].type_index);
			writer::wui32(structures.data[i].element_type.data[ii].length);
		}
	}
	//hash_map definitions and state
	for(uint32_t i = 0; i<hash_maps.length; ++i){
		uint32_t base_length = hash_maps.data[i].type_size;
		writer::wui32(hash_map_type_indexes.data[i]);
		writer::wui32(base_length);
		uint64_t hash_count = 0;
		for(uint32_t ii = 0; ii<hash_maps.data[i].partial_hash_map.length; ++ii){
			hash_count += hash_maps.data[i].partial_hash_map.data[ii].data_array.length;
		}
		writer::wui64(hash_count);
		EasyArray64<hash_and_value> full_hash_table[2];
		full_hash_table[0].new_length(hash_count, {});
		full_hash_table[1].new_length(hash_count, {});

		uint64_t hash_index = 0;
		for(uint32_t ii = 0; ii<hash_maps.data[i].partial_hash_map.length; ++ii){
			for(uint32_t iii = 0; iii<hash_maps.data[i].partial_hash_map.data[ii].data_array.length; ++iii){
				full_hash_table[0].data[hash_index] = {
					hash_maps.data[i].partial_hash_map.data[ii].full_hash.data[iii], //hash value
					hash_maps.data[i].partial_hash_map.data[ii].data_array.data[iii].length, //byte length
					hash_maps.data[i].partial_hash_map.data[ii].data_array.data[iii].data //byte array
				};
				++hash_index;
			}
		}

		//radix sort time :)
		//I could easily use either 8bit or 16 bit buffers, for simplicity I'll just use 8 bits
		
		uint8_t main_table = 0;
		EasyArray<uint64_t> count(256), prefix_sum(256);
		prefix_sum.set_all(0);
		for(uint8_t working_byte = 0; working_byte<32; ++working_byte){
			count.set_all(0);
			for(uint64_t fp = 0; fp<hash_count; ++fp){
				++(count.data[full_hash_table[main_table].data[fp].hash.I[working_byte]]);
			}
			uint64_t working_sum = 0;
			for(uint32_t working_index = 0; working_index<256; ++working_index){
				prefix_sum.data[working_index] = working_sum;
				working_sum += count.data[working_index];
			}
			for(uint64_t sp = 0; sp<hash_count; ++sp){
				hash_and_value a = full_hash_table[main_table].data[sp];
				uint64_t new_index = prefix_sum.data[a.hash.I[working_byte]]; //get the index then increment prefix sum
				++(prefix_sum.data[a.hash.I[working_byte]]);
				full_hash_table[(main_table+1)%2].data[new_index] = a;
			}
			main_table = (main_table+1)%2;
		}
		

		//This whole sort is of order 32*2*N + 32*256*C
		//16bits would be of order 16*2*N + 16*65536*C
		
		for(uint64_t index = 0; index<hash_count; ++index){
			uint32_t arr_leng = full_hash_table[main_table].data[index].length;
			if(base_length == 0){
				writer::wui32(arr_leng);
			}
			for(uint8_t hash_byte = 0; hash_byte<32; ++hash_byte){
				writer::wui8(full_hash_table[main_table].data[index].hash.I[hash_byte]);
			}
			for(uint32_t index2 = 0; index2<(base_length == 0 ? arr_leng : base_length); ++index2){
				writer::wui8(full_hash_table[main_table].data[index].value[index2]);
			}	
		}
		
	}
	//gvar definitions and state
	for(uint32_t i = 0; i<global_variables.length; ++i){
		//TODO
	}
	writer::free_writer();
}
void database_state::file_to_state(std::string pathname, uint32_t hash_base_leng){
	std::ifstream* f = read::initialize_reader(pathname, false);
	Log::conditional_log_quit(f==nullptr || !f->good(), __LINE__, __FILE__, "Failed to open input file: ./"+pathname);
	
	structures.new_length(0,{});
	hash_maps.new_length(0, {});
	hash_map_type_indexes.new_length(0, {});
	global_variables.new_length(0, {});

	uint32_t struct_count, hash_count, gvar_count;
	struct_count = read::rui32(f);
	hash_count = read::rui32(f);
	gvar_count = read::rui32(f);

	structures.new_length(struct_count, {});
	hash_maps.new_length(hash_count, {});
	hash_map_type_indexes.new_length(hash_count, {});
	global_variables.new_length(0, {});

	for(uint32_t i = 0; i<struct_count; ++i){
		uint16_t element_count = read::rui16(f);
		uint32_t struct_size = read::rui32(f);
		structures.data[i].element_count = element_count;
		structures.data[i].struct_size = struct_size;
		structures.data[i].element_type.new_length(element_count, {});
		structures.data[i].byte_offset.new_length(element_count, {});
		for(uint16_t ii = 0; ii<element_count; ++ii){
			structures.data[i].byte_offset.data[ii] = read::rui32(f);
			uint32_t type_index = read::rui32(f);
			uint32_t length = read::rui32(f);
			structures.data[i].element_type.data[ii] = {type_index, length};
		}
	}
	
	for(uint32_t i = 0; i<hash_count; ++i){
		uint32_t struct_type = read::rui32(f);
		uint32_t base_length = read::rui32(f);
		uint64_t net_hash_count = read::rui64(f);

		hash_maps.data[i].type_size = base_length;
		hash_maps.data[i].partial_hash_map.new_length(hash_base_leng, {});
		hash_map_type_indexes.data[i] = struct_type;

		EasyArray64<uint256_t> hash_index_storage(net_hash_count);
		EasyArray64<EasyArray<uint8_t>> hash_value_storage;
		hash_value_storage.new_length(net_hash_count, {});
		for(uint64_t fp = 0; fp<net_hash_count; ++fp){
			uint32_t real_length = base_length;
			if(real_length == 0){
				real_length = read::rui32(f);
			}
			uint256_t hash_index;
			for(uint8_t hb = 0; hb<32; ++hb){
				hash_index.I[hb] = read::rui8(f);
			}
			EasyArray<uint8_t> temp_arr(real_length);
			for(uint32_t rb = 0; rb<real_length; ++rb){
				temp_arr.data[rb] = read::rui8(f);
			}
			hash_index_storage.data[fp] = hash_index;
			hash_value_storage.data[fp].swap(temp_arr);
		}
		EasyArray<uint32_t> hash_loc_counts(hash_base_leng);
		EasyArray<uint32_t> hash_loc_index(hash_base_leng);
		hash_loc_counts.set_all(0);
		hash_loc_index.set_all(0);
		for(uint64_t pi = 0; pi<net_hash_count; ++pi){
			++hash_loc_counts.data[(hash_index_storage.data[pi] % hash_base_leng)];
		} 
		for(uint32_t si = 0; si<hash_base_leng; ++si){
			hash_maps.data[i].partial_hash_map.data[si].data_array.new_length(hash_loc_counts.data[si], {});
			hash_maps.data[i].partial_hash_map.data[si].full_hash.new_length(hash_loc_counts.data[si], {});
		}
		for(uint64_t ti = 0; ti<net_hash_count; ++ti){
			uint256_t temp_hash = hash_index_storage.data[ti];
			uint32_t temp_index = hash_loc_index.data[temp_hash % hash_base_leng]++;
			hash_maps.data[i].partial_hash_map.data[temp_hash % hash_base_leng].data_array.data[temp_index].swap(hash_value_storage.data[ti]);
			hash_maps.data[i].partial_hash_map.data[temp_hash % hash_base_leng].full_hash.data[temp_index] = temp_hash;
		}
	}
	for(uint32_t i = 0; i<gvar_count; ++i){
		//TODO
	}


	read::free_reader(f);
}

//database state file format is as follows:
//uint32_t structure_count
//uint32_t hash_map_count
//uint32_t gvar_count
//FOREACH structure
	//uint16_t element_count
	//uint32_t struct_size
	//FOREACH element
		//uint32_t byte_offset
		//uint32_t type_index
		//uint32_t length
//FOREACH hash_map
	//uint32_t structure_type
	//uint32_t type_byte_count
	//uint64_t hash_count
	//FOREACH hash
		//IF (type_byte_count == 0)
			//uint32_t array_byte_count
		//uint256_t hash_index
		//FOREACH type_byte OR array_byte
			//uint8_t v
//FOREACH gvar
	//uint32_t var_count
	//TODO


