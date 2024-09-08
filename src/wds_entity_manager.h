#pragma once

#include "float.hpp"
#include "string_hash.h"
#include "entity.h"

#include <list>
#include <vector>


#include <cassert>


struct box_trigger;
struct camera;
struct convex_box;
struct entity;
struct po;
struct mString;
struct region;
struct item;
struct vector3d;

struct wds_entity_manager {
    int field_18;
    void *field_1C;
    int field_20;
    int field_24;
    entity *field_28;

    bool remove_item(item* a2)
    {
        return (bool)CDECL_CALL(0x005D5410, this, a2);
    }

    bool remove_entity(entity* a2)
    {
        return (bool)CDECL_CALL(0x005D5350, this, a2);
    }


    void destroy_entity(entity* e)
    {
        assert(e != nullptr);

        if constexpr (1) {
            bool v4;

            auto v3 = e->get_flavor() - 9;
            if (v4 && v4 == 2) {
                v4 = this->remove_item((item*)e);
            } else {
                v4 = this->remove_entity(e);
            }

            if (v4) {
                if ((e->field_8 & 0x80000000) == 0) {
                    e->release_mem();
                } else {
                    e->~entity();
                }
            }
        } else {
            CDECL_CALL(0x005D6F20, this, e);
        }
    }


};

