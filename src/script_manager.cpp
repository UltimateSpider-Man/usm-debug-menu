

#include "chuck_callbacks.h"
#include "chunk_file.h"
#include "filespec.h"
#include "func_wrapper.h"
#include "game.h"
#include "slab_allocator.h"
#include "mstring.h"
#include "parse_generic_mash.h"
#include "resource_manager.h"
#include "script_executable.h"
#include "script_executable_entry.h"
#include "script_executable_allocated_stuff_record.h"
#include "script_var_container.h"
#include "os_developer_options.h"
#include "os_file.h"
#include "trace.h"
#include "utility.h"
#include "variables.h"
#include "bit.h"


#include <cassert>
#include <list>
#include <set>
#include <map>


#include "float.hpp"


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

enum script_manager_callback_reason {};

#if !STANDALONE_SYSTEM
auto & script_manager_execs_pending_link_list = var<std::list<script_executable_entry> *>(0x00965EF4);

auto & script_manager_execs_pending_first_run = var<std::list<script_executable_entry> *>(0x00965EF8);

auto & script_manager_exec_map = var<std::map<script_executable_entry_key, script_executable_entry> *>(0x00965EE4);

auto & script_manager_initialized = var<bool>(0x00965EE1);

auto & script_manager_next_stuff_id = var<int>(0x00965EFC);

auto & script_manager_script_allocated_stuff_map = var<std::map<int, script_executable_allocated_stuff_record> *>(0x00965F00);

auto & script_manager_master_script = var<script_executable *>(0x00965EE8);

auto & script_manager_game_var_container = var<script_var_container *>(0x00965EEC);

auto & script_manager_shared_var_container = var<script_var_container *>(0x00965EF0);

auto & script_manager_time_inc = var<float>(0x00961930);

auto & script_manager_callbacks = var<std::set<void (*)(script_manager_callback_reason, script_executable *, const char *)> *>(0x00965F04);

#else

#include <list>
#include <set>

#define make_var(type, name) \
    static type g_##name {}; \
    type & name = g_##name

std::list<script_executable_entry> *g_script_manager_execs_pending_first_run {nullptr};
auto & script_manager_execs_pending_first_run {g_script_manager_execs_pending_first_run};

_std::list<script_executable_entry> *g_script_manager_execs_pending_link_list {nullptr};
auto & script_manager_execs_pending_link_list = g_script_manager_execs_pending_link_list;

std::map<script_executable_entry_key, script_executable_entry> *g_script_manager_exec_map {nullptr};
std::map<script_executable_entry_key, script_executable_entry> *& script_manager_exec_map = g_script_manager_exec_map;

static bool g_script_manager_initialized {false};
bool & script_manager_initialized = g_script_manager_initialized;

static int g_script_manager_next_stuff_id {};
int & script_manager_next_stuff_id {g_script_manager_next_stuff_id};

static _std::map<int, script_executable_allocated_stuff_record> *
    g_script_manager_script_allocated_stuff_map {nullptr};
std::map<int, script_executable_allocated_stuff_record> *& script_manager_script_allocated_stuff_map = g_script_manager_script_allocated_stuff_map;

make_var(script_executable *, script_manager_master_script);

make_var(script_var_container *, script_manager_game_var_container);

make_var(script_var_container *, script_manager_shared_var_container);

make_var(float, script_manager_time_inc);

make_var(_std::set<void (*)(script_manager_callback_reason, script_executable *, const char *)> *, script_manager_callbacks);

#undef make_var

#endif

