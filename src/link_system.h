#pragma once

#include "float.hpp"

struct link_system {
    // 0x0051ABD0
    static bool use_link_system();

    // 0x0051ABA0
    static void send_command();

    static void frame_advance(Float);
};