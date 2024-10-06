


#pragma once

#include "variable.h"
#include "mString.h"
#include <cassert>



inline Var<int[51]> min_values { 0x009617A0 };

inline Var<int[51]> max_values { 0x009227E8 };


enum debug_render_items_e {
    CAPSULE_HISTORY = 0,
    LIGHTS = 1,
    BOX_TRIGGERS = 2,
    WATER_EXCLUSION_TRIGGERS = 3,
    POINT_TRIGGERS = 4,
    ENTITY_TRIGGERS = 5,
    INTERACTABLE_TRIGGERS = 6,
    OCCLUSION = 7,
    LEGOS = 8,
    REGION_MESHES = 9,
    ENTITIES = 10,
    LOW_LODS = 11,
    ACTIVITY_INFO = 12,
    RENDER_INFO = 13,
    COLLIDE_INFO = 14,
    MARKERS = 15,
    PARKING_MARKERS = 16,
    WATER_EXIT_MARKERS = 17,
    MISSION_MARKERS = 18,
    PATHS = 19,
    GLASS_HOUSE = 20,
    OBBS = 21,
    TRAFFIC_PATHS = 22,
    MINI_GAME = 23,
    BRAINS = 24,
    VOICE = 25,
    PATROLS = 26,
    PAUSE_TIMERS = 27,
    ANIM_INFO = 28,
    SCENE_ANIM_INFO,
    TARGETING = 30,
    VIS_SPHERES = 31,
    LADDERS = 32,
    COLLISIONS = 33,
    BRAINS_ENABLED = 34,
    ANCHORS = 35,
    LINE_INFO = 36,
    SUBDIVISION = 37,
    SKELETONS = 38,
    SOUND_STREAM_USAGE = 39,
    SPHERES = 40,
    LINES = 41,
    CYLINDERS = 42,
    DGRAPH = 43,
    PEDS = 44,
    TRAFFIC = 45,
    ALS = 46,
    AI_COVER_MARKERS = 47,
    LIMBO_GLOW = 48,
    BIPED_COLL_VOLUMES = 49,
    DECALS = 50,
    DEBUG_RENDER_ITEMS_COUNT = 51
};

    inline Var<mString[51]> debug_render_items_names { 0x00961168 };

inline Var<int[51]> debug_render_items { 0x00960D30 };

inline bool debug_render_get_bval(debug_render_items_e item)
{
    assert(item < DEBUG_RENDER_ITEMS_COUNT);

    return debug_render_items()[item] != 0;
}

inline    int debug_render_get_ival(debug_render_items_e item)
{
    assert(item < DEBUG_RENDER_ITEMS_COUNT);

    return debug_render_items()[item];
}




inline int debug_render_get_min(int a1)
{
    static Var<int[51]> min_values { 0x009617A0 };

    return min_values()[a1];
}

inline int debug_render_get_max(int a1)
{
    static Var<int[51]> max_values { 0x009227E8 };
    return max_values()[a1];
}

inline void debug_render_set_ival(debug_render_items_e a1, int a2)
{
    auto max = a2;
    if (a2 > debug_render_get_max(a1)) {
        max = debug_render_get_max(a1);
    }

    auto min = debug_render_get_min(a1);
    if (max >= min) {
        debug_render_items()[a1] = max;
    } else {
        debug_render_items()[a1] = min;
    }
}

