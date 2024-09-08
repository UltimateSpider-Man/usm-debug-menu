#pragma once

#include "float.hpp"
#include "limited_timer.h"
#include "variable.h"

struct mString;

inline constexpr auto SMOKE_LEVEL_MAX_NUM = 96;

struct smoke_test {
    bool field_0;
    float field_4;
    float field_8;
    float field_C;
    limited_timer_base field_10;
    int field_14;
    int field_18;
    int smoke_lvl_num;

    struct {
        mString *field_0;
        float field_4;
    } field_20[SMOKE_LEVEL_MAX_NUM];

    //0x00578F40
    smoke_test(const char **a2, Float a3);

    //0x0057B740
    void frame_advance();

    void sub_57B610();
};

extern Var<smoke_test *> g_smoke_test;
