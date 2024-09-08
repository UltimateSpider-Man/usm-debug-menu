#pragma once

#include "resource_key.h"

struct generic_mash_header;
struct generic_mash_data_ptrs;
struct Dumper;

struct resource_location {
    resource_key field_0;
    uint32_t m_offset;
    int m_size;

    //0x0051FE00
    resource_location();

    void operator=(const resource_location &loc);

    void clear();

    //0x0050E980
    void un_mash(generic_mash_header *, void *, generic_mash_data_ptrs *);
};
