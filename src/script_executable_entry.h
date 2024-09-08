#pragma once

#include "resource_key.h"

struct script_executable;

struct script_executable_entry {
    script_executable *exec;
    int field_4;
    const char *field_8;
    resource_key field_C;
};

struct script_executable_entry_key {
    resource_key field_0;
    resource_key field_8;

};
