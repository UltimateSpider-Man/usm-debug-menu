#pragma once

#include <list>

#include <cstdint>

struct script_executable;
struct mString;

struct script_executable_allocated_stuff_record {
    void (*field_0)(script_executable *, std::list<uint32_t> &, std::list<mString> &);
    std::list<uint32_t> stuff;
    std::list<mString> debug_stuff_descriptions;

    script_executable_allocated_stuff_record() = default;
};
