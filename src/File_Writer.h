#pragma once
#include <string>
#include <cstdint>

namespace writer{
    bool initialize_writer(std::string path);
    void wd(double d);
    void wf(float f);

    void wb(bool b);
    void wi8(int8_t c);
    void wui8(uint8_t uc);
    void wi16(int16_t s);
    void wui16(uint16_t us);
    void wi32(int32_t i);
    void wui32(uint32_t ui);
    void wi64(int64_t ll);
    void wui64(uint64_t ull);

    void wstr(std::string s);

    void free_writer();
}
