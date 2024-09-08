#pragma once

#include "entity_base_vhandle.h"
#include "float.hpp"
#include "game_button.h"
#include "mvector.h"

#include <vector>
#include "func_wrapper.h"

struct entity_base;
struct cut_scene;
struct nalStreamInstance;
struct cut_scene_segment;

struct cut_scene_player {
    cut_scene* current_cut_scene;
    mVector<cut_scene_segment>::iterator current_segment;
    int field_8;
    int field_C;
    char field_10;
    int field_14;
    int field_18;
    int field_1C;
    int field_20;
    int field_24;
    int field_28;
    int field_2C;
    int field_30;
    int field_34;
    int field_38;
    int field_3C;
    int field_40;
    int field_44;
    std::vector<nalStreamInstance*> field_48;
    std::vector<nalStreamInstance*> field_58;
    int field_68[5];
    void* field_7C;
    int field_80;
    int field_84;

    int field_88[12];

    int field_B8;
    std::vector<entity_base_vhandle> field_BC;
    entity_base* field_CC;
    entity_base* field_D0;
    bool field_D4;
    bool field_D5;
    bool field_D6;
    bool field_D7;
    bool m_traffic_enabled;
    bool m_peds_enabled;
    int field_DC;
    bool field_E0;
    bool field_E1;
    bool field_E2;
    game_button field_E4;
    game_button field_118;
    int field_14C;
    int field_150;
    float field_154;
    float field_158;
    int field_15C;
    int field_160;
    int field_164;



// 0x007411C0
extern cut_scene_player* g_cut_scene_player();


