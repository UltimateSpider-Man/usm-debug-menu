#pragma once


#include "ngl_math.h"
#include "variable.h"
#include "vector2d.h"
#include "vector4d.h"
#include "camera.h"
#include "color.h"

struct cut_scene_player;
struct vector3d;
struct actor;
struct entity;
struct signaller;
struct nglTexture;

namespace nalPanel {
struct nalPanelAnim;
}

namespace comic_panels {

    void render()
{
    TRACE("comic_panels::render");

    CDECL_CALL(0x0073EA70);
}

struct panel;

struct panel_params_t {
    uint32_t field_0;
    panel *field_4;
    struct {
        vector4d field_0;
        vector4d field_10;
        int field_20;
        int field_24;
    } field_8[5] {};
    uint8_t field_D0;
    uint8_t field_D1;
};

struct panel_component {

    struct render_info {

        math::MatClass<4, 3> field_0;
        panel_params_t field_40;
        float field_134;
        float field_138;
        panel *field_13C;
        bool field_140;
        bool field_141;
        bool field_142;
        bool field_143;
        bool field_144;

 
    };

    int m_vtbl;
    panel_component *field_4;

};


struct panel_component_base;

struct panel {
    std::intptr_t m_vtbl;
    vector2d m_size;
    cut_scene_player *field_4C;
    float field_50;
    int field_54;
    float field_58;
    bool field_5C;
    panel_component_base *field_60;
    char field_64;
    char field_65;
    char field_66;
    bool field_67;
    bool field_68;


};

struct panel_component_camera : panel_component {

    int field_18;
    int field_1C;
    bool field_20;
    float field_24;
    int field_28;
    int field_2C;
    int field_30;
    nglTexture *field_44;
    char field_48;


};

struct panel_component_base {
    int m_vtbl;
    panel_component_base *field_4;
    float field_8;
    bool field_C;


};





extern Var<camera *> current_view_camera;

extern Var<panel *> game_play_panel;

extern Var<fixed_vector<panel *, 48>> panels;

inline Var<color> default_bgcol {0x009392EC};

extern bool & world_has_been_rendered;

}

