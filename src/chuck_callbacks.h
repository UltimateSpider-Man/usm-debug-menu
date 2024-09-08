#pragma once

#include "script_manager.h"

struct script_executable;

extern void script_manager_callback(script_manager_callback_reason a1, script_executable *a2, const char *a3);

//0x006607E0
extern void register_chuck_callbacks();


extern void chuck_callbacks_patch();
