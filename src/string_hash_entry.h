#pragma once


#include "mstring.h"
#include "string_hash.h"

struct mash_info_struct;

struct string_hash_entry {
    string_hash field_0;
    mString field_4;


    mString generate_text(const char* a3) const
    {
        if constexpr (1) {
            mString a1 { 0, "0x%08x\t%s%s", this->field_0.source_hash_code, this->field_4.c_str(), a3 };

            return a1;
        } else {
            mString result;
           CDECL_CALL(0x0050DBC0, this, &result, a3);

            return result;
        }
    }


};
