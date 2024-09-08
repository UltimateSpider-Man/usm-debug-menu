#pragma once

#include "entity.h"
#include "entity_base_vhandle.h"
#include "float.hpp"

#include <list>

struct resource_key;
struct token_def;
struct token_def_list;

struct wds_token_manager {

    struct active_token {
        token_def *field_0;
        entity field_4;
        bool field_C;
    };

    token_def_list *tokens;
    bool field_4;
    bool editing;
    float field_8;
    int field_C;
    int token_collected_array;
    bool field_14;
    std::list<active_token> field_18;


};

