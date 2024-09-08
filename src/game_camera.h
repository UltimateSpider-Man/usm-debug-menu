#pragma once

#include "camera.h"

struct game_camera : camera {

    static inline constexpr auto CAMERA_SHAKES_TOTAL = 4u;

    struct _camera_shake_t {
        int field_0;
        int field_4;
        int field_8;
        int field_C;
        int empty[2];
        short field_18;

        char field_1A;

    };

    float field_D0[16];
    int empty[2];

    vector3d field_11C;
    float field_128;
    bool field_12C;
    char pad[3];

    _camera_shake_t field_130[CAMERA_SHAKES_TOTAL];

    int empty1[1];

    game_camera() = default;

};

