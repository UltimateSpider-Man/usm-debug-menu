#include "script_var_container.h"

#include "func_wrapper.h"
#include "chunk_file.h"
#include "common.h"
#include "memory.h"
#include "parse_generic_mash.h"
#include "script_library_class.h"
#include "script_manager.h"
#include "slc_manager.h"
#include "trace.h"

#include <cassert>
#include <map>

VALIDATE_SIZE(script_var_container, 0x10u);

static constexpr auto SCRIPT_VAR_FLAG_UN_MASHED = 4u;



void script_var_container::script_var_to_addr_t::un_mash(
        void *,
        generic_mash_header *,
        script_var_container *a3,
        void *,
        generic_mash_data_ptrs *a5)
{
    a5->rebase(4);

    int v6 = *a5->get<int>();

    this->name = a5->get<char>(v6);

    printf("is_game = %s", a3->is_game() ? "true" : "false");

    int addr = ( a3->is_game()
            ? (int) script_manager::get_game_var_address(this->address)
            : (int) script_manager::get_shared_var_address(this->address)
            );

    printf("0x%X", addr);

    this->address = addr;
}

script_var_container::script_var_container()
{
    this->script_var_to_addr = nullptr;
    this->field_10 = 0;
    this->debug_info = nullptr;
    this->flags = 0;
}






void script_var_container::destroy()
{
    CDECL_CALL(0x005A3370, this);
}



void *script_var_container::get_script_var_address(const char *a2, script_library_class **a3) {
    return (void *) CDECL_CALL(0x005A0520, this, a2, a3);
}



