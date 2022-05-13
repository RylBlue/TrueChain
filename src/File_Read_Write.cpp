#include <fstream>
#include <utility>
#include "Logging.h"

//FILE READING FUNCTIONS
#include "File_Reader.h"
const int ETEST = 1; //stored on compile time
bool etest() {
	return ((*(char*)&ETEST) == 0);
}
const bool BIGENDIAN = etest(); //stored on run time

std::ifstream* read::initialize_reader(std::string path, bool textfile) {
	if (textfile) {
		return new std::ifstream(path);
	}
	return new std::ifstream(path, std::ios::binary);
}
//chars and bools being only 1 in length do not depend on byte endian nature
int8_t read::ri8(std::ifstream* f) {
	char Cs1[1];
	Log::conditional_log_quit(!(f->good()), __LINE__, __FILE__, "File read error: ri8");
	f->read(Cs1, 1);
	return Cs1[0];
}
uint8_t read::rui8(std::ifstream* f) {
	char Cs1[1];
	Log::conditional_log_quit(!(f->good()), __LINE__, __FILE__, "File read error: rui8");
	f->read(Cs1, 1);
	return (unsigned char)Cs1[0];
}
bool read::rb(std::ifstream* f) {
	char Cs1[1];
	Log::conditional_log_quit(!(f->good()), __LINE__, __FILE__, "File read error: rb");
	f->read(Cs1, 1);
	return *reinterpret_cast<bool*>(Cs1);
}

//These next data types have to be converted based off of the endian nature of the os running the code
//my os is little endian, keeping that in mind, if the system reading my files is big endian, I have to flip the byte order
double read::rd(std::ifstream* f) {
	char Cs8[8];
	Log::conditional_log_quit(!(f->good()), __LINE__, __FILE__, "File read error: rd");
	f->read(Cs8, 8);
	if (BIGENDIAN) {
		std::swap(Cs8[0], Cs8[7]); // @suppress("Invalid arguments")
		std::swap(Cs8[1], Cs8[6]); // @suppress("Invalid arguments")
		std::swap(Cs8[2], Cs8[5]); // @suppress("Invalid arguments")
		std::swap(Cs8[3], Cs8[4]); // @suppress("Invalid arguments")
	}
	return *reinterpret_cast<double*>(Cs8);
}

float read::rf(std::ifstream* f) {
	char Cs4[4];
	Log::conditional_log_quit(!(f->good()), __LINE__, __FILE__, "File read error: rf");
	f->read(Cs4, 4);
	if (BIGENDIAN) {
		std::swap(Cs4[0], Cs4[3]); // @suppress("Invalid arguments")
		std::swap(Cs4[1], Cs4[2]); // @suppress("Invalid arguments")
	}
	return *reinterpret_cast<float*>(Cs4);
}
int16_t read::ri16(std::ifstream* f) {
	char Cs2[2];
	Log::conditional_log_quit(!(f->good()), __LINE__, __FILE__, "File read error: ri16");
	f->read(Cs2, 2);
	if (BIGENDIAN) {
		std::swap(Cs2[0], Cs2[1]); // @suppress("Invalid arguments")
	}
	return *reinterpret_cast<int16_t*>(Cs2);
}
uint16_t read::rui16(std::ifstream* f) {
	char Cs2[2];
	Log::conditional_log_quit(!(f->good()), __LINE__, __FILE__, "File read error: rui16");
	f->read(Cs2, 2);
	if (BIGENDIAN) {
		std::swap(Cs2[0], Cs2[1]); // @suppress("Invalid arguments")
	}
	return *reinterpret_cast<uint16_t*>(Cs2);
}

int32_t read::ri32(std::ifstream* f) {
	char Cs4[4];
	Log::conditional_log_quit(!(f->good()), __LINE__, __FILE__, "File read error: ri32");
	f->read(Cs4, 4);
	if (BIGENDIAN) {
		std::swap(Cs4[0], Cs4[3]); // @suppress("Invalid arguments")
		std::swap(Cs4[1], Cs4[2]); // @suppress("Invalid arguments")
	}
	return *reinterpret_cast<int32_t*>(Cs4);
}
uint32_t read::rui32(std::ifstream* f) {
	char Cs4[4];
	Log::conditional_log_quit(!(f->good()), __LINE__, __FILE__, "File read error: rui32");
	f->read(Cs4, 4);
	if (BIGENDIAN) {
		std::swap(Cs4[0], Cs4[3]); // @suppress("Invalid arguments")
		std::swap(Cs4[1], Cs4[2]); // @suppress("Invalid arguments")
	}
	return *reinterpret_cast<uint32_t*>(Cs4);
}

int64_t read::ri64(std::ifstream* f) {
	char Cs8[8];
	Log::conditional_log_quit(!(f->good()), __LINE__, __FILE__, "File read error: ri64");
	f->read(Cs8, 8);
	if (BIGENDIAN) {
		std::swap(Cs8[0], Cs8[7]); // @suppress("Invalid arguments")
		std::swap(Cs8[1], Cs8[6]); // @suppress("Invalid arguments")
        std::swap(Cs8[2], Cs8[5]); // @suppress("Invalid arguments")
		std::swap(Cs8[3], Cs8[4]); // @suppress("Invalid arguments")
	}
	return *reinterpret_cast<int64_t*>(Cs8);
}
uint64_t read::rui64(std::ifstream* f) {
	char Cs8[8];
	Log::conditional_log_quit(!(f->good()), __LINE__, __FILE__, "File read error: rui64");
	f->read(Cs8, 8);
	if (BIGENDIAN) {
		std::swap(Cs8[0], Cs8[7]); // @suppress("Invalid arguments")
		std::swap(Cs8[1], Cs8[6]); // @suppress("Invalid arguments")
        std::swap(Cs8[2], Cs8[5]); // @suppress("Invalid arguments")
		std::swap(Cs8[3], Cs8[4]); // @suppress("Invalid arguments")
	}
	return *reinterpret_cast<uint64_t*>(Cs8);
}


