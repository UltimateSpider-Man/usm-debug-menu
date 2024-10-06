#include "city_lod.h"

#include "common.h"
#include "func_wrapper.h"
#include "ngl.h"
#include "os_developer_options.h"
#include "parse_generic_mash.h"
#include "resource_key.h"
#include "resource_manager.h"
#include "trace.h"
#include "utility.h"

VALIDATE_SIZE(city_lod, 8u);
VALIDATE_SIZE(strip_lod, 0x18);



void city_lod::render()
{
    TRACE("city_lod::render");

    CDECL_CALL(0x00540380, this);
}

void strip_lod::un_mash_start(generic_mash_header *a1,
        void *a2,
        generic_mash_data_ptrs *a3,
        void *a4)
{
    CDECL_CALL(0x0052F380, this, a1, a2, a3, a4);
}

void city_lod_patch()
{
    {
        auto address = func_address(&city_lod::render);
        REDIRECT(0x0054B2FB, address);
    }
}
