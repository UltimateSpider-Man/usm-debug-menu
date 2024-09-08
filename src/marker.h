#pragma once

#include "entity_base.h"

struct marker : entity_base {
    int empty[1];
    int field_48;
    int field_4C;
    int field_50;
    int field_54;
    int field_58;
    int field_5C;
    int field_60;
    int field_64;

    marker(const string_hash &a2, uint32_t a3);
};
