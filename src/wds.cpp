#include "wds.h"

#include "common.h"
#include "game.h"
#include "message_board.h"
#include "utility.h"
#include "variables.h"
#include "actor.h"

#include <cassert>

VALIDATE_OFFSET(world_dynamics_system, field_230, 0x230);


entity *world_dynamics_system::get_hero_ptr(int index)
{
    constexpr auto MAX_GAME_PLAYERS = 1;

    assert(index >= 0 && index <= MAX_GAME_PLAYERS);

    auto *result = this->field_230[index];
    return result;
}

void world_dynamics_system::frame_advance(Float a2)
{
    void (__fastcall *func)(void *, void *, Float) = CAST(func, 0x00558370);
    func(this, nullptr, a2);

    {
        g_game_ptr()->mb->frame_advance(a2);
    }
}

camera* world_dynamics_system::get_chase_cam_ptr(int a2)
{
    auto v1 = this->field_234;

    return bit_cast<camera*>(v1);
}

void world_dynamics_system::deactivate_web_splats()
{
    CDECL_CALL(0x054AD20);
}




int world_dynamics_system::remove_player(int player_num)
{
    void(__fastcall * func)(void*, void*, int) = bit_cast<decltype(func)>(0x00558550);

    func(this, nullptr, player_num);

    return this->num_players;
}

void wds_patch()
{
    {
        auto address = func_address(&world_dynamics_system::frame_advance);
        REDIRECT(0x0055A0F7, address);
    }
}
