#pragma once

#include "resource_key.h"
#include "resource_versions.h"

struct resource_pack_header {
    resource_versions field_0;
    int field_14;
    int directory_offset;
    int res_dir_mash_size;
    int field_20;
    int field_24;
    int field_28;

    //0x0051FCA0
    resource_pack_header();

    //0x00537DA0
    bool verify(resource_key a2) const;

    void clear();
};

extern void resource_pack_header_patch();
