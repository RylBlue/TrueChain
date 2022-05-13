#pragma once
#include<fstream>
#include <cstdint>

namespace read{
    std::ifstream* initialize_reader(std::string path, bool textfile);

    double rd(std::ifstream* f);
    float rf(std::ifstream* f);
    bool rb(std::ifstream* f);

    int8_t ri8(std::ifstream* f);
    uint8_t rui8(std::ifstream* f);
    int16_t ri16(std::ifstream* f);
    uint16_t rui16(std::ifstream* f);
    int32_t ri32(std::ifstream* f);
    uint32_t rui32(std::ifstream* f);
    int64_t ri64(std::ifstream* f);
    uint64_t rui64(std::ifstream* f);

    std::string rstr(std::ifstream* f);
    std::string rFile(std::ifstream* f);

    void free_reader(std::ifstream* f);

    static std::string s = "";
};