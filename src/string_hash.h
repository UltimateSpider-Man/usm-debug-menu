#pragma once

#include "bit.h"
#include "string_hash_dictionary.h"
#include "func_wrapper.h"

struct string_hash
{
    unsigned int source_hash_code;

    string_hash() = default;

    string_hash(const char *a1)
    {
        void (__fastcall *func)(void *, int idx, const char *) = bit_cast<decltype(func)>(0x00401960);
        func(this, 0, a1);
    }

    const char *to_string() const
    {
        const char * (__fastcall *func)(const void *) = (decltype(func)) 0x005374B0;
        return func(this);
    }

    void initialize(int a2, const char *a3, int a4)
    {
        typedef void(__fastcall* string_hash_initialize_ptr)(string_hash* , void* edx, int a2, const char* Str1, int a4);
        string_hash_initialize_ptr string_hash_initialize = (string_hash_initialize_ptr) 0x00547A00;
        string_hash_initialize(this, nullptr, a2, a3, a4);
    }

    int* set(const char* str)
    {
        if constexpr (1) {
            if (str == nullptr || str[0] != '\0') {
                string_hash* a2 = 0;
                string_hash* v3 = string_hash_dictionary::register_string(a2, str);

            } else {
                this->source_hash_code = 0;
            }
        } else {
            CDECL_CALL(0x00542710, this, str);
        }
        return 0;
    }
};


