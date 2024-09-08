#include "text_file.h"

#include "filespec.h"
#include "func_wrapper.h"
#include "memory.h"
#include "trace.h"
#include "variables.h"
#include "os_file.h"


#include <cassert>








bool text_file::at_eof() const {
    return this->bufpos >= this->bufamt && (io.field_15 || opened);
}

char text_file::peek_char() const {
    if ( this->field_54 >= 0 ) {
        return this->field_54;
    }

    assert(bufpos < bufamt);
    return this->buffer[this->bufpos];
}


void text_file::eat_whitespace()
{
    CDECL_CALL(0x005CBD10, this);
}

void text_file::read(char *a2, int maxlen) {
    if constexpr (1) {
        int v26 = 0;
        auto *cp = a2;
        this->eat_whitespace();
        if (this->at_eof() || this->peek_char() != '\"' )
        {
            while ( !this->at_eof() )
            {
                auto v3 = this->peek_char();
                if ( isspace(v3) ) {
                    break;
                }

                if ( v26 >= maxlen )
                {
                    *cp = 0;
                    mString v5 = this->get_name() + ": string too long for buffer: " + a2;
                    printf(v5.c_str());
                    assert(0);
                }

                *cp++ = this->read_char();
                ++v26;
            }
        }
        else
        {
            this->read_char(); // eat first quote
            bool v22 = false;
            while ( !this->at_eof() && !v22 )
            {
                auto v24 = this->read_char();
                if ( v24 == '\"' )
                {
                    v22 = true;
                }
                else if ( v24 == '\\' )
                {
                    auto v11 = this->peek_char();
                    if ( v11 == '\"' )
                    {
                        *cp++ = '\"';
                        this->read_char();
                    }
                    else if ( v11 == '\\' )
                    {
                        *cp++ = '\\';
                        this->read_char();
                    }
                    else
                    {
                        *cp++ = v24;
                    }
                }
                else
                {
                    *cp++ = v24;
                }
            }
        }

        *cp = 0;
#if 0
        int v15 = 0;
        v4 = (_WORD *)sub_69745B(v17, &v16, &v15);
        v21 = (char *)sub_66B2E8(a3, v25, *v4);
        if ( v21 != v25 )
        {
            auto v6 = this->get_name() + ": garbage string: " + a3;
            sp_log(v6.c_str());
            assert(0);
        }
#endif

    } else {
        CDECL_CALL(0x005D2DD0, this, a2, maxlen);
    }
}

void text_file::read(int *a2)
{
    CDECL_CALL(0x005D26C0, this, a2);
}

void text_file::read(float *a3)
{
    CDECL_CALL(0x005D2990, this, a3);
}

void text_file::read(mString *a1)
{
    CDECL_CALL(0x005D5970, this, a1);
}

void text_file::write(const mString &str) {
    this->io.write(str.c_str(), str.size());
}
