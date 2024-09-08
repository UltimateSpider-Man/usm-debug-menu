#pragma once

#include "float.hpp"

#include <vector>

#include "entity.h"
#include "entity_base_vhandle.h"


#include "color.h"
#include "trace.h"


struct RenderOptimizations;
struct city_lod;
struct nglMesh;
struct camera;

struct render_data {
    struct region_info {
        region *field_0;
    };

    struct entity_info {};

    std::vector<render_data::region_info> field_0;
    std::vector<render_data::entity_info> field_10;
    vector3d field_20;

    void sub_56FCB0();
};

struct wds_render_manager {
    RenderOptimizations *field_0;
    int empty0[3];
    float field_10[4];
    float field_20[4];
    render_data field_30;
    nglMesh *field_5C;
    color field_60;
    int empty2[5];
    float field_84;
    float field_88;
    float field_8C;
    float field_90;
    city_lod *field_94;
    int field_98;


};