namespace script_manager {

void * get_game_var_address(const mString &a1, bool *a2, script_library_class **a3)
{
    TRACE("script_manager::get_game_var_address", a1.c_str());

    assert(script_manager_game_var_container != nullptr);
    assert(script_manager_shared_var_container != nullptr);

    auto result = script_manager_game_var_container->get_script_var_address(a1.c_str(), a3);
    if (result != nullptr)
    {
        if (a2 != nullptr) {
            *a2 = true;
        }
    }
    else
    {
        result = script_manager_shared_var_container->get_script_var_address(a1.c_str(), a3);
        if (a2 != nullptr) {
            *a2 = false;
        }
    }

    if (result == nullptr && a3 == nullptr)
    {
        mString v1{"unknown game/shared var "};
        v1 += a1;

        printf("%s", v1.c_str());
        assert(0);
    }

    return result;
}

char * get_game_var_address(int a1)
{
    TRACE("script_manager::get_game_var_address");

    assert(script_manager_game_var_container != nullptr);
    return script_manager_game_var_container->get_address(a1);
}

char * get_shared_var_address(int a1)
{
    TRACE("script_manager::get_shared_var_address");

    assert(script_manager_shared_var_container != nullptr);
    return script_manager_shared_var_container->get_address(a1);
}

bool using_chuck_old_fashioned() {
    return os_developer_options::instance()->get_flag(mString { "CHUCK_OLD_FASHIONED" });
}

int get_total_loaded()
{
    int result = (script_manager_exec_map != nullptr
                    ? script_manager_exec_map->size()
                    : 0
                    );
    return result;
}


void link()
{
    TRACE("script_manager::link");

    if constexpr (1) {
        for (auto& entry : (*script_manager_execs_pending_link_list)) {
            {
                script_manager::run_callbacks(static_cast<script_manager_callback_reason>(6), entry.exec, entry.field_8);
                script_manager::run_callbacks(static_cast<script_manager_callback_reason>(7), entry.exec, entry.field_8);
        }
   

            script_manager_execs_pending_first_run->push_back(entry);
        }

        script_manager_execs_pending_link_list->clear();
    } else {
        CDECL_CALL(0x005A3620);
    }
}


void init_game_var()
{
    CDECL_CALL(0x0059EE90);
}

void clear()
{
    CDECL_CALL(0x005B0640);
}

void run(Float a1, bool a2)
{
    CDECL_CALL(0x005AF9F0, a1, a2);
}


int load_game_var_buffer(char *a1)
{
    TRACE("script_manager::load_game_var_buffer");

    if (a1 == nullptr || script_manager_game_var_container == nullptr) {
        return 0;
    }

    assert(script_manager_game_var_container != nullptr);

    return script_manager_game_var_container->load_script_var_buffer(a1);
}

void run_callbacks(script_manager_callback_reason a1, script_executable *a2, const char *a3)
{
    TRACE("script_manager::run_callbacks");

    assert(script_manager_callbacks != nullptr);

    if constexpr (1) {
        for ( auto &cb : (*script_manager_callbacks) )
        {
            cb(a1, a2, a3);
        }
    } else {
        CDECL_CALL(0x005A0AC0, a1, a2, a3);
    }
}

bool is_loadable(const resource_key &a1)
{
    TRACE("script_manager::is_loadable");

     CDECL_CALL(0x0058F3A0, a1);


}

int register_allocated_stuff_callback(
        void (*a1)(script_executable *, std::list<uint32_t> &, std::list<mString> &))
{
    TRACE("script_manager::register_allocated_stuff_callback");

    if ( script_manager_script_allocated_stuff_map == nullptr ) {
        using map_t = std::decay_t<decltype(*script_manager_script_allocated_stuff_map)>;
        script_manager_script_allocated_stuff_map = new map_t {};
        assert(script_manager_script_allocated_stuff_map != nullptr);
    }

    auto v7 = script_manager_next_stuff_id++;
    script_executable_allocated_stuff_record a2{};
    a2.field_0 = a1;
    script_manager_script_allocated_stuff_map->insert({v7, a2});
    return v7;
}

script_executable_entry * load(const resource_key &a1, uint32_t a2, void *a3, const resource_key &a4)
{

        script_executable_entry * (*func)(const resource_key *a1, uint32_t a2, void *a3, const resource_key *a4) = CAST(func, 0x005B0750);
        return func(&a1, a2, a3, &a4);
    }
}


