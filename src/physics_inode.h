#pragma once

#include "physical_interface.h"
#include "string_hash.h"
#include "variable.h"


#include "po.h"
struct entity_base;
struct vector3d;
struct po;
struct from_mash_in_place_constructor;

namespace ai {

struct physics_inode {
    physical_interface *field_1C;



    //0x0068B030
    po& get_abs_po()
        {
            void(__fastcall * func)(void*, void*) = (decltype(func))0x0068B030;
            func(this, nullptr);
        }

    

  
};
}
