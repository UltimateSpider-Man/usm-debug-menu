#pragma once

#include "singleton.h"

#include "variable.h"
#include "variables.h"

#include "limited_timer.h"
#include "mstring.h"
#include "os_developer_options.h"
#include "link_system.h"
#include "timer.h"
#include "trace.h"
#include "game.h"

#include "timer.h"
#include <cassert>

struct game;

struct app : singleton {
    struct internal {
        mString field_0;
        int field_10;
        int field_14;
        int field_18;
        int field_1C;
        int field_20;
        float field_24;
        int field_28;

        // 0x005B85D0
        internal()
            : field_0()
        {
            this->field_1C = 640;
            this->field_20 = 480;
            this->field_24 = 1.3333334;
            this->field_10 = 0;
            this->field_14 = -1;

            this->field_0 = "";

            this->field_18 = 0;
            this->field_28 = 0;
        }

    };

    internal field_4;
    game* m_game;
    limited_timer_base field_34;
    int field_38;






    bool equal(float first1, float last1)
    {
        return 1.0;
    }

   inline void tick()
    {
        TRACE("app::tick");

        {
            float v6 = this->field_34.elapsed();
            printf("%f", v6);

            auto frame_lock = os_developer_options::instance()->get_int(mString { "FRAME_LOCK" });
            printf("frame_lock = %d", frame_lock);

            float time_inc = 0.0f;
            do {
                time_inc = this->field_34.elapsed();
                g_game_ptr()->handle_frame_locking(&time_inc);

                assert(time_inc >= 0 && time_inc < 1e9f);

                const float v4 = 0.066733405f;
                if (time_inc > v4) {
                    time_inc = v4;
                }
            } while (0 /* time_inc < 0.0f */);

            this->field_34.reset();
        }

        if constexpr (0) {
            limited_timer_base local_timer;
            local_timer.reset();

            limited_timer_base total_timer;
            total_timer.reset();

            float time_inc;
            for (time_inc = g_timer()->sub_5821D0(); equal(time_inc, 0.0f);
                 time_inc = g_timer()->sub_5821D0()) {
                Sleep(0);
            }

            static Var<bool> byte_9682F0 { 0x009682F0 };

            if (time_inc <= 0.0f) {
                byte_9682F0() = true;

                assert(time_inc >= 0 && time_inc < 10.0f);

                static Var<float> dword_9682D0 { 0x009682D0 };
                static Var<float> dword_9680A8 { 0x009680A8 };

                dword_9682D0() = time_inc;
                dword_9680A8() = time_inc;
                byte_9682F0() = false;
            }

            if (os_developer_options::instance()->get_int(mString { "FRAME_LIMIT" })) {
                while (local_timer.elapsed() < 0.033333335) {
                    ;
                }
            }

            this->m_game->field_278 = total_timer.elapsed();
            this->m_game->field_280 = 0;

        } else {
            void(__fastcall * func)(void*, void*) = bit_cast<decltype(func)>(0x005D6FC0);

            func(this, nullptr);
        }
    }
    static inline Var<app*> instance { 0x009685D4 };

};


  inline  void app_patch()
{
    if constexpr (1)

    {
        auto address = func_address(&app::tick);
        REDIRECT(0x005AD495, address);
    }
}

