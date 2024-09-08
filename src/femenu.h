#pragma once

#include "float.hpp"
#include "input_mgr.h"
#include "utility.h"

#include <cstdint>
#include "vtbl.h"

struct FEMenuSystem;
struct FEText;
struct FEMenuEntry;



struct FEMenu {
    std::intptr_t m_vtbl;
    FEMenuEntry **field_4;
    FEMenuSystem *field_8;
    int field_C;
    int field_10;
    int field_14;
    int field_18;
    float field_1C;
    int16_t field_20;
    int16_t highlighted;
    int16_t num_entries;
    int16_t field_26;
    int16_t field_28;
    char field_2A;
    char field_2B;

    //0x0060AA90
    FEMenu(FEMenuSystem *a2, uint32_t a3, int a4, int a5, int16_t a6, int16_t a7)    {
        void (__fastcall *func)(void*, void*, uint32_t, int , int,int16_t,int16_t) = (decltype(func)) 0x0060AA90;
        func(this,a2,a3,a4,a5,a6,a7);
    }

    //0x00618610
    //virtual
    void AddEntry(int a2, FEText *a3, bool a4)  {

        void (__fastcall *func)(void*,int, void*, bool) = (decltype(func)) 0x0060AA90;
        func(this,a2,a3,a4);
    }


    void Init()
    {
        void(__fastcall * func)(void*) = CAST(func, get_vfunc(m_vtbl, 0xC));
        func(this);
    }

    void Load()
    {
        void(__fastcall * func)(void*) = CAST(func, get_vfunc(m_vtbl, 0x10));
        func(this);
    }

    void Draw()
    {
        void(__fastcall * func)(void*) = CAST(func, get_vfunc(m_vtbl, 0x18));
        func(this);
    }

};

extern void sub_582A30() {

        CDECL_CALL(0x00582A30);
    }

extern void sub_5A6D70() {

        CDECL_CALL(0x005A6D70);
       
    }

extern void FEMenu_patch();
