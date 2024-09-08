#pragma once

#include "float.hpp"
#include "input_mgr.h"
#include "commands.h"

#include <cstdint>

inline constexpr auto GBFLAG_PRESSED = 1;
inline constexpr auto GBFLAG_TRIGGERED = 2;
inline constexpr auto GBFLAG_RELEASED = 4;

class game_button {

    int m_trigger_type;
    device_id_t field_4;
    game_control_t field_8;
    game_button *field_C;
    game_button *field_10;
    float field_14;
    float field_18;

public:
    float field_1C;

private:
    float field_20;
    float field_24;
    float field_28;

public:
    float field_2C;

private:
    int16_t field_30;
    int16_t m_flags;

public:



    bool is_flagged(uint32_t a2) const {
        return (a2 & this->m_flags) != 0;
    }

    void clear_flags() {
        this->m_flags &= 0x20u;
    }

    void set_flag(uint16_t a2, bool a3)
    {
        if ( a3 ) {
            this->m_flags |= a2;
        } else {
            this->m_flags &= ~a2;
        }
    }



};

