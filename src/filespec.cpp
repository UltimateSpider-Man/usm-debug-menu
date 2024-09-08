#include "filespec.h"

#include "func_wrapper.h"
#include "utility.h"
#include "variables.h"

mString filespec::fullname() const {
    mString a2 = this->m_dir + this->m_name + this->m_ext;
    return a2;
}

mString filespec::name() const {
    mString res = this->m_name + this->m_ext;
    return res;
}

filespec::filespec(const mString &a2, const mString &a3, const mString &a4)
    : m_dir(a2), m_name(a3), m_ext(a4) {
}


