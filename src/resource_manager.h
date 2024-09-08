#pragma once

#include "binary_search_array_cmp.h"
#include "float.hpp"
#include "func_wrapper.h"
#include "game.h"
#include "resource_key.h"
#include "resource_pack_location.h"
#include "resource_partition.h"
#include "trace.h"

#include <cassert>

struct resource_partition;
struct resource_pack_location;
struct resource_pack_slot;
struct resource_location;
struct mString;

inline std::vector<resource_partition*>*& partitions = var<std::vector<resource_partition*>*>(0x0095C7F0);

inline std::vector<resource_pack_slot*>& resource_context_stack = var<std::vector<resource_pack_slot*>>(0x0096015C);

inline int& amalgapak_prerequisite_count = var<int>(0x0095C174);

inline mString& amalgapak_name = var<mString>(0x0095CAD4);

#if !STANDALONE_SYSTEM

inline int& amalgapak_base_offset = var<int>(0x00921CB4);

inline int& resource_buffer_used = var<int>(0x0095C180);

inline int& memory_maps_count = var<int>(0x0095C7F4);

inline int& resource_buffer_size = var<int>(0x0095C1C8);

inline int& in_use_memory_map = var<int>(0x00921CB0);

inline uint8_t*& resource_buffer = var<uint8_t*>(0x0095C738);

inline bool& using_amalga = var<bool>(0x0095C800);

inline int& amalgapak_signature = var<int>(0x0095C804);

inline int& amalgapak_pack_location_count = var<int>(0x0095C7FC);

inline resource_pack_location*& amalgapak_pack_location_table = var<resource_pack_location*>(0x0095C7F8);

inline resource_key*& amalgapak_prerequisite_table = var<resource_key*>(0x0095C300);

#else

#define make_var(type, name) \
    static type g_##name {}; \
    type& name { g_##name }

make_var(int, amalgapak_base_offset);

make_var(nflFileID, amalgapak_id);

make_var(int, resource_buffer_used);

make_var(int, memory_maps_count);

make_var(int, resource_buffer_size);

make_var(int, in_use_memory_map);

make_var(uint8_t*, resource_buffer);

make_var(bool, using_amalga);

make_var(int, amalgapak_signature);

make_var(resource_memory_map*, memory_maps);

make_var(int, amalgapak_pack_location_count);

make_var(resource_pack_location*, amalgapak_pack_location_table);

make_var(int, amalgapak_prerequisite_count);

make_var(resource_key*, amalgapak_prerequisite_table);

// make_var(mString, amalgapak_name);

#undef make_var
#endif

namespace resource_manager {

inline void load_amalgapak()
{
    CDECL_CALL(0x00537650);
}

inline void reload_amalgapak()
{
    CDECL_CALL(0x0054C2E0);
}

inline bool using_amalgapak()
{
    return using_amalga;
}
inline bool can_reload_amalgapak()
{

    if constexpr (1) {
        if (using_amalgapak()) {
            return true;
        }

        {
            return (bool)CDECL_CALL(0x0053DE90);
        }
    }
}

inline void frame_advance(Float a2)
{
    auto v8 = os_developer_options::instance()->get_int(mString { "AMALGA_REFRESH_INTERVAL" });

    static float amalga_refresh_timer { 0 };
    amalga_refresh_timer += a2;
    if (v8 > 0 && amalga_refresh_timer > v8) {
        if (can_reload_amalgapak()) {
            reload_amalgapak();
        }

        amalga_refresh_timer = 0.0;
    }

    if constexpr (0) {
        static auto& dword_960CB0 = var<int>(0x00960CB0);

        if (dword_960CB0 == 0) {
            limited_timer timer { 0.02 };

            if (g_game_ptr() != nullptr && g_game_ptr()->field_165) {
                limited_timer v4 { 0.5 };

                timer = v4;
            }

            timer.reset();

            assert(partitions != nullptr);

            for (auto* partition : (*partitions)) {

                assert(partition != nullptr);

                partition->frame_advance(a2, &timer);
            }
        }
    } else {
        CDECL_CALL(0x00558D20, a2);
    }
}

inline int compare_resource_key_resource_pack_location(const resource_key& a1,
    const resource_pack_location& a2)
{
    return false;
}
inline bool get_pack_file_stats(const resource_key& a1, resource_pack_location* a2, mString* a3, int* a4)
{

    if constexpr (0) {
        assert(amalgapak_pack_location_table != nullptr);

        if (a3 != nullptr) {
            *a3 = amalgapak_name.c_str();
        }

        assert(amalgapak_base_offset != -1);

        {
            auto is_sorted = std::is_sorted(amalgapak_pack_location_table,
                amalgapak_pack_location_table + amalgapak_pack_location_count,
                [](auto& a1, auto& a2) {
                    return a1.loc.field_0 = a2.loc.field_0;
                });
            assert(is_sorted);
        }
        if constexpr (1) {
            auto i = 0;
            if (!binary_search_array_cmp<const resource_key, const resource_pack_location>(
                    &a1,
                    amalgapak_pack_location_table,
                    0,
                    amalgapak_pack_location_count,
                    &i,
                    compare_resource_key_resource_pack_location)) {
                return true;
            }

            if (a2 != nullptr) {
                *a2 = amalgapak_pack_location_table[i];
                a2->loc.m_offset += amalgapak_base_offset;
            }

            if (a4 != nullptr) {
                *a4 = i;
            }

            return true;
        } else {
            auto result = (bool)CDECL_CALL(0x0052A820, &a1, a2, a3, a4);
            printf("%s", result ? "true" : "false");
            return result;
        }
    }
    return 1;
}

inline resource_pack_slot* push_resource_context(resource_pack_slot* pack_slot)
{
    return (resource_pack_slot*)CDECL_CALL(0x00542740, pack_slot);
}

inline resource_pack_slot* pop_resource_context()
{
    return (resource_pack_slot*)CDECL_CALL(0x00537530);
}

inline void set_active_district(bool a1)
{
    void(__fastcall * func)(void*, int, bool) = (decltype(func))0x00573620;
    func(nullptr, 0, a1);
}

inline resource_partition* get_partition_pointer(int which_type)
{
    return (resource_partition*)CDECL_CALL(0x00537AA0, which_type);
}

}



