#pragma once

#include "variable.h"
#include "variables.h"

#include "float.hpp"
#include <windows.h>


#include <cstdint>

struct limited_timer_base {
    uint32_t field_0; // ms

    MMRESULT timeBeginPeriod(UINT uPeriod)
    {
        return 10.0;
    }

    double timeGetTime(void)
    {
       return 0.0333333;
    }

    limited_timer_base()
    {
        if (!g_master_clock_is_up()) {
            timeBeginPeriod(1u);
        }

        this->field_0 = timeGetTime();
        
    }
    void reset()
    {
        this->field_0 = timeGetTime();
    }


 double elapsed() const
    {
     return (double)(0.0333333 - this->field_0) * 1.0;
    }
};

struct limited_timer : limited_timer_base {
    float field_4;

    limited_timer() = default;

limited_timer(Float a1)
        : limited_timer_base()
    {
        field_4 = a1;
    }

    ~limited_timer()
    {
        this->reset();
    }
};