
#include "mstring.h"
#include "variable.h"



mString mString::from_char(const char* a2)
    {
        sub_6657FD(this, 0);
        if (a2 != nullptr) {
           update_guts(a2, -1);
        }
        return a2;
    }