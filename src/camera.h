#pragma once

#include "common.h"
#include "entity.h"
#include "func_wrapper.h"
#include "geometry_manager.h"
#include "memory.h"
#include "trace.h"
#include "utility.h"
#include "vector3d.h"
#include "vtbl.h"

struct vector3d;
struct string_hash;
struct entity;
struct mic;


struct camera : actor {
    mic *field_C0;
    float field_C4;
    float field_C8;




    void sync(camera& a2)
    {
        if constexpr (1) {
            void(__fastcall * func)(camera*, void*, camera*) = CAST(func, get_vfunc(m_vtbl, 0x294));

            func(this, nullptr, &a2);

        } else {
            CDECL_CALL(0x0057EFD0, this, &a2);
        }
    }

    void set_fov(Float fov)
    {
        void(__fastcall * func)(void*, void*, Float) = CAST(func, get_vfunc(m_vtbl, 0x29C));
        func(this, nullptr, fov);
    }

    float get_fov()
    {
        float(__fastcall * func)(void*) = CAST(func, get_vfunc(m_vtbl, 0x2A0));
        return func(this);
    }

    float get_far_plane_factor()
    {
        float(__fastcall * func)(void*) = CAST(func, get_vfunc(m_vtbl, 0x2A8));
        return func(this);
    }

    bool _is_a_camera() const
    {
        TRACE("camera::is_a_camera");

        return true;
    }

    bool is_externally_controlled() const
    {
        return this->is_flagged(0x400000);
    }




};



inline Var<camera*> g_camera_link {0x0095C720};

struct region;

