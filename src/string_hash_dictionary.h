#pragma once


#include "func_wrapper.h"


struct  string_hash;


namespace string_hash_dictionary {

inline string_hash* register_string(string_hash* out, const char* str)
{
    // sp_log(str);

    string_hash* result = (string_hash*)CDECL_CALL(0x0053DE30, out, str);

    return result;
}
};