float get_time_inc()
{
    return script_manager_time_inc;
}

void init()
{
    TRACE("script_manager::init");


        CDECL_CALL(0x005AFCE0);
    }

void clear()
    {
        TRACE("script_manager::clear");

        if constexpr (1) {
            script_manager_time_inc = 0.0;
            script_manager_master_script = nullptr;

            assert(script_manager_exec_map != nullptr);
            while (!script_manager_exec_map->empty()) {
                auto it = script_manager_exec_map->begin();
                auto* exec = it->second.exec;



                script_manager_exec_map->erase(it);
            }

            assert(script_manager_exec_map->size() == 0);

            script_manager_execs_pending_link_list->clear();
            script_manager_execs_pending_first_run->clear();

            assert(script_manager_callbacks != nullptr);
            script_manager_callbacks->clear();
        } else {
            CDECL_CALL(0x005B0640);
        }
    }


void kill()
{
    TRACE("script_manager::kill");

    assert(script_manager_initialized);

    script_manager::clear();
    if ( script_manager_execs_pending_link_list != nullptr )
    {
        delete script_manager_execs_pending_link_list;
        script_manager_execs_pending_link_list = nullptr;
    }

    if ( script_manager_execs_pending_first_run != nullptr )
    {
        delete script_manager_execs_pending_first_run;
        script_manager_execs_pending_first_run = nullptr;
    }

    if ( script_manager_exec_map != nullptr )
    {
        delete script_manager_exec_map;
        script_manager_exec_map = nullptr;
    }

    if ( script_manager_script_allocated_stuff_map != nullptr )
    {
        delete script_manager_script_allocated_stuff_map;
        script_manager_script_allocated_stuff_map = nullptr;
    }

    if ( script_manager_callbacks != nullptr )
    {
        delete script_manager_callbacks;
        script_manager_callbacks = nullptr;
    }

    script_manager_initialized = false;
}

FILE * host_fopen(const char *a1, char arg4)
{
    char v14[3] {};
    auto v7 = arg4 & 7;
    switch ( v7 )
    {
    case 1:
        v14[0] = 'r';
        break;
    case 2:
        v14[0] = 'w';
        break;
    case 4:
        v14[0] = 'a';
        break;
    default:
        v14[0] = 'r';
        break;
    }

    auto v8 = arg4 & 0x18;
    if ( v8 == 8 ) {
        v14[1] = 'b';
    } else if ( v8 == 16 ) {
        v14[1] = 't';
    } else {
        v14[1] = 'b';
    }

    v14[2] = 0;

    mString v13 {a1};
    if ( v13.at(1) != ':' && v13.at(0) != '\\' ) {
#if 0
        if ( !aDData[0] ) {
            debug_print_va("Trying to use os_file::root_dir before it has been set in host_fopen\n");
        }
#endif

        mString a2 {"C:\\usm\\pc_usm\\data\\"};
        v13 = a2 + mString {a1};
    }

    auto *v2 = v13.c_str();
    auto *v12 = fopen(v2, v14);
    return v12;
}

void dump_threads_to_console()
{
}



void run(Float a1, bool a2)
{

        CDECL_CALL(0x005AF9F0, a1, a2);
    }


void destroy_game_var() {
    TRACE("script_manager::destroy_game_var");


        CDECL_CALL(0x005A52F0);
    }


std::map<script_executable_entry_key, script_executable_entry> *get_exec_list()
{
    return script_manager_exec_map;
}




int save_game_var_buffer(char* a1)
{
    TRACE("script_manager::save_game_var_buffer");
    printf("0x%08X", int(a1));

    return CDECL_CALL(0x0058F4C0, a1);
}

int register_callback(
    void (*a2)(script_manager_callback_reason, script_executable*, const char*))
{
    TRACE("script_manager::register_callback");

    return CDECL_CALL(0x005A3600, a2);
}



void init_game_var()


    {

        CDECL_CALL(0x0059EE90);
    }




void clear()


    {
        CDECL_CALL(0x005B0640);
    }









