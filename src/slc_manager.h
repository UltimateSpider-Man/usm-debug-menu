#pragma once

#include "mstring.h"
#include "variable.h"

#include <vector.hpp>

#include <set>

struct script_library_class;

struct slc_manager {
    //0x005AD720
    inline static void init();

    //0x005A5280
    inline static void add(script_library_class *slc);

    //0x005A5200
   inline static void kill();

    //0x0059EB10
  inline  static script_library_class *get_class(int class_index);

  inline  static script_library_class *get(const char *a1);

    //0x0059EC00
  inline  static void un_mash_all_funcs();
};

extern std::vector<script_library_class *> *& slc_manager_class_array;

//0x005AB800
extern void register_standard_script_libs();

extern void slc_manager_patch();
