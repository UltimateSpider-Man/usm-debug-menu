#pragma once

#include "float.hpp"
#include "common.h"
#include "func_wrapper.h"

#include "utility.h"

#include <list>
#include <map>

#if STANDALONE_SYSTEM
#include <map>
#endif

struct mString;
struct script_var_container;
struct script_library_class;
struct resource_key;
struct script_executable_entry_key;
struct script_executable_entry;
struct script_executable;
struct script_executable_allocated_stuff_record;
struct script_object;
struct string_hash;
struct vm_executable;

enum script_manager_callback_reason
{};

namespace script_manager {
static void init_game_var()
{
    CDECL_CALL(0x0059EE90);
}

static void clear()
{
    CDECL_CALL(0x005B0640);
}

static void run(Float a1, bool a2)
{
    CDECL_CALL(0x005AF9F0, a1, a2);
}

static void link()
{
    CDECL_CALL(0x005A3620);
}



static bool is_loadable(const resource_key& a1)
{

    if constexpr (1) {
            return true;
        

        {
              
            bool* (*func)(const resource_key* a1) = CAST(func, 0x0058F3A0);
            return func(&a1);
        }
    }
}






    // 0x005B0750
static script_executable_entry* load(const resource_key& a1, uint32_t a2, void* a3, const resource_key& a4)
{
    script_executable_entry* (*func)(const resource_key* a1, uint32_t a2, void* a3, const resource_key* a4) = CAST(func, 0x005B0750);
    return func(&a1, a2, a3, &a4);
}


   
    
#if !STANDALONE_SYSTEM
    std::map<script_executable_entry_key, script_executable_entry> *get_exec_list();
#else
    std::map<script_executable_entry_key, script_executable_entry> *get_exec_list();
#endif

    //0x0059ED20
    script_object *find_object(const string_hash &a1);
    
    //0x005A0870
    script_object *find_object(
        const resource_key &,
        const string_hash &,
        const resource_key &);

    //0x005AFE40
    int register_allocated_stuff_callback(
        void (*a1)(script_executable *, std::list<uint32_t> &, std::list<mString> &));

    script_object *find_global_object();

    float get_time_inc();

    static int register_callback(
        void (*a2)(script_manager_callback_reason, script_executable *, const char *));
}


