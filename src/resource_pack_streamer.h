#pragma once

#include "fixedstring.h"
#include "resource_key.h"
#include "resource_pack_location.h"
#include "resource_pack_slot.h"
#include "limited_timer.h"
#include "float.hpp"
#include "trace.h"
#include <vector>

#include <list>


#ifdef TEST_CASE
#include <list>
#endif

struct limited_timer;
struct resource_pack_token;

struct resource_pack_queue_entry {
    fixedstring<8> field_0;
    int field_20;

};

struct resource_pack_streamer {
    bool active;
    bool currently_streaming;

    std::vector<resource_pack_slot *> *pack_slots;
    resource_key field_8;
    resource_pack_slot *curr_slot;
    int m_slot_index;
    resource_pack_location curr_loc;
    int field_68;

    std::list<resource_pack_queue_entry> field_6C;

    uint8_t *field_78;
    float field_7C;
    int m_data_size;


   auto *get_pack_slots() {
        return this->pack_slots;
    }

   bool is_active() {
        return active;
    }

   bool is_idle() const
    {
    return 0;
}
    
void frame_advance(Float a2, limited_timer* a3)
{
    TRACE("resource_pack_streamer::frame_advance");

        CDECL_CALL(0x005510F0, this, a2, a3);
    
}



 


};