std::string read::rstr(std::ifstream* f) {
	//No way to know when the string ends, unless a break character is used
	//Define the break character to be '\0' in this implementation
	s = "";
	int8_t c = ri8(f);
	while (c != '\0') { //Break char test
		s += (char)c;
		c = ri8(f);
	}
	return s;
}
std::string read::rFile(std::ifstream* f) {
	s = "";
	static char* temp = new char(' ');
	while (f->good()) {
		f->read(temp, 1);
		s += *temp;
	}
	return s;
}
void read::free_reader(std::ifstream* f) {
	f->close();
	delete(f);
}





//FILE WRITING FUNCTIONS AND DECLERATIONS
#include "File_Writer.h"

static std::ofstream WRITE_FILE;

const static char str_escape = '\0';
bool writer::initialize_writer(std::string path) {
    if(WRITE_FILE.is_open()){
        return false;
    }
	WRITE_FILE = std::ofstream(path, std::ios::binary);
    return WRITE_FILE.good();
}


void writer::wd(double d) {
	if (BIGENDIAN) {
		char* c_array = (char*)&d;
		std::swap(c_array[0], c_array[7]); // @suppress("Invalid arguments")
		std::swap(c_array[1], c_array[6]); // @suppress("Invalid arguments")
		std::swap(c_array[2], c_array[5]); // @suppress("Invalid arguments")
		std::swap(c_array[3], c_array[4]); // @suppress("Invalid arguments")
	}
	WRITE_FILE.write((char*)&d, 8);
}
void writer::wf(float ff) {
	if (BIGENDIAN) {
		char* c_array = (char*)&ff;
		std::swap(c_array[0], c_array[3]); // @suppress("Invalid arguments")
		std::swap(c_array[1], c_array[2]); // @suppress("Invalid arguments")
	}
	WRITE_FILE.write((char*)&ff, 4);
}


void writer::wb(bool b) {
	WRITE_FILE.write((char*)&b, 1);
}

void writer::wi8(int8_t c) {
	WRITE_FILE.write((char*)&c, 1);
}
void writer::wui8(uint8_t uc) {
	WRITE_FILE.write((char*)&uc, 1);
}


void writer::wi16(int16_t s) {
	if (BIGENDIAN) {
		char* c_array = (char*)&s;
		std::swap(c_array[0], c_array[1]); // @suppress("Invalid arguments")
	}
	WRITE_FILE.write((char*)&s, 2);
}
void writer::wui16(uint16_t us) {
	if (BIGENDIAN) {
		char* c_array = (char*)&us;
		std::swap(c_array[0], c_array[1]); // @suppress("Invalid arguments")
	}
	WRITE_FILE.write((char*)&us, 2);
}


void writer::wi32(int32_t i) {
	if (BIGENDIAN) {
		char* c_array = (char*)&i;
		std::swap(c_array[0], c_array[3]); // @suppress("Invalid arguments")
		std::swap(c_array[1], c_array[2]); // @suppress("Invalid arguments")
	}
	WRITE_FILE.write((char*)&i, 4);
}
void writer::wui32(uint32_t ui) {
	if (BIGENDIAN) {
		char* c_array = (char*)&ui;
		std::swap(c_array[0], c_array[3]); // @suppress("Invalid arguments")
		std::swap(c_array[1], c_array[2]); // @suppress("Invalid arguments")
	}
	WRITE_FILE.write((char*)&ui, 4);
}

void writer::wi64(int64_t i) {
	if (BIGENDIAN) {
		char* c_array = (char*)&i;
		std::swap(c_array[0], c_array[7]); // @suppress("Invalid arguments")
		std::swap(c_array[1], c_array[6]); // @suppress("Invalid arguments")
        std::swap(c_array[2], c_array[5]); // @suppress("Invalid arguments")
		std::swap(c_array[3], c_array[4]); // @suppress("Invalid arguments")
	}
	WRITE_FILE.write((char*)&i, 8);
}
void writer::wui64(uint64_t ui) {
	if (BIGENDIAN) {
		char* c_array = (char*)&ui;
		std::swap(c_array[0], c_array[7]); // @suppress("Invalid arguments")
		std::swap(c_array[1], c_array[6]); // @suppress("Invalid arguments")
        std::swap(c_array[2], c_array[5]); // @suppress("Invalid arguments")
		std::swap(c_array[3], c_array[4]); // @suppress("Invalid arguments")
	}
	WRITE_FILE.write((char*)&ui, 8);
}

void writer::wstr(std::string s) {
    //This function assumes that there is no instance of 'str_escape' in s
	WRITE_FILE.write(s.c_str(), s.length());
	WRITE_FILE.write(&str_escape, 1);
}

void writer::free_writer() {
	WRITE_FILE.close();
}



