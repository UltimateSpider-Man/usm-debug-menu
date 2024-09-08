#pragma once

#include "entity_base_vhandle.h"
#include "string_hash.h"

#include <vector>

struct event;
struct event_recipient_entry;
struct event_type;
struct script_executable;

std::vector<event_type*>& event_types = var<std::vector<event_type*>>(0x0095BA48);

namespace event_manager {

void clear_script_callbacks(entity_base_vhandle a1, script_executable* a2)
{
    if constexpr (0) {
        for (auto& v2 : event_types) {
            v2->clear_script_callbacks(a1, a2);
        }
    } else {
        CDECL_CALL(0x004D4380, a1, a2);
    }
}
    int& garbage_index = var<int>(0x0095A6DC);

    }


