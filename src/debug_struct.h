#pragma once

#include "variable.h"

#include <cstdint>

struct debug_struct_t {
    uint8_t field_0;
    uint8_t field_1;
    uint8_t field_2;
    uint8_t field_3;

    static inline constexpr auto OUTPUT_WARNING = 0x80u;

    static inline constexpr auto OUTPUT_ERROR = 4u;
};

inline Var<debug_struct_t> g_debug {0x0095FF80};
