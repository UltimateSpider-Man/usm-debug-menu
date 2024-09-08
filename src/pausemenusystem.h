#pragma once



#include "femenusystem.h"
#include "femanager.h"
#include "variable.h"
#include "float.hpp"
#include "func_wrapper.h"
#include "fetext.h"

struct PanelFile;
struct menu_nav_bar;
struct pause_menu_save_load_display;

struct PauseMenuSystem : FEMenuSystem {
    PanelFile *field_2C;
    menu_nav_bar *field_30;
    pause_menu_save_load_display *field_34;
    int field_38;


    PauseMenuSystem(font_index a2);


    //0x00647E50

    //0x0060C160
    bool IsDialogActivated()    {
        void (__fastcall *func)(void*) = (decltype(func)) 0x0060C160;
        func(this);
    }

    //0x006430F0
    void LoadAll()    {
        void (__fastcall *func)(void*) = (decltype(func)) 0x006430F0;
        func(this);
    }

    //0x0060BEE0
    void Deactivate(){
        void (__fastcall *func)(void*) = (decltype(func)) 0x0060BEE0;
        func(this);
    }


    //0x0060BF10
    //virtual
    void Draw(){
        void (__fastcall *func)(void*) = (decltype(func)) 0x0060BF10;
        func(this);
    }

    //0x0062F0C0
    //virtual
    void Update(Float a2){
        void (__fastcall *func)(void*, Float) = (decltype(func)) 0x0062F0C0;
        func(this, a2);
    }

    //0x00618FC0
    //virtual
    void OnButtonPress(int a2, int a3){
        void (__fastcall *func)(void*, int, int) = (decltype(func)) 0x00618FC0;
        func(this, a2, a3);
    }


};

