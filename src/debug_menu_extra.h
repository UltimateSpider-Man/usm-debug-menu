#pragma once

#include "variable.h"

struct debug_menu;
struct debug_menu_entry;

extern void create_gamefile_menu(debug_menu *parent);

extern void create_warp_menu(debug_menu* parent);

void sub_66A02D();

extern void create_debug_render_menu(debug_menu *parent);

extern void create_debug_district_variants_menu(debug_menu *parent);

extern void create_camera_menu_items(debug_menu *parent);

extern void game_flags_handler(debug_menu_entry *a1);

void create_dvar_menu(debug_menu* arg0);
