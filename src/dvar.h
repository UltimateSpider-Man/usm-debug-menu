#pragma once

#include "mstring.h"

#include <map>

#include <cassert>

    
struct debug_variable_t {
    mString field_0;
    float value;

debug_variable_t(const char* a1, float a2)
        : field_0(a1)
    {
       mString val (0);
        this->add_value(val);
    }

void add_value(mString& a1)
    {
}








};

std::map<mString, mString> g_dvars {};