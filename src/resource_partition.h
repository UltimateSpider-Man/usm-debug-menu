#pragma once

#include "resource_pack_slot.h"
#include "resource_pack_streamer.h"

struct resource_partition {
    int field_0;
    int field_4;

    struct {
        int field_0;
        resource_pack_slot** m_first;
        resource_pack_slot** m_last;
        resource_pack_slot** m_end;

        struct iterator {
            resource_pack_slot** m_ptr;

            auto& operator*() const
            {
                return (*m_ptr);
            }
        };

        iterator begin()
        {
            return iterator { m_first };
        }

        iterator end()
        {
            return iterator { m_last };
        }

        size_t size() const
        {
            return (m_first == nullptr ? 0 : m_last - m_first);
        }

        auto& front()
        {
            return (*begin());
        }

    } m_pack_slots;

    resource_pack_streamer streamer;
    auto& get_pack_slots()
    {
        return this->m_pack_slots;
    }
    resource_pack_streamer* get_streamer()
    {
        return (&streamer);
    }

    void frame_advance(Float a1, limited_timer* a2)
    {
        this->streamer.frame_advance({ a1 }, a2);
    }
};
