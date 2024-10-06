#pragma once

#include "func_wrapper.h"
#include "variable.h"

#include <cassert>

#include <cstdarg>
#include <cstdio>
#include <stdlib.h>    
#include <math.h>
#include <windows.h>

inline int& mString_count = var<int>(0x00957CEC);

inline char *& null = var<char *>(0x0091E7C0);

struct mString
{
    int field_0;
    int m_size;
    const char *guts;
    int field_C;

    mString()
    {
        void (__fastcall *func)(void *) = bit_cast<decltype(func)>(0x00420F00);
        func(this);
    }

    mString(const mString &a2)
    {
        void (__fastcall *func)(void *, int, const mString *) = bit_cast<decltype(func)>(0x00421220);
        func(this, 0, &a2);
    }

    mString(int , const char *Format, ...)
    {
        char Dest[1024];
        va_list Args;

        va_start(Args, Format);
        vsprintf(Dest, Format, Args);
        CDECL_CALL(0x00420F60, this, 0, Dest);
    }

    mString(const char *str)
    {
        typedef int (__fastcall* mString_constructor_ptr)(mString* , void* edx, const char* str);
        mString_constructor_ptr mString_constructor = (mString_constructor_ptr) 0x00421100;
        mString_constructor(this, 0, str);
    }

    ~mString()
    {
        typedef int (__fastcall* mString_finalize_ptr)(mString* , void* edx, int zero);
        mString_finalize_ptr mString_finalize = (mString_finalize_ptr) 0x004209C0;

        mString_finalize(this, 0, 0);
    }



     void finalize(int zero)
    {
         void(__fastcall * func)(void*, int) = bit_cast<decltype(func)>(0x004209C0);
        func(this, zero);
    }

    mString &operator=(const mString &a2)
    {
        if (this != &a2)
        {
            this->update_guts(a2.c_str(), a2.m_size);
        }

        return (*this);
    }

    double to_float() const
    {
        return atof(this->guts);
    }

    mString &operator=(const char *a2)
    {
        this->update_guts(a2, -1);
        return (*this);
    }

    int length() const {
        return this->m_size;
    }


    void update_guts(const char* a2, signed int a3) {

        typedef void(__fastcall* mString_update_guts_ptr)(void *, void* edx, const char* a2, signed int a3);
        mString_update_guts_ptr mString_update_guts = (mString_update_guts_ptr) 0x41F9D0;

        //printf("mString:%s\n", a2);


        return mString_update_guts(this, 0, a2, a3);
    }


    int sub_1066D20(mString*, int a2)
    {
        if (!a2) {
            this->guts = null;
            this->m_size = 0;
        }
        return ++mString_count;
    }

    int sub_6657FD(mString*, int a2)
    {
        return sub_1066D20(this, a2);
    }
    void sub_6A1D7F(mString*, mString* a1)
    {
    }

    mString* sub_1067DC0(mString*, const char* a1)
    {
        update_guts(a1, -1);
        return this;
    }

    mString* __as(int, const char* a1)
    {
        return sub_1067DC0(this, a1);
    }


    mString from_char(const char* a2);



    void sub_691A3D(mString* _this, mString* a1)
    {

    }

        char operator[](int i) const
    {
        assert(guts != nullptr);
        assert(i <= (int)m_size);

        assert(i >= 0);

        return this->guts[i];
    }

    char at(int i) const
    {
        return (*this)[i];
    }




    mString* mString_1(mString* a1, float a2)
    {
        const char* v4 = "%0.3f";
        char a3[128];
        snprintf(a3, 128, "%0.3f", a2);
        sub_6657FD(a1, 0);
        update_guts(v4, -1);
        return a1;
    }

    mString* from_char(char* a2)
    {
        this->sub_6657FD(this, 0);
        if (a2) {
            this->update_guts(a2, -1);
        }
        return this;
    }

        inline bool empty() const
    {
        return m_size == 0;
    }

    const char *c_str() const
    {
        return guts;
    }

    void append(const char *from_string, size_t Count)
    {
        void (__fastcall *func)(void *, int, const char *, unsigned int) = bit_cast<decltype(func)>(0x0041FAB0);
        func(this, 0, from_string, Count);
    }

    mString &operator+=(const mString &a1)
    {
        this->append(a1.c_str(), -1);
        return (*this); 
    }

    friend mString operator+(const char *a2, const mString &a3)
    {
        mString result;
        CDECL_CALL(0x00421410, &result, a2, &a3);
        return result;
    }

    friend mString operator+(const mString &a2, const mString &a3)
    {
        mString result;
        CDECL_CALL(0x00421330, &result, &a2, &a3);
        return result;
    }

};


