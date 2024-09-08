#pragma once

#include "resource_pack_slot.h"
#include "variable.h"

struct mString;
struct resource_pack_group;

struct mission_stack_manager {
    int pack_loads_or_unloads_pending;
    int field_4;
    bool loading_started;
    bool unloading_started;
    bool field_A;
    bool field_B;


    //0x0096851C
    static mission_stack_manager *& s_inst;
};

