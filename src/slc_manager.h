#pragma once

#include "mstring.h"
#include "variable.h"

#include <vector>

#include <set>

struct script_library_class;

struct slc_manager {

 static void init()
{
    TRACE("slc_manager::init");


    {
        CDECL_CALL(0x005AD720);
    }
}


};

extern std::vector<script_library_class *> *& slc_manager_class_array;

