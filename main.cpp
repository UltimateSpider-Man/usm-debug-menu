#ifdef _WIN32
#define _USE_MATH_DEFINES
#define NOMINMAX
#endif

#include <cmath>

#define _CRT_SECURE_NO_WARNINGS
#include <windows.h>
#include <stdio.h>
#include <vector>
#include <string>
#include <algorithm>

#define DIRECTINPUT_VERSION 0x0800
#include <dinput.h>
#include <stdint.h>
#pragma comment(lib, "Dinput8.lib")
#pragma comment(lib, "Dxguid.lib")



#include "src/rtdt_replay_mgr.h"
#include "src/limited_timer.h"

#include "src/base_ai_debug.h"
#include "src/debug_menu_extra.h"
#include "src/devopt.h"
#include "src/debug_menu.h"
#include "src/debug_struct.h"
#include "src/entity_animation_menu.h"
#include "src/game.h"
#include "src/input_mgr.h"
#include "src/resource_manager.h"
#include "src/forwards.h"
#include "src/func_wrapper.h"
#include "src/fixedstring32.h"
#include "src/input_mgr.h"
#include "src/levelmenu.h"
#include "src/memory_menu.h"
#include "src/message_board.h"
#include "src/mission_manager.h"
#include "src/mission_table_container.h"
#include "src/mouselook_controller.h"
#include "src/mstring.h"
#include "src/region.h"
#include "src/os_developer_options.h"
#include "src/script_executable.h"
#include "src/script_library_class.h"
#include "src/script_instance.h"
#include "src/script_object.h"
#include "src/spider_monkey.h"
#include "src/geometry_manager.h"
#include "src/entity.h"
#include "src/terrain.h"
#include "src/vm_executable.h"
#include "src/variable.h"
#include "src/wds.h"
#include "src/app.h"
#include "src/movie_manager.h"
#include "src/debugutil.h"


DWORD* ai_current_player = nullptr;
DWORD* fancy_player_ptr = nullptr;

/*
#undef IsEqualGUID
BOOL WINAPI IsEqualGUID(
	REFGUID rguid1,
	REFGUID rguid2)
{
	return !memcmp(rguid1, rguid2, sizeof(GUID));
}
*/



uint8_t color_ramp_function(float ratio, int period_duration, int cur_time) {

	if (cur_time <= 0 || 4 * period_duration <= cur_time)
		return 0;

	if (cur_time < period_duration) {

		float calc = ratio * cur_time;

		return std::min(calc, 1.0f) * 255;
	}


	if (period_duration <= cur_time && cur_time <= 3 * period_duration) {
		return 255;
	}


	if (cur_time <= 4 * period_duration) {

		int adjusted_time = cur_time - 3 * period_duration;
		float calc = 1.f - ratio * adjusted_time;

		return std::min(calc, 1.0f) * 255;
	}

    return 0;

}

static void *HookVTableFunction(void *pVTable, void *fnHookFunc, int nOffset) {
    intptr_t ptrVtable = *((intptr_t *) pVTable); // Pointer to our chosen vtable
    intptr_t ptrFunction = ptrVtable +
        sizeof(intptr_t) *
            nOffset; // The offset to the function (remember it's a zero indexed array with a size of four bytes)
    intptr_t ptrOriginal = *((intptr_t *) ptrFunction); // Save original address

    // Edit the memory protection so we can modify it
    MEMORY_BASIC_INFORMATION mbi;
    VirtualQuery((LPCVOID) ptrFunction, &mbi, sizeof(mbi));
    VirtualProtect(mbi.BaseAddress, mbi.RegionSize, PAGE_EXECUTE_READWRITE, &mbi.Protect);

    // Overwrite the old function with our new one
    *((intptr_t *) ptrFunction) = (intptr_t) fnHookFunc;

    // Restore the protection
    VirtualProtect(mbi.BaseAddress, mbi.RegionSize, mbi.Protect, &mbi.Protect);

    // Return the original function address incase we want to call it
    return (void *) ptrOriginal;
}

typedef struct _list{
	struct _list* next;
	struct _list* prev;
	void* data;
}list;

static constexpr DWORD MAX_ELEMENTS_PAGE = 18;

debug_menu* game_menu = nullptr;
debug_menu* devopt_menu = nullptr;
debug_menu* script_menu = nullptr;
debug_menu* progression_menu = nullptr;
debug_menu* level_select_menu = nullptr;




debug_menu** all_menus[] = {
    &debug_menu::root_menu,
    &game_menu,
    &devopt_menu,
    &script_menu,
	&progression_menu,
    &level_select_menu
};

void unlock_region(region* cur_region) {
	cur_region->flags &= 0xFE;
}

void remove_debug_menu_entry(debug_menu_entry* entry) {
	
	DWORD to_be = (DWORD)entry;
	for (auto i = 0u; i < (sizeof(all_menus) / sizeof(void*)); ++i) {

		debug_menu *cur = *all_menus[i];

		DWORD start = (DWORD) cur->entries;
		DWORD end = start + cur->used_slots * sizeof(debug_menu_entry);

		if (start <= to_be && to_be < end) {

			int index = (to_be - start) / sizeof(debug_menu_entry);

			memcpy(&cur->entries[index], &cur->entries[index + 1], cur->used_slots - (index + 1));
			memset(&cur->entries[cur->used_slots - 1], 0, sizeof(debug_menu_entry));
			cur->used_slots--;
			return;
		}
		
	}

	printf("FAILED TO DEALLOCATE AN ENTRY :S %08X\n", entry);

}

int vm_debug_menu_entry_garbage_collection_id = -1;

typedef int (*script_manager_register_allocated_stuff_callback_ptr)(void* func);
script_manager_register_allocated_stuff_callback_ptr script_manager_register_allocated_stuff_callback = (script_manager_register_allocated_stuff_callback_ptr) 0x005AFE40;

typedef int (*construct_client_script_libs_ptr)();
construct_client_script_libs_ptr construct_client_script_libs = (construct_client_script_libs_ptr) 0x0058F9C0;

void vm_debug_menu_entry_garbage_collection_callback(void* a1, list* lst) {

	list* end = lst->prev;

	for (list* cur = end->next; cur != end; cur = cur->next) {

		debug_menu_entry* entry = ((debug_menu_entry*)cur->data);
		//printf("Will delete %s %08X\n", entry->text, entry);
		remove_debug_menu_entry(entry);
	}
}

int construct_client_script_libs_hook() {

	if (vm_debug_menu_entry_garbage_collection_id == -1) {
		int res = script_manager_register_allocated_stuff_callback((void *) vm_debug_menu_entry_garbage_collection_callback);
		vm_debug_menu_entry_garbage_collection_id = res;
	}

	return construct_client_script_libs();
}

region** all_regions = (region **) 0x0095C924;
DWORD* number_of_allocated_regions = (DWORD *) 0x0095C920;

typedef const char* (__fastcall* region_get_name_ptr)(void* );
region_get_name_ptr region_get_name = (region_get_name_ptr) 0x00519BB0;

typedef int (__fastcall *region_get_district_variant_ptr)(region* );
region_get_district_variant_ptr region_get_district_variant = (region_get_district_variant_ptr) 0x005503D0;

void set_text_writeable() {

	const DWORD text_end = 0x86F000;
	const DWORD text_start = 0x401000;

	DWORD old;
	VirtualProtect((void*)text_start, text_end - text_start, PAGE_EXECUTE_READWRITE, &old);
}

void set_rdata_writeable() {

	const DWORD end = 0x91B000;
	const DWORD start = 0x86F564;

	DWORD old;
	VirtualProtect((void*)start, end - start, PAGE_READWRITE, &old);
}

void HookFunc(DWORD callAdd, DWORD funcAdd, BOOLEAN jump, const char* reason) {

	//Only works for E8/E9 hooks	
	DWORD jmpOff = funcAdd - callAdd - 5;

	BYTE shellcode[] = { 0, 0, 0, 0, 0 };
	shellcode[0] = jump ? 0xE9 : 0xE8;

	memcpy(&shellcode[1], &jmpOff, sizeof(jmpOff));
	memcpy((void*)callAdd, shellcode, sizeof(shellcode));

	if (reason)
		printf("Hook: %08X -  %s\n", callAdd, reason);

}


void WriteDWORD(int address, DWORD newValue, const char* reason) {
	* ((DWORD *)address) = newValue;
	if (reason)
		printf("Wrote: %08X -  %s\n", address, reason);
}

typedef int (*nflSystemOpenFile_ptr)(HANDLE* hHandle, LPCSTR lpFileName, unsigned int a3, LARGE_INTEGER liDistanceToMove);
nflSystemOpenFile_ptr nflSystemOpenFile_orig = nullptr;

nflSystemOpenFile_ptr* nflSystemOpenFile_data = (nflSystemOpenFile_ptr *) 0x0094985C;

HANDLE USM_handle = INVALID_HANDLE_VALUE;

int nflSystemOpenFile(HANDLE* hHandle, LPCSTR lpFileName, unsigned int a3, LARGE_INTEGER liDistanceToMove) {

	//printf("Opening file %s\n", lpFileName);
	int ret = nflSystemOpenFile_orig(hHandle, lpFileName, a3, liDistanceToMove);


	if (strstr(lpFileName, "ultimate_spiderman.PCPACK")) {

	}
	return ret;
}

typedef int (*ReadOrWrite_ptr)(int a1, HANDLE* a2, int a3, DWORD a4, LPCVOID lpBuffer, DWORD nNumberOfBytesToWrite);
ReadOrWrite_ptr* ReadOrWrite_data = (ReadOrWrite_ptr *)0x0094986C;
ReadOrWrite_ptr ReadOrWrite_orig = nullptr;

int ReadOrWrite(int a1, HANDLE* a2, int a3, DWORD a4, LPCVOID lpBuffer, DWORD nNumberOfBytesToWrite) {

	int ret = ReadOrWrite_orig(a1, a2, a3, a4, lpBuffer, nNumberOfBytesToWrite);

	if (USM_handle == *a2) {
		printf("USM buffer was read %08X\n", (DWORD)lpBuffer);


	}
	return ret;
}

typedef void (*aeps_RenderAll_ptr)();
aeps_RenderAll_ptr aeps_RenderAll_orig = (aeps_RenderAll_ptr)0x004D9310;


unsigned int nglColor(int r, int g, int b, int a)
{
    return ( (a << 24) |  (r << 16) | (g << 8) | (b & 255) );
}

typedef void (*nglListEndScene_ptr)();
nglListEndScene_ptr nglListEndScene = (nglListEndScene_ptr) 0x00742B50;

typedef void (*nglSetClearFlags_ptr)(int);
nglSetClearFlags_ptr nglSetClearFlags2 = (nglSetClearFlags_ptr) 0x00769DB0;

void aeps_RenderAll() {
	static int cur_time = 0;
	int period = 60;
	int duration = 6 * period;
	float ratio = 1.f / period;

	uint8_t red = color_ramp_function(ratio, period, cur_time + 2 * period) + color_ramp_function(ratio, period, cur_time - 4 * period);
	uint8_t green = color_ramp_function(ratio, period, cur_time);
	uint8_t blue = color_ramp_function(ratio, period, cur_time - 2 * period);

	nglListAddString(*nglSysFont, 0.1f, 0.2f, 0.2f, nglColor(red, green, blue, 255), 1.f, 1.f, "");

	cur_time = (cur_time + 1) % duration;


	aeps_RenderAll_orig();
}
int debug_enabled = 0;
int debug_disabled = 0;

uint32_t keys[256];

void getStringDimensions(const char* str, int* width, int* height) {
	nglGetStringDimensions(*nglSysFont, str, width, height, 1.0, 1.0);
}

int getStringHeight(const char* str) {
	int height;
	nglGetStringDimensions(nglSysFont(), str, nullptr, &height, 1.0, 1.0);
	return height;
}

std::string getRealText(debug_menu_entry *entry) {
    assert(entry->render_callback != nullptr);

    auto v1 = entry->render_callback(entry);

    char a2a[256]{};
    if (v1.size() != 0) {
        auto *v7 = v1.c_str();
        auto *v4 = entry->text;
        snprintf(a2a, 255u, "%s: %s", v4, v7);
    } else {
        auto *v5 = entry->text;
        snprintf(a2a, 255u, "%s", v5);
    }

    return {a2a};
}

void render_current_debug_menu() {
    auto UP_ARROW {" ^ ^ ^ "};
    auto DOWN_ARROW {" v v v "};

	int num_elements = std::min((DWORD) MAX_ELEMENTS_PAGE, current_menu->used_slots - current_menu->window_start);
	int needs_down_arrow = ((current_menu->window_start + MAX_ELEMENTS_PAGE) < current_menu->used_slots) ? 1 : 0;

	int cur_width, cur_height;
	int debug_width = 0;
	int debug_height = 0;

    auto get_and_update = [&](auto *x) {\
        getStringDimensions(x, &cur_width, &cur_height); \
        debug_height += cur_height; \
        debug_width = std::max(debug_width, cur_width);\
	};

	//printf("new size: %s %d %d (%d %d)\n", x, debug_width, debug_height, cur_width, cur_height);

	get_and_update(current_menu->title);
	get_and_update(UP_ARROW);

	int total_elements_page = needs_down_arrow ? MAX_ELEMENTS_PAGE : current_menu->used_slots - current_menu->window_start;

	for (int i = 0; i < total_elements_page; ++i) {
		debug_menu_entry *entry = &current_menu->entries[current_menu->window_start + i];
		auto cur = getRealText(entry);
		get_and_update(cur.c_str());
	}

	if (needs_down_arrow) {
		get_and_update(DOWN_ARROW);
	}

	nglQuad quad;

	int menu_x_start = 20, menu_y_start = 40;
	int menu_x_pad = 24, menu_y_pad = 18;

	nglInitQuad(&quad);
	nglSetQuadRect(&quad, menu_x_start, menu_y_start, menu_x_start + debug_width + menu_x_pad, menu_y_start + debug_height + menu_y_pad);
        nglSetQuadColor(&quad, 0xC8141414);
	nglSetQuadZ(&quad, 0.5f);
	nglListAddQuad(&quad);

	int white_color = nglColor(255, 255, 255, 255);
	int yellow_color = nglColor(255, 255, 0, 255);
	int green_color = nglColor(0, 255, 0, 255);
	int pink_color = nglColor(255, 0, 255, 255);

	int render_height = menu_y_start;
	render_height += 12;
	int render_x = menu_x_start;
	render_x += 8;
	nglListAddString(nglSysFont(), render_x, render_height, 0.2f, green_color, 1.f, 1.f, current_menu->title);
	render_height += getStringHeight(current_menu->title);

	if (current_menu->window_start) {
		nglListAddString(*nglSysFont, render_x, render_height, 0.2f, pink_color, 1.f, 1.f, UP_ARROW);
	}

	render_height += getStringHeight(UP_ARROW);

	for (int i = 0; i < total_elements_page; i++) {

		int current_color = current_menu->cur_index == i ? yellow_color : white_color;

		debug_menu_entry* entry = &current_menu->entries[current_menu->window_start + i];
		auto cur = getRealText(entry);
		nglListAddString(*nglSysFont, render_x, render_height, 0.2f, current_color, 1.f, 1.f, cur.c_str());
		render_height += getStringHeight(cur.c_str());
	}

	if (needs_down_arrow) {
		nglListAddString(*nglSysFont, render_x, render_height, 0.2f, pink_color, 1.f, 1.f, DOWN_ARROW);
		render_height += getStringHeight(DOWN_ARROW);
	}
}

void render_current_debug_menu2()
{
    auto UP_ARROW { " ^ ^ ^ " };
    auto DOWN_ARROW { " v v v " };

    int num_elements = std::min((DWORD)MAX_ELEMENTS_PAGE, current_menu->used_slots - current_menu->window_start);
    int needs_down_arrow = ((current_menu->window_start + MAX_ELEMENTS_PAGE) < current_menu->used_slots) ? 1 : 0;

    int cur_width, cur_height;
    int debug_width = 0;
    int debug_height = 0;

    auto get_and_update = [&](auto* x) {
        getStringDimensions(x, &cur_width, &cur_height);
        debug_height += cur_height;
        debug_width = std::max(debug_width, cur_width);
    };

    // printf("new size: %s %d %d (%d %d)\n", x, debug_width, debug_height, cur_width, cur_height);

    get_and_update(current_menu->title);
    get_and_update(UP_ARROW);

    int total_elements_page = needs_down_arrow ? MAX_ELEMENTS_PAGE : current_menu->used_slots - current_menu->window_start;

    for (int i = 0; i < total_elements_page; ++i) {
        debug_menu_entry* entry = &current_menu->entries[current_menu->window_start + i];
        auto cur = getRealText(entry);
        get_and_update(cur.c_str());
    }

    if (needs_down_arrow) {
        get_and_update(DOWN_ARROW);
    }

    nglQuad quad;

    int menu_x_start = 20, menu_y_start = 40;
    int menu_x_pad = 24, menu_y_pad = 18;

    nglInitQuad(&quad);
    nglSetQuadRect(&quad, menu_x_start, menu_y_start, menu_x_start + debug_width + menu_x_pad, menu_y_start + debug_height + menu_y_pad);
    nglSetQuadColor(&quad, 0x68141414);
    nglSetQuadZ(&quad, 0.5f);
    nglListAddQuad(&quad);

    int white_color = nglColor(255, 255, 255, 255);
    int yellow_color = nglColor(255, 255, 0, 255);
    int green_color = nglColor(0, 255, 0, 255);
    int pink_color = nglColor(255, 0, 255, 255);

    int render_height = menu_y_start;
    render_height += 12;
    int render_x = menu_x_start;
    render_x += 8;
    nglListAddString(nglSysFont(), render_x, render_height, 0.2f, green_color, 1.f, 1.f, current_menu->title);
    render_height += getStringHeight(current_menu->title);

    if (current_menu->window_start) {
        nglListAddString(*nglSysFont, render_x, render_height, 0.2f, pink_color, 1.f, 1.f, UP_ARROW);
    }

    render_height += getStringHeight(UP_ARROW);

    for (int i = 0; i < total_elements_page; i++) {

        int current_color = current_menu->cur_index == i ? yellow_color : white_color;

        debug_menu_entry* entry = &current_menu->entries[current_menu->window_start + i];
        auto cur = getRealText(entry);
        nglListAddString(*nglSysFont, render_x, render_height, 0.2f, current_color, 1.f, 1.f, cur.c_str());
        render_height += getStringHeight(cur.c_str());
    }

    if (needs_down_arrow) {
        nglListAddString(*nglSysFont, render_x, render_height, 0.2f, pink_color, 1.f, 1.f, DOWN_ARROW);
        render_height += getStringHeight(DOWN_ARROW);
    }
}

void myDebugMenu() {
	if (debug_enabled) {
		render_current_debug_menu();
	}
        if (debug_disabled) {
            render_current_debug_menu2();
        }

	nglListEndScene();
}

typedef int (*wndHandler_ptr)(HWND, UINT, WPARAM, LPARAM);
wndHandler_ptr orig_WindowHandler = (wndHandler_ptr) 0x005941A0;

int WindowHandler(HWND hwnd, UINT Msg, WPARAM wParam, LPARAM lParam) {

	switch (Msg) {
	case WM_SYSKEYDOWN:
	case WM_SYSKEYUP:
	case WM_KEYUP:
	case WM_KEYDOWN:
	case WM_INPUT:
		printf("swallowed keypress\n");
		return 0;
	}

	return orig_WindowHandler(hwnd, Msg, wParam, lParam);

}

/*
	STDMETHOD(GetDeviceState)(THIS_ DWORD,LPVOID) PURE;
	STDMETHOD(GetDeviceData)(THIS_ DWORD,LPDIDEVICEOBJECTDATA,LPDWORD,DWORD) PURE;

*/

typedef int (__stdcall* GetDeviceState_ptr)(IDirectInputDevice8*, DWORD, LPVOID);
GetDeviceState_ptr GetDeviceStateOriginal = nullptr;

typedef int (__fastcall* game_pause_unpause_ptr)(void* );
game_pause_unpause_ptr game_pause = (game_pause_unpause_ptr) 0x0054FBE0;
game_pause_unpause_ptr game_unpause = (game_pause_unpause_ptr) 0x0053A880;

typedef int (__fastcall* game_get_cur_state_ptr)(void* );
game_get_cur_state_ptr game_get_cur_state = (game_get_cur_state_ptr) 0x005363D0;

typedef int (__fastcall* world_dynamics_system_remove_player_ptr)(void* , void* edx, int number);
world_dynamics_system_remove_player_ptr world_dynamics_system_remove_player = (world_dynamics_system_remove_player_ptr) 0x00558550;

typedef int (__fastcall* world_dynamics_system_add_player_ptr)(void* , void* edx, mString* str);
world_dynamics_system_add_player_ptr world_dynamics_system_add_player = (world_dynamics_system_add_player_ptr) 0x0055B400;

typedef int (*entity_teleport_abs_po_ptr)(DWORD, float*, int one);
entity_teleport_abs_po_ptr entity_teleport_abs_po = (entity_teleport_abs_po_ptr) 0x004F3890;

typedef DWORD* (__fastcall* ai_ai_core_get_info_node_ptr)(DWORD* , void* edx, int a2, char a3);
ai_ai_core_get_info_node_ptr ai_ai_core_get_info_node = (ai_ai_core_get_info_node_ptr) 0x006A3390;


struct vm_thread {
	uint8_t unk[0xC];
	script_instance *inst;
	vm_executable *ex;
};

struct vm_stack {
	uint8_t unk[0x184];
	DWORD stack_ptr;
	vm_thread* thread;

    void push(void* data, DWORD size) {
        memcpy((void *)this->stack_ptr, data, size);
        this->stack_ptr += size;
    }

    void pop(DWORD size) {
        this->stack_ptr -= size;
    }

};


uint8_t __stdcall slf__debug_menu_entry__set_handler__str(vm_stack *stack, void* unk) {

	stack->pop(8);

	void** params = (void**)stack->stack_ptr;

	debug_menu_entry *entry = static_cast<decltype(entry)>(params[0]);
	const char *scrpttext = static_cast<char *>(params[1]);

	string_hash strhash {scrpttext};

	script_instance *instance = stack->thread->inst;
    entry->set_script_handler(instance, mString {scrpttext});
	
	return true;
}

uint8_t __stdcall slf__destroy_debug_menu_entry__debug_menu_entry(vm_stack* function, void* unk) {

	function->pop(4);

	debug_menu_entry** entry = (decltype(entry)) function->stack_ptr;

	remove_debug_menu_entry(*entry);

	return 1;
}

void sub_65BB36(script_library_class::function *func, vm_stack *stack, char *a3, int a4)
{
    for ( auto i = 0; i < a4; ++i )
    {
        if ( *bit_cast<DWORD *>(&a3[4 * i]) == 0x7BAD05CF )
        {
            printf("uninitialized parameters in call to 0x%08X", func->m_vtbl);

            //v5 = j_vm_stack::get_thread(stack);
            //vm_thread::slf_error(v5, v6);

            assert(0 && "uninitialized parameters in call to script library function");
        }
    }
}

uint8_t __fastcall slf__create_progression_menu_entry(script_library_class::function *func, void *, vm_stack *stack, void *unk) {

	stack->pop(8);

    auto *stack_ptr = bit_cast<char *>(stack->stack_ptr);
    sub_65BB36(func, stack, stack_ptr, 2);

	char** strs = (char **)stack->stack_ptr;

	printf("Entry: %s -> %s\n", strs[0], strs[1]);

	string_hash strhash {strs[1]};

	script_instance *instance = stack->thread->inst;

	debug_menu_entry entry {strs[0]};
    entry.set_script_handler(instance, {strs[1]});

	progression_menu->add_entry(&entry);
	
	int push = 0;
	stack->push(&push, sizeof(push));
	return true;
}

bool __fastcall slf__create_debug_menu_entry(script_library_class::function *func, void *, vm_stack* stack, void* unk)
{
	stack->pop(4);

    auto *stack_ptr = bit_cast<char *>(stack->stack_ptr);
    sub_65BB36(func, stack, stack_ptr, 1);
	char** strs = bit_cast<char **>(stack->stack_ptr);

	//printf("Entry: %s ", strs[0]);

	debug_menu_entry entry {};
	entry.entry_type = UNDEFINED;
	strcpy(entry.text, strs[0]);

    printf("entry.text = %s\n", entry.text);

	script_instance *instance = stack->thread->inst;
    printf("Total funcs: %d\n", instance->m_parent->total_funcs);

	void *res = add_debug_menu_entry(script_menu, &entry);

	script_executable *se = stack->thread->ex->owner->parent;
    printf("total_script_objects = %d\n", se->total_script_objects);
    for (auto i = 0; i < se->total_script_objects; ++i) {
        auto *so = se->field_28[i];
        printf("Name of script_object = %s\n", so->name.to_string());

        for (auto i = 0; i < so->total_funcs; ++i) {
            printf("Func name: %s\n", so->funcs[i]->name.to_string());
        }

        printf("\n");
    }

	se->add_allocated_stuff(vm_debug_menu_entry_garbage_collection_id, (int) res, 0);

	//printf("%08X\n", res);

	int push = (int) res;
	stack->push(&push, sizeof(push));
	return 1;
}

DWORD modulo(int num, DWORD mod) {
	if (num >= 0) {
		return num % mod;
	}

	int absolute = abs(num);
	if (absolute % mod == 0)
		return 0;

	return mod - absolute % mod;
}


void menu_go_down() {


	if ((current_menu->window_start + MAX_ELEMENTS_PAGE) < current_menu->used_slots) {

		if (current_menu->cur_index < MAX_ELEMENTS_PAGE / 2)
			++current_menu->cur_index;
		else
			++current_menu->window_start;
	}
	else {

		int num_elements = std::min((DWORD) MAX_ELEMENTS_PAGE, current_menu->used_slots - current_menu->window_start);
		current_menu->cur_index = modulo(current_menu->cur_index + 1, num_elements);
		if (current_menu->cur_index == 0)
			current_menu->window_start = 0;
	}
}

void menu_go_up() {

	int num_elements = std::min( (DWORD) MAX_ELEMENTS_PAGE, current_menu->used_slots - current_menu->window_start);
	if (current_menu->window_start) {


		if (current_menu->cur_index > MAX_ELEMENTS_PAGE / 2)
			current_menu->cur_index--;
		else
			current_menu->window_start--;

	}
	else {

		int num_elements = std::min(MAX_ELEMENTS_PAGE, current_menu->used_slots - current_menu->window_start);
		current_menu->cur_index = modulo(current_menu->cur_index - 1, num_elements);
		if (current_menu->cur_index == (num_elements - 1))
			current_menu->window_start = current_menu->used_slots - num_elements;

	}

}

typedef enum {
	MENU_R3,
	MENU_ACCEPT,
	MENU_BACK,

	MENU_UP,
	MENU_DOWN,
	MENU_LEFT,
	MENU_RIGHT,


	MENU_KEY_MAX
} MenuKey;

uint32_t controllerKeys[MENU_KEY_MAX];

int get_menu_key_value(MenuKey key, int keyboard) {
	if (keyboard) {

		int i = 0;
		switch (key) {
			case MENU_R3:
				i = DIK_X;
				break;
			case MENU_ACCEPT:
				i = DIK_RETURN;
				break;
			case MENU_BACK:
				i = DIK_ESCAPE;
				break;

			case MENU_UP:
				i = DIK_UPARROW;
				break;
			case MENU_DOWN:
				i = DIK_DOWNARROW;
				break;
			case MENU_LEFT:
				i = DIK_LEFTARROW;
				break;
			case MENU_RIGHT:
				i = DIK_RIGHTARROW;
				break;
		}
		return keys[i];
	}



	return controllerKeys[key];
}


int is_menu_key_pressed(MenuKey key, int keyboard) {
	return (get_menu_key_value(key, keyboard) == 2);
}

int is_menu_key_clicked(MenuKey key, int keyboard) {
	return get_menu_key_value(key, keyboard);
}

void GetDeviceStateHandleKeyboardInput(LPVOID lpvData) {
	BYTE* keysCurrent = (BYTE *) lpvData;

	for (int i = 0; i < 256; i++) {

		if (keysCurrent[i]) {
			keys[i]++;
		}
		else {
			keys[i] = 0;
		}
	}

	
}

void read_and_update_controller_key_button(LPDIJOYSTATE2 joy, int index, MenuKey key) {
	int res = 0;
	if (joy->rgbButtons[index]) {
		++controllerKeys[key];
	}
	else {
		controllerKeys[key] = 0;
	}
}


void read_and_update_controller_key_dpad(LPDIJOYSTATE2 joy, int angle, MenuKey key) {
	
	if (joy->rgdwPOV[0] == 0xFFFFFFFF)
		controllerKeys[key] = 0;
	else
		controllerKeys[key] = (joy->rgdwPOV[0] == angle) ? controllerKeys[key] + 1 : 0;
}


void GetDeviceStateHandleControllerInput(LPVOID lpvData) {
	LPDIJOYSTATE2 joy = (decltype(joy)) lpvData;

	read_and_update_controller_key_button(joy, 1, MENU_ACCEPT);
	read_and_update_controller_key_button(joy, 2, MENU_BACK);
	read_and_update_controller_key_button(joy, 11, MENU_R3);

	read_and_update_controller_key_dpad(joy, 0, MENU_UP);
	read_and_update_controller_key_dpad(joy, 9000, MENU_RIGHT);
	read_and_update_controller_key_dpad(joy, 18000, MENU_DOWN);
	read_and_update_controller_key_dpad(joy, 27000, MENU_LEFT);
}

typedef int (*resource_manager_can_reload_amalgapak_ptr)(void);
resource_manager_can_reload_amalgapak_ptr resource_manager_can_reload_amalgapak = (resource_manager_can_reload_amalgapak_ptr) 0x0053DE90;

typedef void (*resource_manager_reload_amalgapak_ptr)(void);
resource_manager_reload_amalgapak_ptr resource_manager_reload_amalgapak = (resource_manager_reload_amalgapak_ptr) 0x0054C2E0;

struct mission_t
{
    std::string field_0;
    const char *field_C;
    int field_10;
    int field_14;
};

std::vector<mission_t> menu_missions; 

void mission_unload_handler(debug_menu_entry *a1)
{
    auto *v1 = mission_manager::s_inst();
    v1->prepare_unload_script();

	close_debug();
}


void create_game_flags_menu(debug_menu *parent);

void empty_handler(debug_menu_entry* a1)
{
}

void mission_select_handler(debug_menu_entry* entry)
{
    auto v1 = (int)entry->m_id;
    auto v7 = &menu_missions.at(v1);
    auto v6 = v7->field_C;
    auto v5 = v7->field_14;
    auto* v4 = v7->field_0.c_str();
    auto v3 = v7->field_10;
    auto* v2 = mission_manager::s_inst();
    v2->force_mission(v3, v4, v5, v6);
    close_debug();
}

void create_game_flags_menu(debug_menu* parent);

void _populate_missions()
{
    auto handle_table = [](mission_table_container* table, int district_id) -> void {
        std::vector<mission_table_container::script_info> script_infos {};

        if (table != nullptr) {
            table->append_script_info(&script_infos);
        }

        for (auto& info : script_infos) {
            mString a2 { "pk_" };
            auto v19 = a2 + info.field_0;
            auto* v11 = v19.c_str();
            auto key = create_resource_key_from_path(v11, 25);
            if (resource_manager::get_pack_file_stats(key, nullptr, nullptr, nullptr)) {
                mission_t mission {};
                mission.field_0 = info.field_0;
                mission.field_10 = district_id;
                mission.field_14 = info.field_8;

                mission.field_C = info.field_4->get_script_data_name();
                menu_missions.push_back(mission);

                auto v47 = [](mission_t& mission) -> mString {
                    if (mission.field_C != nullptr) {
                        auto* v17 = mission.field_C;
                        auto* v14 = mission.field_0.c_str();
                        mString str { 0, "%s (%s)", v14, v17 };
                        return str;
                    }

                    auto v18 = mission.field_14;
                    auto* v15 = mission.field_0.c_str();
                    mString str { 0, "%s (%d)", v15, v18 };
                    return str;
                }(mission);

                printf(v47.c_str());
            }
        }
    };

    auto* v2 = mission_manager::s_inst();
    auto count = v2->get_district_table_count();
    printf("%s %d", "table_count = ", count);

    {
        auto* v3 = mission_manager::s_inst();
        auto* table = v3->get_global_table();

        handle_table(table, 0);
    }

    std::for_each(&v2->m_district_table_containers[0], &v2->m_district_table_containers[0] + count, [&handle_table](auto* table) {
        auto* reg = table->get_region();
        auto& v6 = reg->get_name();
        fixedstring32 v53 { v6.to_string() };

        auto district_id = reg->get_district_id();

        // sp_log("%d %s", i, v53.to_string());

        handle_table(table, district_id);
    });

    assert(0);
}




void populate_missions_menu(debug_menu_entry* entry)
{

    auto* head_menu = create_menu("Missions");
    entry->set_submenu(head_menu);

    if (head_menu->used_slots == 0) {
        menu_missions = {};
        if (resource_manager_can_reload_amalgapak()) {
            resource_manager::reload_amalgapak();
        }

        auto* mission_unload_entry = create_menu_entry(mString { "UNLOAD CURRENT MISSION" });

        mission_unload_entry->set_game_flags_handler(mission_unload_handler);
        head_menu->add_entry(mission_unload_entry);

        auto* v2 = mission_manager::s_inst();
        auto v58 = v2->get_district_table_count();
        for (auto i = -1; i < v58; ++i) {
            fixedstring32 v53 {};
            int v52;
            mission_table_container* table = nullptr;
            if (i == -1) {
                table = v2->get_global_table();
                fixedstring32 a1 { "global" };
                v53 = a1;
                v52 = 0;
            } else {
                table = v2->get_district_table(i);
                auto* reg = table->get_region();
                auto& v6 = reg->get_name();
                v53 = v6;

                auto v52 = reg->get_district_id();

                auto* v25 = create_menu(v53.to_string(), debug_menu::sort_mode_t::ascending);

                debug_menu_entry v26 { v25 };
                add_debug_menu_entry(head_menu, &v26);

                auto* v28 = create_menu("", (menu_handler_function)empty_handler, 10);
                debug_menu_entry v30 { v28 };
                v30.entry_type = UNDEFINED;
                debug_menu_entry v29 { v30 };
                add_debug_menu_entry(v25, &v29);
            }

            std::vector<mission_table_container::script_info> script_infos;

            if (table != nullptr) {
                auto res = table->append_script_info(&script_infos);
                assert(res);
            }

            for (auto& info : script_infos) {

                auto v50 = menu_missions.size();
                mString a2 { "pk_" };
                auto v19 = a2 + info.field_0;
                auto* v11 = v19.c_str();
                auto key = create_resource_key_from_path(v11, 25);
                if (resource_manager::get_pack_file_stats(key, nullptr, nullptr, nullptr)) {
                    mission_t mission {};
                    mission.field_0 = info.field_0;
                    mission.field_10 = v52;
                    mission.field_14 = info.field_8;

                    mission.field_C = info.field_4->get_script_data_name();
                    menu_missions.push_back(mission);

                    mString v47 {};
                    if (mission.field_C != nullptr) {
                        auto* v17 = mission.field_C;
                        auto* v14 = mission.field_0.c_str();
                        auto a2a = mString { 0, "%s (%s)", v14, v17 };
                        v47 = a2a;
                    } else {
                        auto v18 = mission.field_14;
                        auto* v15 = mission.field_0.c_str();
                        auto a2b = mString { 0, "%s (%d)", v15, v18 };
                        v47 = a2b;
                    }
                    auto* v27 = create_menu_entry(v47);

                    auto* v43 = v27;
                    auto* v46 = v27;
                    v27->set_id(v50);
                    v46->set_game_flags_handler(mission_select_handler);
                    head_menu->add_entry(v46);

                    auto handle_table = [](mission_table_container* table, int district_id) -> void {
                        std::vector<mission_table_container::script_info> script_infos {};

                        if (table != nullptr) {
                            table->append_script_info(&script_infos);
                        }

                        for (auto& info : script_infos) {
                            mString a2 { "pk_" };
                            auto v19 = a2 + info.field_0;
                            auto* v11 = v19.c_str();
                            auto key = create_resource_key_from_path(v11, 25);
                            if (resource_manager::get_pack_file_stats(key, nullptr, nullptr, nullptr)) {
                                mission_t mission {};
                                mission.field_0 = info.field_0;
                                mission.field_10 = district_id;
                                mission.field_14 = info.field_8;

                                mission.field_C = info.field_4->get_script_data_name();
                                menu_missions.push_back(mission);

                                auto v47 = [](mission_t& mission) -> mString {
                                    if (mission.field_C != nullptr) {
                                        auto* v17 = mission.field_C;
                                        auto* v14 = mission.field_0.c_str();
                                        mString str { 0, "%s (%s)", v14, v17 };
                                        return str;
                                    }

                                    auto v18 = mission.field_14;
                                    auto* v15 = mission.field_0.c_str();
                                    mString str { 0, "%s (%d)", v15, v18 };
                                    return str;
                                }(mission);

                                printf(v47.c_str());
                            }
                        }
                    };

                    auto* v2 = mission_manager::s_inst();
                    auto count = v2->get_district_table_count();
                    printf("%s %d", "table_count = ", count);

                    {
                        auto* v3 = mission_manager::s_inst();
                        auto* table = v3->get_global_table();

                        handle_table(table, 0);
                    }

                    std::for_each(&v2->m_district_table_containers[0], &v2->m_district_table_containers[0] + count, [&handle_table](auto* table) {
                        auto* reg = table->get_region();
                        auto& v6 = reg->get_name();
                        fixedstring32 v53 { v6.to_string() };

                        auto district_id = reg->get_district_id();

                        // sp_log("%d %s", i, v53.to_string());

                        handle_table(table, district_id);
                    });

                    assert(0);
                }
            }
        }
    }
}

void create_missions_menu(debug_menu* parent)
{
    auto* missions_menu = create_menu("Missions");
    auto* v2 = create_menu_entry(missions_menu);
    v2->set_game_flags_handler(populate_missions_menu);
    parent->add_entry(v2);
}

void disable_physics()
{
    debug_enabled = 1;
    game_unpause(g_game_ptr());
    current_menu = current_menu;
    g_game_ptr()->enable_physics(false);
}

void enable_physics()
{
    debug_disabled = 1;
    game_unpause(g_game_ptr());
    current_menu = current_menu;
    g_game_ptr()->enable_physics(true);
}

void custom()
{
    debug_disabled = 1;
    game_unpause(g_game_ptr());
    current_menu = current_menu;
    !os_developer_options::instance()->get_flag("ENABLE_ZOOM_MAP");
    spider_monkey::is_running();
    g_game_ptr()->enable_physics(false);
}


void menu_setup(int game_state, int keyboard) {

	//debug menu stuff
	if (is_menu_key_pressed(MENU_R3, keyboard) && (game_state == 6 || game_state == 7)) {

        if (!debug_enabled && game_state == 6) {
                game_unpause(g_game_ptr());
                debug_enabled = !debug_enabled;
                current_menu = debug_menu::root_menu;
                custom();
            }

            else if (!debug_disabled && game_state == 6) {
                game_unpause(g_game_ptr());
                debug_disabled = !debug_disabled;
                current_menu = current_menu;
                disable_physics();

            }

            else if (!debug_enabled && game_state == 6) {
                game_unpause(g_game_ptr());
                debug_enabled = !debug_enabled;
                current_menu = current_menu;
                disable_physics();

            }

            else if (!debug_enabled, debug_disabled && game_state == 6) {
                game_unpause(g_game_ptr());
                debug_disabled, debug_enabled = !debug_disabled, debug_enabled;
                current_menu = current_menu;
                enable_physics();
            }
        }

        if (is_menu_key_pressed(MENU_R3, keyboard) && (game_state == 6 || game_state == 7)) {

            if (!debug_enabled && game_state == 7) {
                game_unpause(g_game_ptr());
                debug_enabled = !debug_enabled;
                current_menu = debug_menu::root_menu;
                disable_physics();

            }

            else if (!debug_disabled && game_state == 7) {
                game_unpause(g_game_ptr());
                debug_disabled = !debug_disabled;
                current_menu = current_menu;
                disable_physics();

            }

            else if (!debug_enabled && game_state == 7) {
                game_unpause(g_game_ptr());
                debug_enabled = !debug_enabled;
                current_menu = current_menu;
                disable_physics();

            }

            else if (!debug_enabled, debug_disabled && game_state == 7) {
                game_unpause(g_game_ptr());
                debug_disabled, debug_enabled = !debug_disabled, debug_enabled;
                current_menu = current_menu;
                enable_physics();
            }

            if (!debug_enabled && game_state == 7) {
                game_unpause(g_game_ptr());
                debug_enabled = !debug_enabled;
                current_menu = debug_menu::root_menu;
                disable_physics();

            }

            else if (!debug_disabled && game_state == 7) {
                game_unpause(g_game_ptr());
                debug_disabled = !debug_disabled;
                current_menu = current_menu;
                disable_physics();

            }

            else if (!debug_enabled && game_state == 7) {
                game_unpause(g_game_ptr());
                debug_enabled = !debug_enabled;
                current_menu = current_menu;
                disable_physics();

            }

            else if (!debug_enabled, debug_disabled && game_state == 7) {
                game_unpause(g_game_ptr());
                debug_disabled, debug_enabled = !debug_disabled, debug_enabled;
                current_menu = current_menu;
                enable_physics();
            }




        if (level_select_menu->used_slots == 0)
        {
            create_level_select_menu(level_select_menu);
        }
	}
}

void menu_input_handler(int keyboard, int SCROLL_SPEED) {
	if (is_menu_key_clicked(MENU_DOWN, keyboard)) {

		int key_val = get_menu_key_value(MENU_DOWN, keyboard);
		if (key_val == 1) {
			menu_go_down();
		}
		else if ((key_val >= SCROLL_SPEED) && (key_val % SCROLL_SPEED == 0)) {
			menu_go_down();
		}
	}
	else if (is_menu_key_clicked(MENU_UP, keyboard)) {

		int key_val = get_menu_key_value(MENU_UP, keyboard);
		if (key_val == 1) {
			menu_go_up();
		}
		else if ((key_val >= SCROLL_SPEED) && (key_val % SCROLL_SPEED == 0)) {
			menu_go_up();
		}
	}
	else if (is_menu_key_pressed(MENU_ACCEPT, keyboard))
    {
        auto *entry = &current_menu->entries[current_menu->window_start + current_menu->cur_index];
        assert(entry != nullptr);
        entry->on_select(1.0);

		//current_menu->handler(entry, ENTER);
	}
	else if (is_menu_key_pressed(MENU_BACK, keyboard)) {
		current_menu->go_back();
	}
	else if (is_menu_key_pressed(MENU_LEFT, keyboard) || is_menu_key_pressed(MENU_RIGHT, keyboard)) {

		debug_menu_entry *cur = &current_menu->entries[current_menu->window_start + current_menu->cur_index];
        if (is_menu_key_pressed(MENU_LEFT, keyboard)) {
            cur->on_change(-1.0, false);
        } else {
            cur->on_change(1.0, true);
        }
	}

	debug_menu_entry *highlighted = &current_menu->entries[current_menu->window_start + current_menu->cur_index];
    assert(highlighted->frame_advance_callback != nullptr);
    highlighted->frame_advance_callback(highlighted);
}


HRESULT __stdcall GetDeviceStateHook(IDirectInputDevice8* self, DWORD cbData, LPVOID lpvData) {

	HRESULT res = GetDeviceStateOriginal(self, cbData, lpvData);

	//printf("cbData %d %d %d\n", cbData, sizeof(DIJOYSTATE), sizeof(DIJOYSTATE2));


	
	//keyboard time babyyy
	if (cbData == 256 || cbData == sizeof(DIJOYSTATE2)) {

		
		if (cbData == 256)
			GetDeviceStateHandleKeyboardInput(lpvData);
		else if (cbData == sizeof(DIJOYSTATE2))
			GetDeviceStateHandleControllerInput(lpvData);

		int game_state = 0;
		if (g_game_ptr())
        {
			game_state = game_get_cur_state(g_game_ptr());
        }

		//printf("INSERT %d %d %c\n", keys[DIK_INSERT], game_state, debug_enabled ? 'y' : 'n');

		int keyboard = cbData == 256;
		menu_setup(game_state, keyboard);

		if (debug_enabled) {
			menu_input_handler(keyboard, 5);
		}

	}


	if (debug_enabled) {
		memset(lpvData, 0, cbData);
	}



	//printf("Device State called %08X %d\n", this, cbData);

	return res;
}

typedef HRESULT(__stdcall* GetDeviceData_ptr)(IDirectInputDevice8*, DWORD, LPDIDEVICEOBJECTDATA, LPDWORD, DWORD);
GetDeviceData_ptr GetDeviceDataOriginal = nullptr;

HRESULT __stdcall GetDeviceDataHook(IDirectInputDevice8* self, DWORD cbObjectData, LPDIDEVICEOBJECTDATA rgdod, LPDWORD pdwInOut, DWORD dwFlags) {

	HRESULT res = GetDeviceDataOriginal(self, cbObjectData, rgdod, pdwInOut, dwFlags);

	printf("data\n");
	if (res == DI_OK) {

		printf("All gud\n");
		for (int i = 0; i < *pdwInOut; i++) {


			if (LOBYTE(rgdod[i].dwData) > 0) {

				if (rgdod[i].dwOfs == DIK_ESCAPE) {

					printf("Pressed escaped\n");
					__debugbreak();
				}
			}
		}
	}
	//printf("Device Data called %08X\n", this);

	return res;
}



typedef HRESULT(__stdcall* IDirectInput8CreateDevice_ptr)(IDirectInput8W*, const GUID*, LPDIRECTINPUTDEVICE8W*, LPUNKNOWN);
IDirectInput8CreateDevice_ptr createDeviceOriginal = nullptr;

HRESULT  __stdcall IDirectInput8CreateDeviceHook(IDirectInput8W* self, const GUID* guid, LPDIRECTINPUTDEVICE8W* device, LPUNKNOWN unk) {

	//printf("CreateDevice %d %d %d %d %d %d %d\n", *guid, GUID_SysMouse, GUID_SysKeyboard, GUID_SysKeyboardEm, GUID_SysKeyboardEm2, GUID_SysMouseEm, GUID_SysMouseEm2);
	printf("Guid = {%08lX-%04hX-%04hX-%02hhX%02hhX-%02hhX%02hhX%02hhX%02hhX%02hhX%02hhX}\n",
		guid->Data1, guid->Data2, guid->Data3,
		guid->Data4[0], guid->Data4[1], guid->Data4[2], guid->Data4[3],
		guid->Data4[4], guid->Data4[5], guid->Data4[6], guid->Data4[7]);

	HRESULT res = createDeviceOriginal(self, guid, device, unk);


	if (IsEqualGUID(GUID_SysMouse, *guid))
		return res; // ignore mouse

	if (IsEqualGUID(GUID_SysKeyboard, *guid))
		puts("Found the keyboard");
	else
		puts("Hooking something different...maybe a controller");

#if 0
	DWORD* vtbl = (*device)->lpVtbl;
	if (!GetDeviceStateOriginal) {
		GetDeviceStateOriginal = vtbl[9];
		vtbl[9] = GetDeviceStateHook;
	}

	if (!GetDeviceDataOriginal) {
		GetDeviceDataOriginal = vtbl[10];
		vtbl[10] = GetDeviceDataHook;
	}
#else
    if (GetDeviceStateOriginal == nullptr) {
        GetDeviceStateOriginal = (GetDeviceState_ptr)
            HookVTableFunction((void *) *device, (void *) GetDeviceStateHook, 9);
    }

    if (GetDeviceDataOriginal == nullptr) {
        GetDeviceDataOriginal = (GetDeviceData_ptr) HookVTableFunction((void *) *device,
                                                                       (void *) GetDeviceDataHook,
                                                                       10);
    }
#endif

	return res;
}

typedef HRESULT(__stdcall* IDirectInput8Release_ptr)(IDirectInput8W*);
IDirectInput8Release_ptr releaseDeviceOriginal = nullptr;

HRESULT  __stdcall IDirectInput8ReleaseHook(IDirectInput8W* self) {

	printf("Release\n");

	return releaseDeviceOriginal(self);
}

/*
BOOL CALLBACK EnumDevices(LPCDIDEVICEINSTANCE lpddi, LPVOID buffer) {

	wchar_t wGUID[40] = { 0 };
	char cGUID[40] = { 0 };

	//printf("%d\n", lpddi->guidProduct);
}
*/

typedef HRESULT(__stdcall* DirectInput8Create_ptr)(HINSTANCE hinst, DWORD dwVersion, REFIID riidltf, LPVOID* ppvOut, LPUNKNOWN punkOuter);
HRESULT __stdcall HookDirectInput8Create(HINSTANCE hinst, DWORD dwVersion, REFIID riidltf, LPVOID* ppvOut, LPUNKNOWN punkOuter)
{
	DirectInput8Create_ptr caller = (decltype(caller)) *(void**)0x00987944;
	HRESULT res = caller(hinst, dwVersion, riidltf, ppvOut, punkOuter);

	IDirectInput8* iDir = (IDirectInput8 *) (*ppvOut);
	//printf("it's me mario %08X %08X\n", ppvOut, (*iDir)->lpVtbl);

#if 0
	DWORD* vtbl = (DWORD*)(*iDir)->lpVtbl;
	if (!createDeviceOriginal) {
		createDeviceOriginal = vtbl[3];
		vtbl[3] = IDirectInput8CreateDeviceHook;
	}
#else
    if (createDeviceOriginal == nullptr) {
        createDeviceOriginal = (IDirectInput8CreateDevice_ptr)
            HookVTableFunction((void *) iDir, (void *) IDirectInput8CreateDeviceHook, 3);
    }
#endif

	//(*iDir)->lpVtbl->EnumDevices(*iDir, DI8DEVCLASS_ALL, EnumDevices, NULL, DIEDFL_ALLDEVICES);
	return res;
}

DWORD hookDirectInputAddress = (DWORD) HookDirectInput8Create;

/*
void update_state() {

	while (1) {
		OutputDebugStringA("PILA %d", 6);
	}
}
*/

typedef int(__fastcall* game_handle_game_states_ptr)(game* , void* edx, void* a2);
game_handle_game_states_ptr game_handle_game_states_original = (game_handle_game_states_ptr) 0x0055D510;

int __fastcall game_handle_game_states(game* self, void* edx, void* a2) {

	if (!g_game_ptr()) {
		g_game_ptr() = self;
	}

	/*
	if (game_get_cur_state(this) == 14)
		__debugbreak();
		*/


		//printf("Current state %d %08X\n", game_get_cur_state(this), g_game_ptr);

	return game_handle_game_states_original(self, edx, a2);
}




typedef DWORD(__fastcall* ai_hero_base_state_check_transition_ptr)(DWORD* , void* edx, DWORD* a2, int a3);
ai_hero_base_state_check_transition_ptr ai_hero_base_state_check_transition = (ai_hero_base_state_check_transition_ptr) 0x00478D80;

DWORD __fastcall ai_hero_base_state_check_transition_hook(DWORD* self, void* edx, DWORD* a2, int a3) {
	ai_current_player = self;
	return ai_hero_base_state_check_transition(self, edx, a2, a3);
}

typedef DWORD* (__fastcall* get_info_node_ptr)(void*, void* edx, int a2, char a3);
get_info_node_ptr get_info_node = (get_info_node_ptr) 0x006A3390;

DWORD* __fastcall get_info_node_hook(void* self, void* edx, int a2, char a3) {

	DWORD* res = get_info_node(self, edx, a2, a3);

	fancy_player_ptr = res;
	return res;
}

void init_shadow_targets()
{
    debug_menu::init();

    CDECL_CALL(0x00592E80);
}


typedef void (__fastcall* resource_pack_streamer_load_internal_ptr)(void* self, void* edx, char* str, int a3, int a4, int a5);
resource_pack_streamer_load_internal_ptr resource_pack_streamer_load_internal = (resource_pack_streamer_load_internal_ptr) 0x0054C580;

void __fastcall resource_pack_streamer_load_internal_hook(void* self, void* edx, char* str, int a3, int a4, int a5) {

	resource_pack_streamer_load_internal(self, edx, str, a3, a4, a5);
}

uint8_t __fastcall os_developer_options(BYTE *self, void *edx, int flag) {

	char** flag_list = (decltype(flag_list)) 0x936420;
	char* flag_text = flag_list[flag];
		
	uint8_t res = self[flag + 4];

	if (flag == 0x90) {
		printf("Game wants to know about: %d (%s) -> %d\n", flag, flag_text, res);
		__debugbreak();
	}
	
	
	//this[5 + 4] = 1;
	
	return res;
}

unsigned int hook_controlfp(unsigned int, unsigned int) {
    return {};
}

static constexpr uint32_t NOP = 0x90;

void set_nop(ptrdiff_t address, size_t num_bytes) {
    for (size_t i = 0u; i < num_bytes; ++i) {
        *bit_cast<uint8_t *>(static_cast<size_t>(address) + i) = NOP;
    }
}

void install_patches()
{

    //fix invalid float operation
    {
        set_nop(0x005AC34C, 6);

        HookFunc(0x005AC347, (DWORD) hook_controlfp, 0, "Patching call to controlfp");
    }

    REDIRECT(0x005E10EE, init_shadow_targets);

    input_mgr_patch();

    mouselook_controller_patch();

    spider_monkey_patch();

    message_board_patch();
    
    wds_patch();

    ngl_patch();

    game_patch();

    slab_allocator_patch();

    HookFunc(0x0052B4BF, (DWORD) spider_monkey::render, 0, "Patching call to spider_monkey::render");


	HookFunc(0x004EACF0, (DWORD)aeps_RenderAll, 0, "Patching call to aeps::RenderAll");

	HookFunc(0x0052B5D7, (DWORD)myDebugMenu, 0, "Hooking nglListEndScene to inject debug menu");
	//save orig ptr
	nflSystemOpenFile_orig = *nflSystemOpenFile_data;
	*nflSystemOpenFile_data = &nflSystemOpenFile;
	printf("Replaced nflSystemOpenFile %08X -> %08X\n", (DWORD)nflSystemOpenFile_orig, (DWORD)&nflSystemOpenFile);


	ReadOrWrite_orig = *ReadOrWrite_data;
	*ReadOrWrite_data = &ReadOrWrite;
	printf("Replaced ReadOrWrite %08X -> %08X\n", (DWORD)ReadOrWrite_orig, (DWORD)&ReadOrWrite);

	*(DWORD*)0x008218B2 = (DWORD) &hookDirectInputAddress;
	printf("Patching the DirectInput8Create call\n");


	HookFunc(0x0055D742, (DWORD)game_handle_game_states, 0, "Hooking handle_game_states");

	/*
	WriteDWORD(0x00877524, ai_hero_base_state_check_transition_hook, "Hooking check_transition for peter hooded");
	WriteDWORD(0x00877560, ai_hero_base_state_check_transition_hook, "Hooking check_transition for spider-man");
	WriteDWORD(0x0087759C, ai_hero_base_state_check_transition_hook, "Hooking check_transition for venom");
	*/

	HookFunc(0x00478DBF, (DWORD) get_info_node_hook, 0, "Hook get_info_node to get player ptr");


	WriteDWORD(0x0089C710, (DWORD) slf__create_progression_menu_entry, "Hooking first ocurrence of create_progession_menu_entry");
	WriteDWORD(0x0089C718, (DWORD) slf__create_progression_menu_entry, "Hooking second  ocurrence of create_progession_menu_entry");


	WriteDWORD(0x0089AF70, (DWORD) slf__create_debug_menu_entry, "Hooking first ocurrence of create_debug_menu_entry");
	WriteDWORD(0x0089C708, (DWORD) slf__create_debug_menu_entry, "Hooking second  ocurrence of create_debug_menu_entry");


	HookFunc(0x005AD77D, (DWORD) construct_client_script_libs_hook, 0, "Hooking construct_client_script_libs to inject my vm");

	WriteDWORD(0x0089C720, (DWORD) slf__destroy_debug_menu_entry__debug_menu_entry, "Hooking destroy_debug_menu_entry");
	WriteDWORD(0x0089C750, (DWORD) slf__debug_menu_entry__set_handler__str, "Hooking set_handler");

	//HookFunc(0x0054C89C, resource_pack_streamer_load_internal_hook, 0, "Hooking resource_pack_streamer::load_internal to inject interior loading");

	//HookFunc(0x005B87E0, os_developer_options, 1, "Hooking os_developer_options::get_flag");

	/*

	DWORD* windowHandler = 0x005AC48B;
	*windowHandler = WindowHandler;

	DWORD* otherHandler = 0x0076D6D1;
	*otherHandler = 0;

	*/
}

    bool equal(float first1, float last1)
{
    return 1.0;
}

void close_debug()
{
    debug_enabled = 0;
    debug_disabled = 0;
    game_unpause(g_game_ptr());
    g_game_ptr()->enable_physics(true);
}


void handle_debug_entry(debug_menu_entry* entry, custom_key_type) {
	current_menu = entry->m_value.p_menu;
}

typedef bool (__fastcall *entity_tracker_manager_get_the_arrow_target_pos_ptr)(void *, void *, vector3d *);
entity_tracker_manager_get_the_arrow_target_pos_ptr entity_tracker_manager_get_the_arrow_target_pos = (entity_tracker_manager_get_the_arrow_target_pos_ptr) 0x0062EE10;


void debug_menu_enabled()
{
    debug_enabled = 1;
    game_unpause(g_game_ptr());
    g_game_ptr()->enable_physics(true);
}


void handle_devopt_entry(debug_menu_entry* entry, custom_key_type key_type)
{
    printf("handle_game_entry = %s, %s, entry_type = %s\n", entry->text, to_string(key_type), to_string(entry->entry_type));

    if (key_type == ENTER) {
        switch (entry->entry_type) {
        case UNDEFINED: {
            if (entry->m_game_flags_handler != nullptr) {
                entry->m_game_flags_handler(entry);
            }
            break;
        }
        case BOOLEAN_E:
        case POINTER_BOOL: {
            auto v3 = entry->get_bval();
            entry->set_bval(!v3, true);
            break;
        }
        case POINTER_MENU: {
            if (entry->m_value.p_menu != nullptr) {
                current_menu = entry->m_value.p_menu;
            }
            return;
        }
        default:
            break;
        }
    } else if (key_type == LEFT) {
        entry->on_change(-1.0, false);
        auto v3 = entry->get_bval();
        entry->set_bval(!v3, true);
    } else if (key_type == RIGHT) {
        entry->on_change(1.0, true);
        auto v3 = entry->get_bval();
        entry->set_bval(!v3, true);
    }
}

 

typedef void (*us_lighting_switch_time_of_day_ptr)(int time_of_day);
us_lighting_switch_time_of_day_ptr us_lighting_switch_time_of_day = (us_lighting_switch_time_of_day_ptr)0x00408790;




void devopt_ints_handler(debug_menu_entry* a1)
{
    switch (a1->get_id()) {
    case 0u: // Show Districts
    {
        debug_menu::hide();
        os_developer_options::instance()->set_int(mString { "DIFFICULTY" }, a1->get_ival());

        if (a1->get_ival()) {
            os_developer_options::instance()->set_int(mString { "DIFFICULTY" }, true);
        }
        debug_menu_enabled();
        // TODO
        // sub_66C242(&g_game_ptr->field_4C);
        break;
    }
    case 1u: // Show Districts
    {
        debug_menu::hide();
        os_developer_options::instance()->set_int(mString { "CAMERA_STYLE" }, a1->get_ival());

        if (a1->get_ival()) {
            os_developer_options::instance()->set_int(mString { "CAMERA_STYLE" }, true);
        }
        debug_menu_enabled();
        // TODO
        // sub_66C242(&g_game_ptr->field_4C);
        break;
    }
    case 2u: // Show Districts
    {
        debug_menu::hide();
        os_developer_options::instance()->set_int(mString { "CAMERA_STATE" }, a1->get_ival());

        if (a1->get_ival()) {
            os_developer_options::instance()->set_int(mString { "CAMERA_STATE" }, true);
        }
        debug_menu_enabled();
        // TODO
        // sub_66C242(&g_game_ptr->field_4C);
        break;
    }
    case 3u: // Show Districts
    {
        debug_menu::hide();
        os_developer_options::instance()->set_int(mString { "CAMERA_FOV" }, a1->get_ival());

        if (a1->get_ival()) {
            os_developer_options::instance()->set_int(mString { "CAMERA_FOV" }, true);
        }
        debug_menu_enabled();
        // TODO
        // sub_66C242(&g_game_ptr->field_4C);
        break;
    }
    case 4u: // Show Districts
    {
        debug_menu::hide();
        os_developer_options::instance()->set_int(mString { "FOG_RED" }, a1->get_ival());

        if (a1->get_ival()) {
            os_developer_options::instance()->set_int(mString { "FOG_RED" }, true);
        }
        debug_menu_enabled();
        // TODO
        // sub_66C242(&g_game_ptr->field_4C);
        break;
    }
    case 5u: // Show Districts
    {
        debug_menu::hide();
        os_developer_options::instance()->set_int(mString { "FOG_GREEN" }, a1->get_ival());

        if (a1->get_ival()) {
            os_developer_options::instance()->set_int(mString { "FOG_GREEN" }, true);
        }
        debug_menu_enabled();
        // TODO
        // sub_66C242(&g_game_ptr->field_4C);
        break;
    }
    case 6u: // Show Districts
    {
        debug_menu::hide();
        os_developer_options::instance()->set_int(mString { "FOG_BLUE" }, a1->get_ival());

        if (a1->get_ival()) {
            os_developer_options::instance()->set_int(mString { "FOG_BLUE" }, true);
        }
        debug_menu_enabled();
        // TODO
        // sub_66C242(&g_game_ptr->field_4C);
        break;
    }
    case 7u: // Show Districts
    {
        debug_menu::hide();
        os_developer_options::instance()->set_int(mString { "FOG_DISTANCE" }, a1->get_ival());

        if (a1->get_ival()) {
            os_developer_options::instance()->set_int(mString {"FOG_DISTANCE" }, true);
        }
        debug_menu_enabled();
        // TODO
        // sub_66C242(&g_game_ptr->field_4C);
        break;
    }

    case 8u: // Show Districts
    {
        debug_menu::hide();
        os_developer_options::instance()->set_int(mString { "BIT_DEPTH" }, a1->get_ival());

        if (a1->get_ival()) {
            os_developer_options::instance()->set_int(mString { "BIT_DEPTH" }, true);
        }
        debug_menu_enabled();
        // TODO
        // sub_66C242(&g_game_ptr->field_4C);
        break;
    }

    case 9u: // Show Districts
    {
        debug_menu::hide();
        os_developer_options::instance()->set_int(mString { "MONKEY_MODE" }, a1->get_ival());

        if (a1->get_ival()) {
            os_developer_options::instance()->set_int(mString { "MONKEY_MODE" }, true);
        }
        debug_menu_enabled();
        // TODO
        // sub_66C242(&g_game_ptr->field_4C);
        break;
    }

    case 10u: // Show Districts
    {
        debug_menu::hide();
        os_developer_options::instance()->set_int(mString { "RANDOM_SEED" }, a1->get_ival());

        if (a1->get_ival()) {
            os_developer_options::instance()->set_int(mString { "RANDOM_SEED" }, true);
        }
        debug_menu_enabled();
        // TODO
        // sub_66C242(&g_game_ptr->field_4C);
        break;
    }

    case 11u: // Show Districts
    {
        debug_menu::hide();
        os_developer_options::instance()->set_int(mString { "FORCE_WIN" }, a1->get_ival());

        if (a1->get_ival()) {
            os_developer_options::instance()->set_int(mString { "FORCE_WIN" }, true);
        }
        debug_menu_enabled();
        // TODO
        // sub_66C242(&g_game_ptr->field_4C);
        break;
    }

    case 12u: // Show Districts
    {
        debug_menu::hide();
        os_developer_options::instance()->set_int(mString { "CONTROLLER_TYPE" }, a1->get_ival());

        if (a1->get_ival()) {
            os_developer_options::instance()->set_int(mString { "CONTROLLER_TYPE" }, true);
        }
        debug_menu_enabled();
        // TODO
        // sub_66C242(&g_game_ptr->field_4C);
        break;
    }

    case 13u: // Show Districts
    {
        debug_menu::hide();

        os_developer_options::instance()->set_int(mString { "FRAME_LOCK" }, a1->get_ival());

        if (a1->get_ival()) {
            os_developer_options::instance()->set_int(mString { "FRAME_LOCK" }, true);
        }
        debug_menu_enabled();
        // TODO
        // sub_66C242(&g_game_ptr->field_4C);
        break;
    }

    case 14u: // Show Districts
    {

        debug_menu::hide();
        os_developer_options::instance()->set_int(mString { "FRAME_LIMIT" }, a1->get_ival());

        if (a1->get_ival()) {
            os_developer_options::instance()->set_int(mString { "FRAME_LIMIT" }, true);
        }
    

        debug_menu_enabled();
        // TODO
        // sub_66C242(&g_game_ptr->field_4C);
    }

    case 15u: // Show Districts
    {

        debug_menu::hide();
        os_developer_options::instance()->set_int(mString { "SWING_DEBUG_TRAILS" }, a1->get_ival());

        if (a1->get_ival()) {
            os_developer_options::instance()->set_int(mString { "SWING_DEBUG_TRAILS" }, true);
        }
        debug_menu_enabled();
        // TODO
        // sub_66C242(&g_game_ptr->field_4C);
        break;
    }

    case 16u: // Show Districts
    {
        debug_menu::hide();
        os_developer_options::instance()->set_int(mString { "SOAK_SMOKE"  },a1->get_ival());

        if (a1->get_ival()) {
            os_developer_options::instance()->set_int(mString { "SOAK_SMOKE"  }, true);
        }
        debug_menu_enabled();
        // TODO
        // sub_66C242(&g_game_ptr->field_4C);
        break;
    }

    case 17u: // Show Districts
    {
        debug_menu::hide();
        os_developer_options::instance()->set_int(mString { "FAR_CLIP_PLANE" }, a1->get_ival());

        if (a1->get_ival()) {
            os_developer_options::instance()->set_int(mString {  "FAR_CLIP_PLANE"  }, true);
        }
        debug_menu_enabled();
        // TODO
        // sub_66C242(&g_game_ptr->field_4C);
        break;
    }

    case 18u: // Show Districts
    {
        debug_menu::hide();
        os_developer_options::instance()->set_int(mString { "POI_DISPLAY_TYPE" }, a1->get_ival());

        if (a1->get_ival()) {
            os_developer_options::instance()->set_int(mString { "POI_DISPLAY_TYPE" }, true);
        }
        debug_menu_enabled();
        // TODO
        // sub_66C242(&g_game_ptr->field_4C);
        break;
    }

    case 19u: // Show Districts
    {
        debug_menu::hide();
        os_developer_options::instance()->set_int(mString { "STORY_MISSION" }, a1->get_ival());

        if (a1->get_ival()) {
            os_developer_options::instance()->set_int(mString {  "STORY_MISSION" }, true);
        }
        debug_menu_enabled();
        // TODO
        // sub_66C242(&g_game_ptr->field_4C);
        break;
    }

    case 20u: // Show Districts
    {
        debug_menu::hide();
        os_developer_options::instance()->set_int(mString { "EXEC_DELAY" }, a1->get_ival());

        if (a1->get_ival()) {
            os_developer_options::instance()->set_int(mString { "EXEC_DELAY" }, true);
        }
        debug_menu_enabled();
        // TODO
        // sub_66C242(&g_game_ptr->field_4C);
        break;
    }

    case 21u: // Show Districts
    {

        debug_menu::hide();
        os_developer_options::instance()->set_int(mString { "RUN_LENGTH" }, a1->get_ival());

        if (a1->get_ival()) {
            os_developer_options::instance()->set_int(mString { "RUN_LENGTH" }, true);
        }
        debug_menu_enabled();
        // TODO
        // sub_66C242(&g_game_ptr->field_4C);
        break;
    }

    case 22u: // Show Districts
    {
        debug_menu::hide();
        os_developer_options::instance()->set_int(mString { "PC_WINDOW_TOP" }, a1->get_ival());

        if (a1->get_ival()) {
            os_developer_options::instance()->set_int(mString {  "PC_WINDOW_TOP" }, true);
        }
        debug_menu_enabled();
        // TODO
        // sub_66C242(&g_game_ptr->field_4C);
        break;
    }

    case 23u: // Show Districts
    {
        debug_menu::hide();
        os_developer_options::instance()->set_int(mString { "PC_WINDOW_LEFT" }, a1->get_ival());

        if (a1->get_ival()) {
            os_developer_options::instance()->set_int(mString { "PC_WINDOW_LEFT" }, true);
        }
        debug_menu_enabled();
        // TODO
        // sub_66C242(&g_game_ptr->field_4C);
        break;
    }

    case 24u: // Show Districts
    {
        debug_menu::hide();
        os_developer_options::instance()->set_int(mString { "PC_WINDOW_WIDTH" }, a1->get_ival());

        if (a1->get_ival()) {
            os_developer_options::instance()->set_int(mString { "PC_WINDOW_WIDTH" }, true);
        }
        debug_menu_enabled();
        // TODO
        // sub_66C242(&g_game_ptr->field_4C);
        break;
    }

    case 25u: // Show Districts
    {
        debug_menu::hide();
        os_developer_options::instance()->set_int(mString {"PC_WINDOW_HEIGHT" }, a1->get_ival());

        if (a1->get_ival()) {
            os_developer_options::instance()->set_int(mString { "PC_WINDOW_HEIGHT" }, true);
        }
        debug_menu_enabled();
        // TODO
        // sub_66C242(&g_game_ptr->field_4C);
        break;
    }

    case 26u: // Show Districts
    {
        debug_menu::hide();
        os_developer_options::instance()->set_int(mString { "ALLOW_SCREENSHOT" }, a1->get_ival());

        if (a1->get_ival()) {
            os_developer_options::instance()->set_int(mString { "ALLOW_SCREENSHOT" }, true);
        }
        debug_menu_enabled();
        // TODO
        // sub_66C242(&g_game_ptr->field_4C);
        break;
    }

    case 27u: // Show Districts
    {
        debug_menu::hide();
        os_developer_options::instance()->set_int(mString { "AMALGA_REFRESH_INTERVAL" }, a1->get_ival());

        if (a1->get_ival()) {
            os_developer_options::instance()->set_int(mString { "AMALGA_REFRESH_INTERVAL" }, true);
        }
        debug_menu_enabled();
        // TODO
        // sub_66C242(&g_game_ptr->field_4C);
        break;
    }

    case 28u: // Show Districts
    {
        debug_menu::hide();
        os_developer_options::instance()->set_int(mString { "ENABLE_LONG_MALOR_ASSERTS" }, a1->get_ival());

        if (a1->get_ival()) {
            os_developer_options::instance()->set_int(mString { "ENABLE_LONG_MALOR_ASSERTS" }, true);
        }
        debug_menu_enabled();
        // TODO
        // sub_66C242(&g_game_ptr->field_4C);
        break;
    }

    case 29u: // Show Districts
    {
        debug_menu::hide();
        os_developer_options::instance()->set_int(mString { "GOD_MODE" }, a1->get_ival());

        if (a1->get_ival()) {
            os_developer_options::instance()->set_int(mString { "GOD_MODE" }, true);
        }
        debug_menu_enabled();
        // TODO
        // sub_66C242(&g_game_ptr->field_4C);
        break;
    }

    case 30u: // Show Districts
    {
        debug_menu::hide();
        os_developer_options::instance()->set_int(mString {  "PCLISTBUFFER" }, a1->get_ival());

        if (a1->get_ival()) {
            os_developer_options::instance()->set_int(mString {  "PCLISTBUFFER" }, true);
        }
        debug_menu_enabled();
        // TODO
        // sub_66C242(&g_game_ptr->field_4C);
        break;
    }

    case 31u: // Show Districts
    {
        debug_menu::hide();
        os_developer_options::instance()->set_int(mString { "PCSCRATCHBUFFER" },a1->get_ival());

        if (a1->get_ival()) {
            os_developer_options::instance()->set_int(mString { "PCSCRATCHBUFFER"  }, true);
        }
        debug_menu_enabled();
        // TODO
        // sub_66C242(&g_game_ptr->field_4C);
        break;
    }

    case 32u: // Show Districts
    {
        debug_menu::hide();
        os_developer_options::instance()->set_int(mString { "PCSCRATCHINDEXBUFFER" }, a1->get_ival());

        if (a1->get_ival()) {
            os_developer_options::instance()->set_int(mString { "PCSCRATCHINDEXBUFFER" }, true);
        }
        debug_menu_enabled();
        // TODO
        // sub_66C242(&g_game_ptr->field_4C);
        break;
    }

    case 33u: // Show Districts
    {
        debug_menu::hide();
        os_developer_options::instance()->set_int(mString { "PCSCRATCHVERTEXBUFFER" }, a1->get_ival());

        if (a1->get_ival()) {
            os_developer_options::instance()->set_int(mString { "PCSCRATCHVERTEXBUFFER" }, true);
        }
        debug_menu_enabled();
        // TODO
        // sub_66C242(&g_game_ptr->field_4C);
        break;
    }

    case 34u: // Show Districts
    {
        debug_menu::hide();
        os_developer_options::instance()->set_int(mString {  "NAL_HEAP_SIZE" }, a1->get_ival());

        if (a1->get_ival()) {
            os_developer_options::instance()->set_int(mString {  "NAL_HEAP_SIZE" }, true);
        }
        debug_menu_enabled();
        // TODO
        // sub_66C242(&g_game_ptr->field_4C);
        break;
    }

    case 35u: // Show Districts
    {
        debug_menu::hide();
        os_developer_options::instance()->set_int(mString {  "ASSERT_BOX_MARGIN" }, a1->get_ival());

        if (a1->get_ival()) {
            os_developer_options::instance()->set_int(mString {  "ASSERT_BOX_MARGIN" }, true);
        }
        debug_menu_enabled();
        // TODO
        // sub_66C242(&g_game_ptr->field_4C);
        break;
    }

    case 36u: // Show Districts
    {
        debug_menu::hide();
        os_developer_options::instance()->set_int(mString { "ASSERT_TEXT_MARGIN" }, a1->get_ival());

        if (a1->get_ival()) {
            os_developer_options::instance()->set_int(mString { "ASSERT_TEXT_MARGIN" }, true);
        }
        debug_menu_enabled();
        // TODO
        // sub_66C242(&g_game_ptr->field_4C);
        break;
    }

    case 37u: // Show Districts
    {
        debug_menu::hide();
        os_developer_options::instance()->set_int(mString {  "ASSERT_FONT_PCT_X" }, a1->get_ival());

        if (a1->get_ival()) {
            os_developer_options::instance()->set_int(mString {  "ASSERT_FONT_PCT_X" }, true);
        }
        debug_menu_enabled();
        // TODO
        // sub_66C242(&g_game_ptr->field_4C);
        break;
    }

    case 38u: // Show Districts
    {
        debug_menu::hide();
        os_developer_options::instance()->set_int(mString {  "ASSERT_FONT_PCT_Y" }, a1->get_ival());

        if (a1->get_ival()) {
            os_developer_options::instance()->set_int(mString {  "ASSERT_FONT_PCT_Y" }, true);
        }
        debug_menu_enabled();
        // TODO
        // sub_66C242(&g_game_ptr->field_4C);
        break;
    }

    case 39u: // Show Districts
    {
        debug_menu::hide();
        os_developer_options::instance()->set_int(mString { "STREAMER_INFO_FONT_PCT" }, a1->get_ival());

        if (a1->get_ival()) {
            os_developer_options::instance()->set_int(mString {  "STREAMER_INFO_FONT_PCT" }, true);
        }
        debug_menu_enabled();
        // TODO
        // sub_66C242(&g_game_ptr->field_4C);
        break;
    }
    case 40u: // Show Districts
    {
        debug_menu::hide();
        os_developer_options::instance()->set_int(mString { "DEBUG_INFO_FONT_PCT" }, a1->get_ival());

        if (a1->get_ival()) {
            os_developer_options::instance()->set_int(mString {  "DEBUG_INFO_FONT_PCT" }, true);
        }
        debug_menu_enabled();
        // TODO
        // sub_66C242(&g_game_ptr->field_4C);
        break;
    }

    case 41u: // Show Districts
    {
        debug_menu::hide();
        os_developer_options::instance()->set_int(mString { "PITCH_FACTOR" }, a1->get_ival());

        if (a1->get_ival()) {
            os_developer_options::instance()->set_int(mString { "PITCH_FACTOR" }, true);
        }
        debug_menu_enabled();
        // TODO
        // sub_66C242(&g_game_ptr->field_4C);
        break;
    }
    case 42u: // Show Districts
    {
        debug_menu::hide();
        os_developer_options::instance()->set_int(mString { "BANK_FACTOR" }, a1->get_ival());

        if (a1->get_ival()) {
            os_developer_options::instance()->set_int(mString { "BANK_FACTOR" }, true);
        }
        debug_menu_enabled();
        // TODO
        // sub_66C242(&g_game_ptr->field_4C);
        break;
    }

    case 43u: // Show Districts
    {
        debug_menu::hide();
        os_developer_options::instance()->set_int(mString { "SWING_INTERPOLATION_TIME" }, a1->get_ival());

        if (a1->get_ival()) {
            os_developer_options::instance()->set_int(mString { "SWING_INTERPOLATION_TIME" }, true);
        }
        debug_menu_enabled();
        // TODO
        // sub_66C242(&g_game_ptr->field_4C);
        break;
    }
    case 44u: // Show Districts
    {
        debug_menu::hide();
        os_developer_options::instance()->set_int(mString { "BOTH_HANDS_INTERPOLATION_TIME" }, a1->get_ival());

        if (a1->get_ival()) {
            os_developer_options::instance()->set_int(mString { "BOTH_HANDS_INTERPOLATION_TIME" }, true);
        }
        debug_menu_enabled();
        // TODO
        // sub_66C242(&g_game_ptr->field_4C);
        break;
    }

    case 45u: // Show Districts
    {
        debug_menu::hide();
        os_developer_options::instance()->set_int(mString { "MEM_DUMP_FRAME" }, a1->get_ival());

        if (a1->get_ival()) {
            os_developer_options::instance()->set_int(mString { "MEM_DUMP_FRAME" }, true);
        }
        debug_menu_enabled();
        // TODO
        // sub_66C242(&g_game_ptr->field_4C);
        break;
    }
    case 46u: // Show Districts
    {
        debug_menu::hide();
        os_developer_options::instance()->set_int(mString { "HERO_START_X" }, a1->get_ival());

        if (a1->get_ival()) {
            os_developer_options::instance()->set_int(mString { "HERO_START_X" }, true);
        }
        debug_menu_enabled();
        // TODO
        // sub_66C242(&g_game_ptr->field_4C);
        break;
    }

    case 47u: // Show Districts
    {
        debug_menu::hide();
        os_developer_options::instance()->set_int(mString { "HERO_START_Y" }, a1->get_ival());

        if (a1->get_ival()) {
            os_developer_options::instance()->set_int(mString { "HERO_START_Y" }, true);
        }
        debug_menu_enabled();
        // TODO
        // sub_66C242(&g_game_ptr->field_4C);
        break;
    }
    case 48u: // Show Districts
    {
        debug_menu::hide();
        os_developer_options::instance()->set_int(mString { "HERO_START_Z" }, a1->get_ival());

        if (a1->get_ival()) {
            os_developer_options::instance()->set_int(mString { "HERO_START_Z" }, true);
        }
        debug_menu_enabled();
        // TODO
        // sub_66C242(&g_game_ptr->field_4C);
        break;
    }

    case 49u: // Show Districts
    {
        debug_menu::hide();
        os_developer_options::instance()->set_int(mString { "SHOW_SOUND_INFO" }, a1->get_ival());

        if (a1->get_ival()) {
            os_developer_options::instance()->set_int(mString { "SHOW_SOUND_INFO" }, true);
        }
        debug_menu_enabled();
        // TODO
        // sub_66C242(&g_game_ptr->field_4C);
        break;
    }
    case 50u: // Show Districts
    {
        debug_menu::hide();
        os_developer_options::instance()->set_int(mString { "SHOW_VOICE_BOX_INFO" }, a1->get_ival());

        if (a1->get_ival()) {
            os_developer_options::instance()->set_int(mString { "SHOW_VOICE_BOX_INFO" }, true);
        }
        debug_menu_enabled();
        // TODO
        // sub_66C242(&g_game_ptr->field_4C);
        break;
    }

    case 51u: // Show Districts
    {
        debug_menu::hide();
        os_developer_options::instance()->set_int(mString { "DEBUG_CAMERA_PITCH_MULTIPLIER" }, a1->get_ival());

        if (a1->get_ival()) {
            os_developer_options::instance()->set_int(mString { "DEBUG_CAMERA_PITCH_MULTIPLIER" }, true);
        }
        debug_menu_enabled();
        // TODO
        // sub_66C242(&g_game_ptr->field_4C);
        break;
    }
    case 52u: // Show Districts
    {
        debug_menu::hide();
        os_developer_options::instance()->set_int(mString { "DEBUG_CAMERA_YAW_MULTIPLIER" }, a1->get_ival());

        if (a1->get_ival()) {
            os_developer_options::instance()->set_int(mString { "DEBUG_CAMERA_YAW_MULTIPLIER" }, true);
        }
        debug_menu_enabled();
        // TODO
        // sub_66C242(&g_game_ptr->field_4C);
        break;
    }

    case 53u: // Show Districts
    {
        debug_menu::hide();
        os_developer_options::instance()->set_int(mString { "DEBUG_CAMERA_MOVE_MULTIPLIER" }, a1->get_ival());

        if (a1->get_ival()) {
            os_developer_options::instance()->set_int(mString { "DEBUG_CAMERA_MOVE_MULTIPLIER" }, true);
        }
        debug_menu_enabled();
        // TODO
        // sub_66C242(&g_game_ptr->field_4C);
        break;
    }
    case 54u: // Show Districts
    {
        debug_menu::hide();
        os_developer_options::instance()->set_int(mString { "DEBUG_CAMERA_STRAFE_MULTIPLIER" }, a1->get_ival());

        if (a1->get_ival()) {
            os_developer_options::instance()->set_int(mString {"DEBUG_CAMERA_STRAFE_MULTIPLIER" }, true);
        }
        debug_menu_enabled();
        // TODO
        // sub_66C242(&g_game_ptr->field_4C);
        break;
    }

    case 55u: // Show Districts
    {
        debug_menu::hide();
        os_developer_options::instance()->set_int(mString { "TAM_SCALE_MIN_DISTANCE" }, a1->get_ival());

        if (a1->get_ival()) {
            os_developer_options::instance()->set_int(mString { "TAM_SCALE_MIN_DISTANCE" }, true);
        }
        debug_menu_enabled();
        // TODO
        // sub_66C242(&g_game_ptr->field_4C);
        break;
    }
    case 56u: // Show Districts
    {
        debug_menu::hide();
        os_developer_options::instance()->set_int(mString {"TAM_SCALE_MAX_DISTANCE" }, a1->get_ival());

        if (a1->get_ival()) {
            os_developer_options::instance()->set_int(mString { "TAM_SCALE_MAX_DISTANCE" }, true);
        }
        debug_menu_enabled();
        // TODO
        // sub_66C242(&g_game_ptr->field_4C);
        break;
    }

    case 57u: // Show Districts
    {
        debug_menu::hide();
        os_developer_options::instance()->set_int(mString { "TAM_SCALE_MIN_PERCENT" }, a1->get_ival());

        if (a1->get_ival()) {
            os_developer_options::instance()->set_int(mString { "TAM_SCALE_MIN_PERCENT" }, true);
        }
        debug_menu_enabled();
        // TODO
        // sub_66C242(&g_game_ptr->field_4C);
        break;
    }
    case 58u: // Show Districts
    {
        debug_menu::hide();
        os_developer_options::instance()->set_int(mString { "THUG_HEALTH_UI_SCALE_MIN_DISTANCE" }, a1->get_ival());

        if (a1->get_ival()) {
            os_developer_options::instance()->set_int(mString { "THUG_HEALTH_UI_SCALE_MIN_DISTANCE" }, true);
        }
        debug_menu_enabled();
        // TODO
        // sub_66C242(&g_game_ptr->field_4C);
        break;
    }

    case 59u: // Show Districts
    {
        debug_menu::hide();
        os_developer_options::instance()->set_int(mString { "THUG_HEALTH_UI_SCALE_MAX_DISTANCE" }, a1->get_ival());

        if (a1->get_ival()) {
            os_developer_options::instance()->set_int(mString { "THUG_HEALTH_UI_SCALE_MAX_DISTANCE" }, true);
        }
        debug_menu_enabled();
        // TODO
        // sub_66C242(&g_game_ptr->field_4C);
        break;
    }
    case 60u: // Show Districts
    {
        debug_menu::hide();
        os_developer_options::instance()->set_int(mString { "THUG_HEALTH_UI_SCALE_MIN_PERCENT" }, a1->get_ival());

        if (a1->get_ival()) {
            os_developer_options::instance()->set_int(mString { "THUG_HEALTH_UI_SCALE_MIN_PERCENT" }, true);
        }
        debug_menu_enabled();
        // TODO
        // sub_66C242(&g_game_ptr->field_4C);
        break;
    }

    case 61u: // Show Districts
    {
        debug_menu::hide();
        os_developer_options::instance()->set_int(mString { "TARGETING_RETICLE_SCALE_MIN_DISTANCE" }, a1->get_ival());

        if (a1->get_ival()) {
            os_developer_options::instance()->set_int(mString { "TARGETING_RETICLE_SCALE_MIN_DISTANCE" }, true);
        }
        debug_menu_enabled();
        // TODO
        // sub_66C242(&g_game_ptr->field_4C);
        break;
    }
    case 62u: // Show Districts
    {
        debug_menu::hide();
        os_developer_options::instance()->set_int(mString { "TARGETING_RETICLE_SCALE_MAX_DISTANCE" }, a1->get_ival());

        if (a1->get_ival()) {
            os_developer_options::instance()->set_int(mString { "TARGETING_RETICLE_SCALE_MAX_DISTANCE" }, true);
        }
        debug_menu_enabled();
        // TODO
        // sub_66C242(&g_game_ptr->field_4C);
        break;
    }

    case 63u: // Show Districts
    {
        debug_menu::hide();
        os_developer_options::instance()->set_int(mString { "TARGETING_RETICLE_SCALE_MIN_PERCENT" }, a1->get_ival());

        if (a1->get_ival()) {
            os_developer_options::instance()->set_int(mString { "TARGETING_RETICLE_SCALE_MIN_PERCENT" }, true);
        }
        debug_menu_enabled();
        // TODO
        // sub_66C242(&g_game_ptr->field_4C);
        break;
    }
    case 64u: // Show Districts
    {
        debug_menu::hide();
        os_developer_options::instance()->set_int(mString { "HIRES_SCREENSHOT_X" }, a1->get_ival());

        if (a1->get_ival()) {
            os_developer_options::instance()->set_int(mString { "HIRES_SCREENSHOT_X" }, true);
        }
        debug_menu_enabled();
        // TODO
        // sub_66C242(&g_game_ptr->field_4C);
        break;
    }

    case 65u: // Show Districts
    {
        debug_menu::hide();
        os_developer_options::instance()->set_int(mString { "HIRES_SCREENSHOT_Y" }, a1->get_ival());

        if (a1->get_ival()) {
            os_developer_options::instance()->set_int(mString { "HIRES_SCREENSHOT_Y" }, true);
        }
        debug_menu_enabled();
        // TODO
        // sub_66C242(&g_game_ptr->field_4C);
        break;
    }
    case 66u: // Show Districts
    {
        debug_menu::hide();
        DWORD currentTOD = *g_TOD;
        switch (a1->get_ival()) {
        case 0u:
            os_developer_options::instance()->set_int(mString { "TIME_OF_DAY" }, 0);
            us_lighting_switch_time_of_day(modulo(currentTOD - 1, 4));
            break;
        case 1u:
            os_developer_options::instance()->set_int(mString { "TIME_OF_DAY" }, 1);
            us_lighting_switch_time_of_day(modulo(currentTOD + 1, 4));
            break;
        case 2u:
            os_developer_options::instance()->set_int(mString { "TIME_OF_DAY" }, 2);
            us_lighting_switch_time_of_day(modulo(currentTOD + 2, 4));
            break;
        case 3u:
            os_developer_options::instance()->set_int(mString { "TIME_OF_DAY" }, 3);
            us_lighting_switch_time_of_day(modulo(currentTOD + 3, 4));
            break;
        }
        debug_menu_enabled();
        // TODO
        // sub_66C242(&g_game_ptr->field_4C);
        break;
    }

    case 67u: // Show Districts
    {
        debug_menu::hide();
        switch (a1->get_ival()) {
        case 0u:
            os_developer_options::instance()->set_int(mString { "MINI_MAP_ZOOM" }, 50);
            break;
        case 1u:
            os_developer_options::instance()->set_int(mString { "MINI_MAP_ZOOM" }, 60);
            break;
        case 2u:
            os_developer_options::instance()->set_int(mString { "MINI_MAP_ZOOM" }, 70);
            break;
        case 3u:
            os_developer_options::instance()->set_int(mString { "MINI_MAP_ZOOM" }, 80);
            break;
        case 4u:
            os_developer_options::instance()->set_int(mString { "MINI_MAP_ZOOM" }, 90);
            break;
        case 5u:
            os_developer_options::instance()->set_int(mString { "MINI_MAP_ZOOM" }, 100);
            break;
        case 6u:
            os_developer_options::instance()->set_int(mString { "MINI_MAP_ZOOM" }, 110);
            break;
        case 7u:
            os_developer_options::instance()->set_int(mString { "MINI_MAP_ZOOM" }, 120);
            break;
        case 8u:
            os_developer_options::instance()->set_int(mString { "MINI_MAP_ZOOM" }, 130);
            break;
        case 9u:
            os_developer_options::instance()->set_int(mString { "MINI_MAP_ZOOM" }, 140);
            break;
        case 10u:
            os_developer_options::instance()->set_int(mString { "MINI_MAP_ZOOM" }, 150);
            break;
        case 11u:
            os_developer_options::instance()->set_int(mString { "MINI_MAP_ZOOM" }, 160);
            break;
        case 12u:
            os_developer_options::instance()->set_int(mString { "MINI_MAP_ZOOM" }, 170);
            break;
        case 13u:
            os_developer_options::instance()->set_int(mString { "MINI_MAP_ZOOM" }, 180);
            break;
        case 14u:
            os_developer_options::instance()->set_int(mString { "MINI_MAP_ZOOM" }, 190);
            break;
        case 15u:
            os_developer_options::instance()->set_int(mString { "MINI_MAP_ZOOM" }, 200);
            break;
        case 16u:
            os_developer_options::instance()->set_int(mString { "MINI_MAP_ZOOM" }, 210);
            break;
        case 17u:
            os_developer_options::instance()->set_int(mString { "MINI_MAP_ZOOM" }, 220);
            break;
        case 18u:
            os_developer_options::instance()->set_int(mString { "MINI_MAP_ZOOM" }, 230);
            break;
        case 19u:
            os_developer_options::instance()->set_int(mString { "MINI_MAP_ZOOM" }, 240);
            break;
        case 20u:
            os_developer_options::instance()->set_int(mString { "MINI_MAP_ZOOM" }, 250);
            break;
        case 21u:
            os_developer_options::instance()->set_int(mString { "MINI_MAP_ZOOM" }, 260);
            break;
        case 22u:
            os_developer_options::instance()->set_int(mString { "MINI_MAP_ZOOM" }, 270);
            break;
        case 23u:
            os_developer_options::instance()->set_int(mString { "MINI_MAP_ZOOM" }, 280);
            break;
        case 24u:
            os_developer_options::instance()->set_int(mString { "MINI_MAP_ZOOM" }, 290);
            break;
        case 25u:
            os_developer_options::instance()->set_int(mString { "MINI_MAP_ZOOM" }, 300);
            break;
        case 26u:
            os_developer_options::instance()->set_int(mString { "MINI_MAP_ZOOM" }, 310);
            break;
        case 27u:
            os_developer_options::instance()->set_int(mString { "MINI_MAP_ZOOM" }, 320);
            break;
        case 28u:
            os_developer_options::instance()->set_int(mString { "MINI_MAP_ZOOM" }, 330);
            break;
        case 29u:
            os_developer_options::instance()->set_int(mString { "MINI_MAP_ZOOM" }, 340);
            break;
        case 30u:
            os_developer_options::instance()->set_int(mString { "MINI_MAP_ZOOM" }, 350);
            break;
        }
        debug_menu_enabled();
        // TODO
        // sub_66C242(&g_game_ptr->field_4C);
        break;
    }
    case 68u: // Show Districts
    {
        debug_menu::hide();
        os_developer_options::instance()->set_int(mString { "RTDT_REPLAY_BUFFER_SIZE" }, a1->get_ival());

        if (a1->get_ival()) {
            os_developer_options::instance()->set_int(mString { "RTDT_REPLAY_BUFFER_SIZE" }, true);
        }
        debug_menu_enabled();
        // TODO
        // sub_66C242(&g_game_ptr->field_4C);
        break;
    }

    case 69u: // Show Districts
    {
        debug_menu::hide();
        os_developer_options::instance()->set_int(mString { "TIMER_WIDGET_TIME_DELTA_PERCENT" }, a1->get_ival());

        if (a1->get_ival()) {
            os_developer_options::instance()->set_int(mString {"TIMER_WIDGET_TIME_DELTA_PERCENT" }, true);
        }
        debug_menu_enabled();
        // TODO
        // sub_66C242(&g_game_ptr->field_4C);
        break;
    }
    case 70u: // Show Districts
    {
        debug_menu::hide();
        os_developer_options::instance()->set_int(mString { "DEBUG_PARTICLE_LEVEL" }, a1->get_ival());

        if (a1->get_ival()) {
            os_developer_options::instance()->set_int(mString { "DEBUG_PARTICLE_LEVEL" }, true);
        }
        debug_menu_enabled();
        // TODO
        // sub_66C242(&g_game_ptr->field_4C);
        break;
    }

    case 71u: // Show Districts
    {
        debug_menu::hide();
        os_developer_options::instance()->set_int(mString { "DEBUG_PARTICLE_MEMORY" }, a1->get_ival());

        if (a1->get_ival()) {
            os_developer_options::instance()->set_int(mString { "DEBUG_PARTICLE_MEMORY" }, true);
        }
        debug_menu_enabled();
        // TODO
        // sub_66C242(&g_game_ptr->field_4C);
        break;
    }
    case 72u: // Show Districts
    {
        debug_menu::hide();
        os_developer_options::instance()->set_int(mString { "MAX_AEPS_ENTITIES" }, a1->get_ival());

        if (a1->get_ival()) {
            os_developer_options::instance()->set_int(mString { "MAX_AEPS_ENTITIES" }, true);
        }
        debug_menu_enabled();
        // TODO
        // sub_66C242(&g_game_ptr->field_4C);
        break;
    }

    case 73u: // Show Districts
    {
        debug_menu::hide();
        os_developer_options::instance()->set_int(mString { "MAX_AEPS_SPAWNERS" }, a1->get_ival());

        if (a1->get_ival()) {
            os_developer_options::instance()->set_int(mString { "MAX_AEPS_SPAWNERS" }, true);
        }
        debug_menu_enabled();
        // TODO
        // sub_66C242(&g_game_ptr->field_4C);
        break;
    }
    case 74u: // Show Districts
    {
        debug_menu::hide();
        os_developer_options::instance()->set_int(mString { "MAX_AEPS_EMITTERS" }, a1->get_ival());

        if (a1->get_ival()) {
            os_developer_options::instance()->set_int(mString { "MAX_AEPS_EMITTERS" }, true);
        }
        debug_menu_enabled();
        // TODO
        // sub_66C242(&g_game_ptr->field_4C);
        break;
    }
    case 75u: // Show Districts
    {
        debug_menu::hide();
        os_developer_options::instance()->set_int(mString { "MAX_AEPS_PARTICLES" }, a1->get_ival());

        if (a1->get_ival()) {
            os_developer_options::instance()->set_int(mString {  "MAX_AEPS_PARTICLES"  }, true);
        }
        debug_menu_enabled();
        // TODO
        // sub_66C242(&g_game_ptr->field_4C);
        break;
    }
    }
}

void devopt_flags_handler(debug_menu_entry* a1)
{
    switch (a1->get_id()) {
    case 0u: // Show Districts
    {
        debug_menu::hide();
        os_developer_options::instance()->set_flag(mString { "CD_ONLY" }, a1->get_bval());

        if (a1->get_bval()) {
            os_developer_options::instance()->set_flag(mString { "CD_ONLY" }, true);
        }
        debug_menu_enabled();
        // TODO
        // sub_66C242(&g_game_ptr->field_4C);
        break;
    }
    case 1u: // Show Districts
    {
        debug_menu::hide();
        os_developer_options::instance()->set_flag(mString { "ENVMAP_TOOL" }, a1->get_bval());

        if (a1->get_bval()) {
            os_developer_options::instance()->set_flag(mString { "ENVMAP_TOOL" }, true);
        }
        debug_menu_enabled();
        // TODO
        // sub_66C242(&g_game_ptr->field_4C);
        break;
    }
    case 2u: // Show Districts
    {
        debug_menu::hide();
        os_developer_options::instance()->set_flag(mString { "CHATTY_LOAD" }, a1->get_bval());

        if (a1->get_bval()) {
            os_developer_options::instance()->set_flag(mString { "CHATTY_LOAD" }, true);
        }
        debug_menu_enabled();
        // TODO
        // sub_66C242(&g_game_ptr->field_4C);
        break;
    }
    case 3u: // Show Districts
    {
        debug_menu::hide();
        os_developer_options::instance()->set_flag(mString { "NO_CD" }, a1->get_bval());

        if (a1->get_bval()) {
            os_developer_options::instance()->set_flag(mString { "NO_CD" }, true);
        }
        debug_menu_enabled();
        // TODO
        // sub_66C242(&g_game_ptr->field_4C);
        break;
    }
    case 4u: // Show Districts
    {
        debug_menu::hide();
        os_developer_options::instance()->set_flag(mString { "WINDOW_DEFAULT" }, a1->get_bval());

        if (a1->get_bval()) {
            os_developer_options::instance()->set_flag(mString { "WINDOW_DEFAULT" }, true);
        }
        debug_menu_enabled();
        // TODO
        // sub_66C242(&g_game_ptr->field_4C);
        break;
    }
    case 5u: // Show Districts
    {
        debug_menu::hide();
        os_developer_options::instance()->set_flag(mString { "SHOW_FPS" }, a1->get_bval());

        if (a1->get_bval()) {
            os_developer_options::instance()->set_flag(mString { "SHOW_FPS" }, true);
        }
        debug_menu_enabled();
        // TODO
        // sub_66C242(&g_game_ptr->field_4C);
        break;
    }
    case 6u: // Show Districts
    {
        debug_menu::hide();
        os_developer_options::instance()->set_flag(mString { "SHOW_STREAMER_INFO" }, a1->get_bval());

        if (a1->get_bval()) {
            os_developer_options::instance()->set_flag(mString { "SHOW_STREAMER_INFO" }, true);
        }
        debug_menu_enabled();
        // TODO
        // sub_66C242(&g_game_ptr->field_4C);
        break;
    }
    case 7u: // Show Districts
    {
        debug_menu::hide();
        os_developer_options::instance()->set_flag(mString { "SHOW_STREAMER_SPAM" }, a1->get_bval());

        if (a1->get_bval()) {
            os_developer_options::instance()->set_flag(mString { "SHOW_STREAMER_SPAM" }, true);
        }
        debug_menu_enabled();
        // TODO
        // sub_66C242(&g_game_ptr->field_4C);
        break;
    }

    case 8u: // Show Districts
    {
        debug_menu::hide();
        os_developer_options::instance()->set_flag(mString { "SHOW_RESOURCE_SPAM" }, a1->get_bval());

        if (a1->get_bval()) {
            os_developer_options::instance()->set_flag(mString { "SHOW_RESOURCE_SPAM" }, true);
        }
        debug_menu_enabled();
        // TODO
        // sub_66C242(&g_game_ptr->field_4C);
        break;
    }

    case 9u: // Show Districts
    {
        debug_menu::hide();
        os_developer_options::instance()->set_flag(mString { "SHOW_MEMORY_INFO" }, a1->get_bval());

        if (a1->get_bval()) {
            os_developer_options::instance()->set_flag(mString { "SHOW_MEMORY_INFO" }, true);
        }
        debug_menu_enabled();
        // TODO
        // sub_66C242(&g_game_ptr->field_4C);
        break;
    }

    case 10u: // Show Districts
    {
        debug_menu::hide();
        os_developer_options::instance()->set_flag(mString { "SHOW_SPIDEY_SPEED" }, a1->get_bval());

        if (a1->get_bval()) {
            os_developer_options::instance()->set_flag(mString { "SHOW_SPIDEY_SPEED" }, true);
        }
        debug_menu_enabled();
        // TODO
        // sub_66C242(&g_game_ptr->field_4C);
        break;
    }

    case 11u: // Show Districts
    {
        debug_menu::hide();
        os_developer_options::instance()->set_flag(mString { "TRACE_MISSION_MANAGER" }, a1->get_bval());

        if (a1->get_bval()) {
            os_developer_options::instance()->set_flag(mString { "TRACE_MISSION_MANAGER" }, true);
        }
        debug_menu_enabled();
        // TODO
        // sub_66C242(&g_game_ptr->field_4C);
        break;
    }

    case 12u: // Show Districts
    {
        debug_menu::hide();
        os_developer_options::instance()->set_flag(mString { "DUMP_MISSION_HEAP" }, a1->get_bval());

        if (a1->get_bval()) {
            os_developer_options::instance()->set_flag(mString { "DUMP_MISSION_HEAP" }, true);
        }
        debug_menu_enabled();
        // TODO
        // sub_66C242(&g_game_ptr->field_4C);
        break;
    }

    case 13u: // Show Districts
    {
        debug_menu::hide();
        os_developer_options::instance()->set_flag(mString { "CAMERA_CENTRIC_STREAMER" }, a1->get_bval());

        if (a1->get_bval()) {
            os_developer_options::instance()->set_flag(mString { "CAMERA_CENTRIC_STREAMER" }, true);
        }
        debug_menu_enabled();
        // TODO
        // sub_66C242(&g_game_ptr->field_4C);
        break;
    }

    case 14u: // Show Districts
    {
        debug_menu::hide();
        os_developer_options::instance()->set_flag(mString { "RENDER_LOWLODS" }, a1->get_bval());

        if (a1->get_bval()) {
            os_developer_options::instance()->set_flag(mString { "RENDER_LOWLODS" }, true);
        }
        debug_menu_enabled();
        // TODO
        // sub_66C242(&g_game_ptr->field_4C);
        break;
    }

    case 15u: // Show Districts
    {
        debug_menu::hide();
        os_developer_options::instance()->set_flag(mString { "LOAD_STRING_HASH_DICTIONARY" }, a1->get_bval());

        if (a1->get_bval()) {
            os_developer_options::instance()->set_flag(mString { "LOAD_STRING_HASH_DICTIONARY" }, true);
        }
        debug_menu_enabled();
        // TODO
        // sub_66C242(&g_game_ptr->field_4C);
        break;
    }

    case 16u: // Show Districts
    {
        debug_menu::hide();
        os_developer_options::instance()->set_flag(mString { "LOG_RUNTIME_STRING_HASHES" }, a1->get_bval());

        if (a1->get_bval()) {
            os_developer_options::instance()->set_flag(mString { "LOG_RUNTIME_STRING_HASHES" }, true);
        }
        debug_menu_enabled();
        // TODO
        // sub_66C242(&g_game_ptr->field_4C);
        break;
    }

    case 17u: // Show Districts
    {
        debug_menu::hide();
        os_developer_options::instance()->set_flag(mString { "SHOW_WATERMARK_VELOCITY" }, a1->get_bval());

        if (a1->get_bval()) {
            os_developer_options::instance()->set_flag(mString { "SHOW_WATERMARK_VELOCITY" }, true);
        }
        debug_menu_enabled();
        // TODO
        // sub_66C242(&g_game_ptr->field_4C);
        break;
    }

    case 18u: // Show Districts
    {
        debug_menu::hide();
        os_developer_options::instance()->set_flag(mString { "SHOW_STATS" }, a1->get_bval());

        if (a1->get_bval()) {
            os_developer_options::instance()->set_flag(mString { "SHOW_STATS" }, true);
        }
        debug_menu_enabled();
        // TODO
        // sub_66C242(&g_game_ptr->field_4C);
        break;
    }

    case 19u: // Show Districts
    {
        debug_menu::hide();
        os_developer_options::instance()->set_flag(mString { "ENABLE_ZOOM_MAP" }, a1->get_bval());

        if (a1->get_bval()) {
            os_developer_options::instance()->set_flag(mString { "ENABLE_ZOOM_MAP" }, true);
        }
        debug_menu_enabled();
        // TODO
        // sub_66C242(&g_game_ptr->field_4C);
        break;
    }

    case 20u: // Show Districts
    {
        debug_menu::hide();
        os_developer_options::instance()->set_flag(mString { "SHOW_DEBUG_INFO" }, a1->get_bval());

        if (a1->get_bval()) {
            os_developer_options::instance()->set_flag(mString { "SHOW_DEBUG_INFO" }, false);
        }
        debug_menu_enabled();
        // TODO
        // sub_66C242(&g_game_ptr->field_4C);
        break;
    }

    case 21u: // Show Districts
    {
        debug_menu::hide();
        os_developer_options::instance()->set_flag(mString { "SHOW_PROFILE_INFO" }, a1->get_bval());

        if (a1->get_bval()) {
            os_developer_options::instance()->set_flag(mString { "SHOW_PROFILE_INFO" }, true);
        }
        debug_menu_enabled();
        // TODO
        // sub_66C242(&g_game_ptr->field_4C);
        break;
    }

    case 22u: // Show Districts
    {
        debug_menu::hide();
        os_developer_options::instance()->set_flag(mString { "SHOW_LOCOMOTION_INFO" }, a1->get_bval());

        if (a1->get_bval()) {
            os_developer_options::instance()->set_flag(mString { "SHOW_LOCOMOTION_INFO" }, true);
        }
        debug_menu_enabled();
        // TODO
        // sub_66C242(&g_game_ptr->field_4C);
        break;
    }

    case 23u: // Show Districts
    {
        debug_menu::hide();
        os_developer_options::instance()->set_flag(mString { "GRAVITY" }, a1->get_bval());

        if (a1->get_bval()) {
            os_developer_options::instance()->set_flag(mString { "GRAVITY" }, true);
        }
        debug_menu_enabled();
        // TODO
        // sub_66C242(&g_game_ptr->field_4C);
        break;
    }

    case 24u: // Show Districts
    {
        debug_menu::hide();
        os_developer_options::instance()->set_flag(mString { "TEST_MEMORY_LEAKS" }, a1->get_bval());

        if (a1->get_bval()) {
            os_developer_options::instance()->set_flag(mString { "TEST_MEMORY_LEAKS" }, true);
        }
        debug_menu_enabled();
        // TODO
        // sub_66C242(&g_game_ptr->field_4C);
        break;
    }

    case 25u: // Show Districts
    {
        debug_menu::hide();
        os_developer_options::instance()->set_flag(mString { "HALT_ON_ASSERTS" }, a1->get_bval());

        if (a1->get_bval()) {
            os_developer_options::instance()->set_flag(mString { "HALT_ON_ASSERTS" }, true);
        }
        debug_menu_enabled();
        // TODO
        // sub_66C242(&g_game_ptr->field_4C);
        break;
    }

    case 26u: // Show Districts
    {
        debug_menu::hide();
        os_developer_options::instance()->set_flag(mString { "SCREEN_ASSERTS" }, a1->get_bval());

        if (a1->get_bval()) {
            os_developer_options::instance()->set_flag(mString { "SCREEN_ASSERTS" }, true);
        }
        debug_menu_enabled();
        // TODO
        // sub_66C242(&g_game_ptr->field_4C);
        break;
    }

    case 27u: // Show Districts
    {
        debug_menu::hide();
        os_developer_options::instance()->set_flag(mString { "NO_ANIM_WARNINGS" }, a1->get_bval());

        if (a1->get_bval()) {
            os_developer_options::instance()->set_flag(mString { "NO_ANIM_WARNINGS" }, true);
        }
        debug_menu_enabled();
        // TODO
        // sub_66C242(&g_game_ptr->field_4C);
        break;
    }

    case 28u: // Show Districts
    {
        debug_menu::hide();
        os_developer_options::instance()->set_flag(mString { "PROFILING_ON" }, a1->get_bval());

        if (a1->get_bval()) {
            os_developer_options::instance()->set_flag(mString { "PROFILING_ON" }, true);
        }
        debug_menu_enabled();
        // TODO
        // sub_66C242(&g_game_ptr->field_4C);
        break;
    }

    case 29u: // Show Districts
    {
        debug_menu::hide();
        os_developer_options::instance()->set_flag(mString { "MONO_AUDIO" }, a1->get_bval());

        if (a1->get_bval()) {
            os_developer_options::instance()->set_flag(mString { "MONO_AUDIO" }, true);
        }
        debug_menu_enabled();
        // TODO
        // sub_66C242(&g_game_ptr->field_4C);
        break;
    }

    case 30u: // Show Districts
    {
        debug_menu::hide();
        os_developer_options::instance()->set_flag(mString { "NO_MESSAGES" }, a1->get_bval());

        if (a1->get_bval()) {
            os_developer_options::instance()->set_flag(mString { "NO_MESSAGES" }, true);
        }
        debug_menu_enabled();
        // TODO
        // sub_66C242(&g_game_ptr->field_4C);
        break;
    }

    case 31u: // Show Districts
    {
        debug_menu::hide();
        os_developer_options::instance()->set_flag(mString { "LOCK_STEP" }, a1->get_bval());

        if (a1->get_bval()) {
            os_developer_options::instance()->set_flag(mString { "LOCK_STEP" }, true);
        }
        debug_menu_enabled();
        // TODO
        // sub_66C242(&g_game_ptr->field_4C);
        break;
    }

    case 32u: // Show Districts
    {
        debug_menu::hide();
        os_developer_options::instance()->set_flag(mString { "TEXTURE_DUMP" }, a1->get_bval());

        if (a1->get_bval()) {
            os_developer_options::instance()->set_flag(mString { "TEXTURE_DUMP" }, true);
        }
        debug_menu_enabled();
        // TODO
        // sub_66C242(&g_game_ptr->field_4C);
        break;
    }

    case 33u: // Show Districts
    {
        debug_menu::hide();
        os_developer_options::instance()->set_flag(mString { "DISABLE_SOUND_WARNINGS" }, a1->get_bval());

        if (a1->get_bval()) {
            os_developer_options::instance()->set_flag(mString { "DISABLE_SOUND_WARNINGS" }, true);
        }
        debug_menu_enabled();
        // TODO
        // sub_66C242(&g_game_ptr->field_4C);
        break;
    }

    case 34u: // Show Districts
    {
        debug_menu::hide();
        os_developer_options::instance()->set_flag(mString { "DISABLE_SOUND_DEBUG_OUTPUT" }, a1->get_bval());

        if (a1->get_bval()) {
            os_developer_options::instance()->set_flag(mString { "DISABLE_SOUND_DEBUG_OUTPUT" }, true);
        }
        debug_menu_enabled();
        // TODO
        // sub_66C242(&g_game_ptr->field_4C);
        break;
    }

    case 35u: // Show Districts
    {
        debug_menu::hide();
        os_developer_options::instance()->set_flag(mString { "DELETE_UNUSED_SOUND_BANKS_ON_PACK" }, a1->get_bval());

        if (a1->get_bval()) {
            os_developer_options::instance()->set_flag(mString { "DELETE_UNUSED_SOUND_BANKS_ON_PACK" }, true);
        }
        debug_menu_enabled();
        // TODO
        // sub_66C242(&g_game_ptr->field_4C);
        break;
    }

    case 36u: // Show Districts
    {
        debug_menu::hide();
        os_developer_options::instance()->set_flag(mString { "LOCKED_HERO" }, a1->get_bval());

        if (a1->get_bval()) {
            os_developer_options::instance()->set_flag(mString { "LOCKED_HERO" }, true);
        }
        debug_menu_enabled();
        // TODO
        // sub_66C242(&g_game_ptr->field_4C);
        break;
    }

    case 37u: // Show Districts
    {
        debug_menu::hide();
        os_developer_options::instance()->set_flag(mString { "FOG_OVERR_IDE" }, a1->get_bval());

        if (a1->get_bval()) {
            os_developer_options::instance()->set_flag(mString { "FOG_OVERR IDE" }, true);
        }
        debug_menu_enabled();
        // TODO
        // sub_66C242(&g_game_ptr->field_4C);
        break;
    }

    case 38u: // Show Districts
    {
        debug_menu::hide();
        os_developer_options::instance()->set_flag(mString { "FOG_DISABLE" }, a1->get_bval());

        if (a1->get_bval()) {
            os_developer_options::instance()->set_flag(mString { "FOG_DISABLE" }, true);
        }
        debug_menu_enabled();
        // TODO
        // sub_66C242(&g_game_ptr->field_4C);
        break;
    }

    case 39u: // Show Districts
    {
        debug_menu::hide();
        os_developer_options::instance()->set_flag(mString { "MOVE_EDITOR" }, a1->get_bval());

        if (a1->get_bval()) {
            os_developer_options::instance()->set_flag(mString { "MOVE_EDITOR" }, true);
        }
        debug_menu_enabled();
        // TODO
        // sub_66C242(&g_game_ptr->field_4C);
        break;
    }

    case 40u: // Show Districts
    {
        debug_menu::hide();
        os_developer_options::instance()->set_flag(mString { "AI_PATH_DEBUG" }, a1->get_bval());

        if (a1->get_bval()) {
            os_developer_options::instance()->set_flag(mString { "AI_PATH_DEBUG" }, true);
        }
        debug_menu_enabled();
        // TODO
        // sub_66C242(&g_game_ptr->field_4C);
        break;
    }

    case 41u: // Show Districts
    {
        debug_menu::hide();
        os_developer_options::instance()->set_flag(mString { "AI_PATH_COLOR" }, a1->get_bval());

        if (a1->get_bval()) {
            os_developer_options::instance()->set_flag(mString { "AI_PATH_COLOR" }, true);
        }
        debug_menu_enabled();
        // TODO
        // sub_66C242(&g_game_ptr->field_4C);
        break;
    }
    case 42u: // Show Districts
    {
        debug_menu::hide();
        os_developer_options::instance()->set_flag(mString { "AI_CRITTER_ACTIVITY" }, a1->get_bval());

        if (a1->get_bval()) {
            os_developer_options::instance()->set_flag(mString { "AI_CRITTER_ACTIVITY" }, true);
        }
        debug_menu_enabled();
        // TODO
        // sub_66C242(&g_game_ptr->field_4C);
        break;
    }

    case 43u: // Show Districts
    {
        debug_menu::hide();
        os_developer_options::instance()->set_flag(mString { "AI_PATH_COLOR_CRITTER" }, a1->get_bval());

        if (a1->get_bval()) {
            os_developer_options::instance()->set_flag(mString { "AI_PATH_COLOR_CRITTER" }, true);
        }
        debug_menu_enabled();
        // TODO
        // sub_66C242(&g_game_ptr->field_4C);
        break;
    }
    case 44u: // Show Districts
    {
        debug_menu::hide();
        os_developer_options::instance()->set_flag(mString { "AI_PATH_COLOR_HERO" }, a1->get_bval());

        if (a1->get_bval()) {
            os_developer_options::instance()->set_flag(mString { "AI_PATH_COLOR_HERO" }, true);
        }
        debug_menu_enabled();
        // TODO
        // sub_66C242(&g_game_ptr->field_4C);
        break;
    }

    case 45u: // Show Districts
    {
        debug_menu::hide();
        os_developer_options::instance()->set_flag(mString { "NO_PARTICLES" }, a1->get_bval());

        if (a1->get_bval()) {
            os_developer_options::instance()->set_flag(mString { "NO_PARTICLES" }, true);
        }
        debug_menu_enabled();
        // TODO
        // sub_66C242(&g_game_ptr->field_4C);
        break;
    }
    case 46u: // Show Districts
    {
        debug_menu::hide();
        os_developer_options::instance()->set_flag(mString { "NO_PARTICLE_PUMP" }, a1->get_bval());

        if (a1->get_bval()) {
            os_developer_options::instance()->set_flag(mString { "NO_PARTICLE_PUMP" }, true);
        }
        debug_menu_enabled();
        // TODO
        // sub_66C242(&g_game_ptr->field_4C);
        break;
    }
    case 47u: // Show Districts
    {
        debug_menu::hide();
        os_developer_options::instance()->set_flag(mString { "SHOW_SHADOW_BALL" }, a1->get_bval());

        if (a1->get_bval()) {
            os_developer_options::instance()->set_flag(mString { "SHOW_SHADOW_BALL" }, true);
        }
        debug_menu_enabled();
        // TODO
        // sub_66C242(&g_game_ptr->field_4C);
        break;
    }

    case 48u: // Show Districts
    {
        debug_menu::hide();
        os_developer_options::instance()->set_flag(mString { "SHOW_NORMALS" }, a1->get_bval());

        if (a1->get_bval()) {
            os_developer_options::instance()->set_flag(mString { "SHOW_NORMALS" }, true);
        }
        debug_menu_enabled();
        // TODO
        // sub_66C242(&g_game_ptr->field_4C);
        break;
    }
    case 49u: // Show Districts
    {
        debug_menu::hide();
        os_developer_options::instance()->set_flag(mString { "SHOW_LIGHTS" }, a1->get_bval());

        if (a1->get_bval()) {
            os_developer_options::instance()->set_flag(mString { "SHOW_LIGHTS" }, true);
        }
        debug_menu_enabled();
        // TODO
        // sub_66C242(&g_game_ptr->field_4C);
        break;
    }

    case 50u: // Show Districts
    {
        debug_menu::hide();
        os_developer_options::instance()->set_flag(mString { "SHOW_PLRCTRL" }, a1->get_bval());

        if (a1->get_bval()) {
            os_developer_options::instance()->set_flag(mString { "SHOW_PLRCTRL" }, true);
        }
        debug_menu_enabled();
        // TODO
        // sub_66C242(&g_game_ptr->field_4C);
        break;
    }
    case 51u: // Show Districts
    {
        debug_menu::hide();
        os_developer_options::instance()->set_flag(mString { "SHOW_PSX_INFO" }, a1->get_bval());

        if (a1->get_bval()) {
            os_developer_options::instance()->set_flag(mString { "SHOW_PSX_INFO" }, true);
        }
        debug_menu_enabled();
        // TODO
        // sub_66C242(&g_game_ptr->field_4C);
        break;
    }

    case 52u: // Show Districts
    {
        debug_menu::hide();
        os_developer_options::instance()->set_flag(mString { "FLAT_SHADE" }, a1->get_bval());

        if (a1->get_bval()) {
            os_developer_options::instance()->set_flag(mString { "FLAT_SHADE" }, true);
        }
        debug_menu_enabled();
        // TODO
        // sub_66C242(&g_game_ptr->field_4C);
        break;
    }
    case 53u: // Show Districts
    {
        debug_menu::hide();
        os_developer_options::instance()->set_flag(mString { "INTERFACE_DISABLE" }, a1->get_bval());

        if (a1->get_bval()) {
            os_developer_options::instance()->set_flag(mString { "INTERFACE_DISABLE" }, true);
        }
        debug_menu_enabled();
        // TODO
        // sub_66C242(&g_game_ptr->field_4C);
        break;
    }

    case 54u: // Show Districts
    {
        debug_menu::hide();
        os_developer_options::instance()->set_flag(mString { "WIDGET_TOOLS" }, a1->get_bval());

        if (a1->get_bval()) {
            os_developer_options::instance()->set_flag(mString { "WIDGET_TOOLS" }, true);
        }
        debug_menu_enabled();
        // TODO
        // sub_66C242(&g_game_ptr->field_4C);
        break;
    }
    case 55u: // Show Districts
    {
        debug_menu::hide();
        os_developer_options::instance()->set_flag(mString { "LIGHTING" }, a1->get_bval());

        if (a1->get_bval()) {
            os_developer_options::instance()->set_flag(mString { "LIGHTING" }, true);
        }
        debug_menu_enabled();
        // TODO
        // sub_66C242(&g_game_ptr->field_4C);
        break;
    }

    case 56u: // Show Districts
    {
        debug_menu::hide();
        os_developer_options::instance()->set_flag(mString { "FAKE_POINT_LIGHTS" }, a1->get_bval());

        if (a1->get_bval()) {
            os_developer_options::instance()->set_flag(mString { "FAKE_POINT_LIGHTS" }, true);
        }
        debug_menu_enabled();
        // TODO
        // sub_66C242(&g_game_ptr->field_4C);
        break;
    }
    case 57u: // Show Districts
    {
        debug_menu::hide();
        os_developer_options::instance()->set_flag(mString { "BSP_SPRAY_PAINT" }, a1->get_bval());

        if (a1->get_bval()) {
            os_developer_options::instance()->set_flag(mString { "BSP_SPRAY_PAINT" }, true);
        }
        debug_menu_enabled();
        // TODO
        // sub_66C242(&g_game_ptr->field_4C);
        break;
    }

    case 58u: // Show Districts
    {
        debug_menu::hide();
        os_developer_options::instance()->set_flag(mString { "CAMERA_EDITOR" }, a1->get_bval());

        if (a1->get_bval()) {
            os_developer_options::instance()->set_flag(mString { "CAMERA_EDITOR" }, true);
        }
        debug_menu_enabled();
        // TODO
        // sub_66C242(&g_game_ptr->field_4C);
        break;
    }
    case 59u: // Show Districts
    {
        debug_menu::hide();
        os_developer_options::instance()->set_flag(mString { "IGNORE_RAMPING" }, a1->get_bval());

        if (a1->get_bval()) {
            os_developer_options::instance()->set_flag(mString { "IGNORE_RAMPING" }, true);
        }
        debug_menu_enabled();
        // TODO
        // sub_66C242(&g_game_ptr->field_4C);
        break;
    }

    case 60u: // Show Districts
    {
        debug_menu::hide();
        os_developer_options::instance()->set_flag(mString { "POINT_SAMPLE" }, a1->get_bval());

        if (a1->get_bval()) {
            os_developer_options::instance()->set_flag(mString { "POINT_SAMPLE" }, true);
        }
        debug_menu_enabled();
        // TODO
        // sub_66C242(&g_game_ptr->field_4C);
        break;
    }
    case 61u: // Show Districts
    {
        debug_menu::hide();
        os_developer_options::instance()->set_flag(mString { "DISTANCE_FADING" }, a1->get_bval());

        if (a1->get_bval()) {
            os_developer_options::instance()->set_flag(mString { "DISTANCE_FADING" }, true);
        }
        debug_menu_enabled();
        // TODO
        // sub_66C242(&g_game_ptr->field_4C);
        break;
    }

    case 62u: // Show Districts
    {
        debug_menu::hide();
        os_developer_options::instance()->set_flag(mString { "OVERRIDE_CONTROLLER_OPTIONS" }, a1->get_bval());

        if (a1->get_bval()) {
            os_developer_options::instance()->set_flag(mString { "OVERRIDE_CONTROLLER_OPTIONS" }, true);
        }
        debug_menu_enabled();
        // TODO
        // sub_66C242(&g_game_ptr->field_4C);
        break;
    }
    case 63u: // Show Districts
    {
        debug_menu::hide();
        os_developer_options::instance()->set_flag(mString { "DISABLE_MOUSE_PLAYER_CONTROL" }, a1->get_bval());

        if (a1->get_bval()) {
            os_developer_options::instance()->set_flag(mString { "DISABLE_MOUSE_PLAYER_CONTROL" }, true);
        }
        debug_menu_enabled();
        // TODO
        // sub_66C242(&g_game_ptr->field_4C);
        break;
    }

    case 64u: // Show Districts
    {
        debug_menu::hide();
        os_developer_options::instance()->set_flag(mString { "NO_MOVIES" }, a1->get_bval());

        if (a1->get_bval()) {
            os_developer_options::instance()->set_flag(mString { "NO_MOVIES" }, true);
        }
        debug_menu_enabled();
        // TODO
        // sub_66C242(&g_game_ptr->field_4C);
        break;
    }
    case 65u: // Show Districts
    {
        debug_menu::hide();
        os_developer_options::instance()->set_flag(mString { "XBOX_USER_CAM" }, a1->get_bval());

        if (a1->get_bval()) {
            os_developer_options::instance()->set_flag(mString { "XBOX_USER_CAM" }, true);
        }
        debug_menu_enabled();
        // TODO
        // sub_66C242(&g_game_ptr->field_4C);
        break;
    }

    case 66u: // Show Districts
    {
        debug_menu::hide();
        os_developer_options::instance()->set_flag(mString { "NO_LOAD_SCREEN" }, a1->get_bval());

        if (a1->get_bval()) {
            os_developer_options::instance()->set_flag(mString { "NO_LOAD_SCREEN" }, true);
        }
        debug_menu_enabled();
        // TODO
        // sub_66C242(&g_game_ptr->field_4C);
        break;
    }
    case 67u: // Show Districts
    {
        debug_menu::hide();
        os_developer_options::instance()->set_flag(mString { "EXCEPTION_HANDLER" }, a1->get_bval());

        if (a1->get_bval()) {
            os_developer_options::instance()->set_flag(mString { "EXCEPTION_HANDLER" }, true);
        }
        debug_menu_enabled();
        // TODO
        // sub_66C242(&g_game_ptr->field_4C);
        break;
    }

    case 68u: // Show Districts
    {
        debug_menu::hide();
        os_developer_options::instance()->set_flag(mString { "NO_EXCEPTION_HANDLER" }, a1->get_bval());

        if (a1->get_bval()) {
            os_developer_options::instance()->set_flag(mString { "NO_EXCEPTION_HANDLER" }, true);
        }
        debug_menu_enabled();
        // TODO
        // sub_66C242(&g_game_ptr->field_4C);
        break;
    }
    case 69u: // Show Districts
    {
        debug_menu::hide();
        os_developer_options::instance()->set_flag(mString { "NO_CD_CHECK" }, a1->get_bval());

        if (a1->get_bval()) {
            os_developer_options::instance()->set_flag(mString { "NO_CD_CHECK" }, true);
        }
        debug_menu_enabled();
        // TODO
        // sub_66C242(&g_game_ptr->field_4C);
        break;
    }

    case 70u: // Show Districts
    {
        debug_menu::hide();
        os_developer_options::instance()->set_flag(mString { "NO_LOAD_METER" }, a1->get_bval());

        if (a1->get_bval()) {
            os_developer_options::instance()->set_flag(mString { "NO_LOAD_METER" }, true);
        }
        debug_menu_enabled();
        // TODO
        // sub_66C242(&g_game_ptr->field_4C);
        break;
    }
    case 71u: // Show Districts
    {
        debug_menu::hide();
        os_developer_options::instance()->set_flag(mString { "NO_MOVIE_BUFFER_WARNING" }, a1->get_bval());

        if (a1->get_bval()) {
            os_developer_options::instance()->set_flag(mString { "NO_MOVIE_BUFFER_WARNING" }, true);
        }
        debug_menu_enabled();
        // TODO
        // sub_66C242(&g_game_ptr->field_4C);
        break;
    }

    case 72u: // Show Districts
    {
        debug_menu::hide();
        os_developer_options::instance()->set_flag(mString { "LIMITED_EDITION_DISC" }, a1->get_bval());

        if (a1->get_bval()) {
            os_developer_options::instance()->set_flag(mString { "LIMITED_EDITION_DISC" }, true);
        }
        debug_menu_enabled();
        // TODO
        // sub_66C242(&g_game_ptr->field_4C);
        break;
    }
    case 73u: // Show Districts
    {
        debug_menu::hide();
        os_developer_options::instance()->set_flag(mString { "NEW_COMBAT_LOCO" }, a1->get_bval());

        if (a1->get_bval()) {
            os_developer_options::instance()->set_flag(mString { "NEW_COMBAT_LOCO" }, true);
        }
        debug_menu_enabled();
        // TODO
        // sub_66C242(&g_game_ptr->field_4C);
        break;
    }

    case 74u: // Show Districts
    {
        debug_menu::hide();
        os_developer_options::instance()->set_flag(mString { "LEVEL_WARP" }, a1->get_bval());

        if (a1->get_bval()) {
            os_developer_options::instance()->set_flag(mString { "LEVEL_WARP" }, true);
        }
        debug_menu_enabled();
        // TODO
        // sub_66C242(&g_game_ptr->field_4C);
        break;
    }
    case 75u: // Show Districts
    {
        debug_menu::hide();
        os_developer_options::instance()->set_flag(mString { "SMOKE_TEST" }, a1->get_bval());

        if (a1->get_bval()) {
            os_developer_options::instance()->set_flag(mString { "SMOKE_TEST" }, true);
        }
        debug_menu_enabled();
        // TODO
        // sub_66C242(&g_game_ptr->field_4C);
        break;
    }

    case 76u: // Show Districts
    {
        debug_menu::hide();
        os_developer_options::instance()->set_flag(mString { "SMOKE_TEST_LEVEL" }, a1->get_bval());

        if (a1->get_bval()) {
            os_developer_options::instance()->set_flag(mString { "SMOKE_TEST_LEVEL" }, true);
        }
        debug_menu_enabled();
        // TODO
        // sub_66C242(&g_game_ptr->field_4C);
        break;
    }
    case 77u: // Show Districts
    {
        debug_menu::hide();
        os_developer_options::instance()->set_flag(mString { "COMBO_TESTER" }, a1->get_bval());

        if (a1->get_bval()) {
            os_developer_options::instance()->set_flag(mString { "COMBO_TESTER" }, true);
        }
        debug_menu_enabled();
        // TODO
        // sub_66C242(&g_game_ptr->field_4C);
        break;
    }

    case 78u: // Show Districts
    {
        debug_menu::hide();
        os_developer_options::instance()->set_flag(mString { "DROP_SHADOWS_ALWAYS_RAYCAST" }, a1->get_bval());

        if (a1->get_bval()) {
            os_developer_options::instance()->set_flag(mString { "DROP_SHADOWS_ALWAYS_RAYCAST" }, true);
        }
        debug_menu_enabled();
        // TODO
        // sub_66C242(&g_game_ptr->field_4C);
        break;
    }
    case 79u: // Show Districts
    {
        debug_menu::hide();
        os_developer_options::instance()->set_flag(mString { "DISABLE_DROP_SHADOWS" }, a1->get_bval());

        if (a1->get_bval()) {
            os_developer_options::instance()->set_flag(mString { "DISABLE_DROP_SHADOWS" }, true);
        }
        debug_menu_enabled();
        // TODO
        // sub_66C242(&g_game_ptr->field_4C);
        break;
    }

    case 80u: // Show Districts
    {
        debug_menu::hide();
        os_developer_options::instance()->set_flag(mString { "DISABLE_HIRES_SHADOWS" }, a1->get_bval());

        if (a1->get_bval()) {
            os_developer_options::instance()->set_flag(mString { "DISABLE_HIRES_SHADOWS" }, true);
        }
        debug_menu_enabled();
        // TODO
        // sub_66C242(&g_game_ptr->field_4C);
        break;
    }

    case 81u: // Show Districts
    {
        debug_menu::hide();
        os_developer_options::instance()->set_flag(mString { "DISABLE_STENCIL_SHADOWS" }, a1->get_bval());

        if (a1->get_bval()) {
            os_developer_options::instance()->set_flag(mString { "DISABLE_STENCIL_SHADOWS" }, true);
        }
        debug_menu_enabled();
        // TODO
        // sub_66C242(&g_game_ptr->field_4C);
        break;
    }

    case 82u: // Show Districts
    {
        debug_menu::hide();
        os_developer_options::instance()->set_flag(mString { "DISABLE_COLORVOLS" }, a1->get_bval());

        if (a1->get_bval()) {
            os_developer_options::instance()->set_flag(mString { "DISABLE_COLORVOLS" }, true);
        }
        debug_menu_enabled();
        // TODO
        // sub_66C242(&g_game_ptr->field_4C);
        break;
    }
    case 83u: // Show Districts
    {
        debug_menu::hide();
        os_developer_options::instance()->set_flag(mString { "DISABLE_RENDER_ENTS" }, a1->get_bval());

        if (a1->get_bval()) {
            os_developer_options::instance()->set_flag(mString { "DISABLE_RENDER_ENTS" }, true);
        }
        debug_menu_enabled();
        // TODO
        // sub_66C242(&g_game_ptr->field_4C);
        break;
    }

    case 84u: // Show Districts
    {
        debug_menu::hide();
        os_developer_options::instance()->set_flag(mString { "DISABLE_FULLSCREEN_BLUR" }, a1->get_bval());

        if (a1->get_bval()) {
            os_developer_options::instance()->set_flag(mString { "DISABLE_FULLSCREEN_BLUR" }, true);
        }
        debug_menu_enabled();
        // TODO
        // sub_66C242(&g_game_ptr->field_4C);
        break;
    }
    case 85u: // Show Districts
    {
        debug_menu::hide();
        os_developer_options::instance()->set_flag(mString { "FORCE_TIGHTCRAWL" }, a1->get_bval());

        if (a1->get_bval()) {
            os_developer_options::instance()->set_flag(mString { "FORCE_TIGHTCRAWL" }, true);
        }
        debug_menu_enabled();
        // TODO
        // sub_66C242(&g_game_ptr->field_4C);
        break;
    }
    case 86u: // Show Districts
    {
        debug_menu::hide();
        os_developer_options::instance()->set_flag(mString { "FORCE_NONCRAWL" }, a1->get_bval());

        if (a1->get_bval()) {
            os_developer_options::instance()->set_flag(mString { "FORCE_NONCRAWL" }, true);
        }
        debug_menu_enabled();
        // TODO
        // sub_66C242(&g_game_ptr->field_4C);
        break;
    }
    case 87u: // Show Districts
    {
        debug_menu::hide();
        os_developer_options::instance()->set_flag(mString { "SHOW_DEBUG_TEXT" }, a1->get_bval());

        if (a1->get_bval()) {
            os_developer_options::instance()->set_flag(mString { "SHOW_DEBUG_TEXT" }, true);
        }
        debug_menu_enabled();
        // TODO
        // sub_66C242(&g_game_ptr->field_4C);
        break;
    }
    case 88u: // Show Districts
    {
        debug_menu::hide();
        os_developer_options::instance()->set_flag(mString { "SHOW_STYLE_POINTS" }, a1->get_bval());

        if (a1->get_bval()) {
            os_developer_options::instance()->set_flag(mString { "SHOW_STYLE_POINTS" }, true);
        }
        debug_menu_enabled();
        // TODO
        // sub_66C242(&g_game_ptr->field_4C);
        break;
    }
    case 89u: // Show Districts
    {
        debug_menu::hide();
        os_developer_options::instance()->set_flag(mString { "CAMERA_MOUSE_MODE" }, a1->get_bval());

        if (a1->get_bval()) {
            os_developer_options::instance()->set_flag(mString { "CAMERA_MOUSE_MODE" }, true);
        }
        debug_menu_enabled();
        // TODO
        // sub_66C242(&g_game_ptr->field_4C);
        break;
    }
    case 90u: // Show Districts
    {
        debug_menu::hide();
        os_developer_options::instance()->set_flag(mString { "USERCAM_ON_CONTROLLER2" }, a1->get_bval());

        if (a1->get_bval()) {
            os_developer_options::instance()->set_flag(mString { "USERCAM_ON_CONTROLLER2" }, true);
        }
        debug_menu_enabled();
        // TODO
        // sub_66C242(&g_game_ptr->field_4C);
        break;
    }
    case 91u: // Show Districts
    {
        debug_menu::hide();
        os_developer_options::instance()->set_flag(mString { "DISABLE_ANCHOR_RETICLE_RENDERING" }, a1->get_bval());

        if (a1->get_bval()) {
            os_developer_options::instance()->set_flag(mString { "DISABLE_ANCHOR_RETICLE_RENDERING" }, true);
        }
        debug_menu_enabled();
        // TODO
        // sub_66C242(&g_game_ptr->field_4C);
        break;
    }
    case 92u: // Show Districts
    {
        debug_menu::hide();
        os_developer_options::instance()->set_flag(mString { "SHOW_ANCHOR_LINE" }, a1->get_bval());

        if (a1->get_bval()) {
            os_developer_options::instance()->set_flag(mString { "SHOW_ANCHOR_LINE" }, true);
        }
        debug_menu_enabled();
        // TODO
        // sub_66C242(&g_game_ptr->field_4C);
        break;
    }
    case 93u: // Show Districts
    {
        debug_menu::hide();
        os_developer_options::instance()->set_flag(mString { "FREE_SPIDER_REFLEXES" }, a1->get_bval());

        if (a1->get_bval()) {
            os_developer_options::instance()->set_flag(mString { "FREE_SPIDER_REFLEXES" }, true);
        }
        debug_menu_enabled();
        // TODO
        // sub_66C242(&g_game_ptr->field_4C);
        break;
    }
    case 94u: // Show Districts
    {
        debug_menu::hide();
        switch (a1->get_bval()) {
        case 0u:
            os_developer_options::instance()->set_flag(mString { "SHOW_BAR_OF_SHAME" }, false);
            break;
        case 1u:
            os_developer_options::instance()->set_flag(mString { "SHOW_BAR_OF_SHAME" }, true);
            break;
        }
        debug_menu_enabled();
        // TODO
        // sub_66C242(&g_game_ptr->field_4C);
        break;
    }
    case 95u: // Show Districts
    {
        debug_menu::hide();
        os_developer_options::instance()->set_flag(mString { "SHOW_ENEMY_HEALTH_WIDGETS" }, a1->get_bval());

        if (a1->get_bval()) {
            os_developer_options::instance()->set_flag(mString { "SHOW_ENEMY_HEALTH_WIDGETS" }, true);
        }
        debug_menu_enabled();
        // TODO
        // sub_66C242(&g_game_ptr->field_4C);
        break;
    }

    case 96u: // Show Districts
    {
        debug_menu::hide();
        os_developer_options::instance()->set_flag(mString { "ALLOW_IGC_PAUSE" }, a1->get_bval());

        if (a1->get_bval()) {
            os_developer_options::instance()->set_flag(mString { "ALLOW_IGC_PAUSE" }, true);
        }
        debug_menu_enabled();
        // TODO
        // sub_66C242(&g_game_ptr->field_4C);
        break;
    }

    case 97u: // Show Districts
    {
        debug_menu::hide();
        os_developer_options::instance()->set_flag(mString { "SHOW_OBBS" }, a1->get_bval());

        if (a1->get_bval()) {
            os_developer_options::instance()->set_flag(mString { "SHOW_OBBS" }, true);
        }
        debug_menu_enabled();
        // TODO
        // sub_66C242(&g_game_ptr->field_4C);
        break;
    }

    case 98u: // Show Districts
    {
        debug_menu::hide();
        os_developer_options::instance()->set_flag(mString { "SHOW_DISTRICTS" }, a1->get_bval());

        if (a1->get_bval()) {
            os_developer_options::instance()->set_flag(mString { "SHOW_DISTRICTS" }, true);
        }
        debug_menu_enabled();
        // TODO
        // sub_66C242(&g_game_ptr->field_4C);
        break;
    }

    case 99u: // Show Districts
    {
        debug_menu::hide();
        os_developer_options::instance()->set_flag(mString { "SHOW_CHUCK_DEBUGGER" }, a1->get_bval());

        if (a1->get_bval()) {
            os_developer_options::instance()->set_flag(mString { "SHOW_CHUCK_DEBUGGER" }, true);
        }
        debug_menu_enabled();
        // TODO
        // sub_66C242(&g_game_ptr->field_4C);
        break;
    }

    case 100u: // Show Districts
    {
        debug_menu::hide();
        os_developer_options::instance()->set_flag(mString { "CHUCK_OLD_FASHIONED" }, a1->get_bval());

        if (a1->get_bval()) {
            os_developer_options::instance()->set_flag(mString { "CHUCK_OLD_FASHIONED" }, true);
        }
        debug_menu_enabled();
        // TODO
        // sub_66C242(&g_game_ptr->field_4C);
        break;
    }

    case 101u: // Show Districts
    {
        debug_menu::hide();
        os_developer_options::instance()->set_flag(mString { "CHUCK_DISABLE_BREAKPOINTS" }, a1->get_bval());

        if (a1->get_bval()) {
            os_developer_options::instance()->set_flag(mString { "CHUCK_DISABLE_BREAKPOINTS" }, true);
        }
        debug_menu_enabled();
        // TODO
        // sub_66C242(&g_game_ptr->field_4C);
        break;
    }

    case 102u: // Show Districts
    {
        debug_menu::hide();
        os_developer_options::instance()->set_flag(mString { "SHOW_AUDIO_BOXES" }, a1->get_bval());

        if (a1->get_bval()) {
            os_developer_options::instance()->set_flag(mString { "SHOW_AUDIO_BOXES" }, true);
        }
        debug_menu_enabled();
        // TODO
        // sub_66C242(&g_game_ptr->field_4C);
        break;
    }

    case 103u: // Show Districts
    {
        debug_menu::hide();
        os_developer_options::instance()->set_flag(mString { "DISABLE_SOUNDS" }, a1->get_bval());

        if (a1->get_bval()) {
            os_developer_options::instance()->set_flag(mString { "DISABLE_SOUNDS" }, true);
        }
        debug_menu_enabled();
        // TODO
        // sub_66C242(&g_game_ptr->field_4C);
        break;
    }

    case 104u: // Show Districts
    {
        debug_menu::hide();
        os_developer_options::instance()->set_flag(mString { "SHOW_TERRAIN_INFO" }, a1->get_bval());

        if (a1->get_bval()) {
            os_developer_options::instance()->set_flag(mString { "SHOW_TERRAIN_INFO" }, true);
        }
        debug_menu_enabled();
        // TODO
        // sub_66C242(&g_game_ptr->field_4C);
        break;
    }

    case 105u: // Show Districts
    {
        debug_menu::hide();
        os_developer_options::instance()->set_flag(mString { "DISABLE_AUDIO_BOXES" }, a1->get_bval());

        if (a1->get_bval()) {
            os_developer_options::instance()->set_flag(mString { "DISABLE_AUDIO_BOXES" }, true);
        }
        debug_menu_enabled();
        // TODO
        // sub_66C242(&g_game_ptr->field_4C);
        break;
    }

    case 106u: // Show Districts
    {
        debug_menu::hide();
        os_developer_options::instance()->set_flag(mString { "NSL_OLD_FASHIONED" }, a1->get_bval());

        if (a1->get_bval()) {
            os_developer_options::instance()->set_flag(mString { "NSL_OLD_FASHIONED" }, true);
        }
        debug_menu_enabled();
        // TODO
        // sub_66C242(&g_game_ptr->field_4C);
        break;
    }

    case 107u: // Show Districts
    {
        debug_menu::hide();
        os_developer_options::instance()->set_flag(mString { "SHOW_MASTER_CLOCK" }, a1->get_bval());

        if (a1->get_bval()) {
            os_developer_options::instance()->set_flag(mString { "SHOW_MASTER_CLOCK" }, true);
        }
        debug_menu_enabled();
        // TODO
        // sub_66C242(&g_game_ptr->field_4C);
        break;
    }

    case 108u: // Show Districts
    {
        debug_menu::hide();
        os_developer_options::instance()->set_flag(mString { "LOAD_GRADIENTS" }, a1->get_bval());

        if (a1->get_bval()) {
            os_developer_options::instance()->set_flag(mString { "LOAD_GRADIENTS" }, true);
        }
        debug_menu_enabled();
        // TODO
        // sub_66C242(&g_game_ptr->field_4C);
        break;
    }

    case 109u: // Show Districts
    {
        debug_menu::hide();
        os_developer_options::instance()->set_flag(mString { "BONUS_LEVELS_AVAILABLE" }, a1->get_bval());

        if (a1->get_bval()) {
            os_developer_options::instance()->set_flag(mString { "BONUS_LEVELS_AVAILABLE" }, true);
        }
        debug_menu_enabled();
        // TODO
        // sub_66C242(&g_game_ptr->field_4C);
        break;
    }

    case 110u: // Show Districts
    {
        debug_menu::hide();
        os_developer_options::instance()->set_flag(mString { "COMBAT_DISPLAY" }, a1->get_bval());

        if (a1->get_bval()) {
            os_developer_options::instance()->set_flag(mString { "COMBAT_DISPLAY" }, true);
        }
        debug_menu_enabled();
        // TODO
        // sub_66C242(&g_game_ptr->field_4C);
        break;
    }

    case 111u: // Show Districts
    {
        debug_menu::hide();
        os_developer_options::instance()->set_flag(mString { "COMBAT_DEBUGGER" }, a1->get_bval());

        if (a1->get_bval()) {
            os_developer_options::instance()->set_flag(mString { "COMBAT_DEBUGGER" }, true);
        }
        debug_menu_enabled();
        // TODO
        // sub_66C242(&g_game_ptr->field_4C);
        break;
    }

    case 112u: // Show Districts
    {
        debug_menu::hide();
        os_developer_options::instance()->set_flag(mString { "ALLOW_ERROR_POPUPS" }, a1->get_bval());

        if (a1->get_bval()) {
            os_developer_options::instance()->set_flag(mString { "ALLOW_ERROR_POPUPS" }, true);
        }
        debug_menu_enabled();
        // TODO
        // sub_66C242(&g_game_ptr->field_4C);
        break;
    }

    case 113u: // Show Districts
    {
        debug_menu::hide();
        os_developer_options::instance()->set_flag(mString { "ALLOW_WARNING_POPUPS" }, a1->get_bval());

        if (a1->get_bval()) {
            os_developer_options::instance()->set_flag(mString { "ALLOW_WARNING_POPUPS" }, true);
        }
        debug_menu_enabled();
        // TODO
        // sub_66C242(&g_game_ptr->field_4C);
        break;
    }

    case 114u: // Show Districts
    {
        debug_menu::hide();
        os_developer_options::instance()->set_flag(mString { "OUTPUT_WARNING_DISABLE" }, a1->get_bval());

        if (a1->get_bval()) {
            os_developer_options::instance()->set_flag(mString { "OUTPUT_WARNING_DISABLE" }, true);
        }

        debug_menu_enabled();
        // TODO
        // sub_66C242(&g_game_ptr->field_4C);
        break;
    }

    case 115u: // Show Districts
    {
        debug_menu::hide();
        os_developer_options::instance()->set_flag(mString { "OUTPUT_ASSERT_DISABLE" }, a1->get_bval());

        if (a1->get_bval()) {
            os_developer_options::instance()->set_flag(mString { "OUTPUT_ASSERT_DISABLE" }, true);
        }
        debug_menu_enabled();
        // TODO
        // sub_66C242(&g_game_ptr->field_4C);
        break;
    }

    case 116u: // Show Districts
    {
        debug_menu::hide();
        const char* arg0;
        va_list va;
        va_start(va, arg0);
        if ((g_debug().field_0 & debug_struct_t::OUTPUT_WARNING) != 0) {
            va_list v2;
            va_copy(v2, va);

            char a1[2048];
            vsnprintf(a1, 2048, arg0, va);
            if (os_developer_options::instance()->get_flag(mString { "ASSERT_ON_WARNING" })) {
                assert(a1);
            } else {
                debug_print_va(a1);
            }
        }
        debug_menu_enabled();
        // TODO
        // sub_66C242(&g_game_ptr->field_4C);
        break;
    }

    case 117u: // Show Districts
    {
        debug_menu::hide();
        os_developer_options::instance()->set_flag(mString { "ALWAYS_ACTIVE" }, a1->get_bval());

        if (a1->get_bval()) {
            os_developer_options::instance()->set_flag(mString { "ALWAYS_ACTIVE" }, true);
        }
        debug_menu_enabled();
        // TODO
        // sub_66C242(&g_game_ptr->field_4C);
        break;
    }

    case 118u: // Show Districts
    {
        debug_menu::hide();
        os_developer_options::instance()->set_flag(mString { "FORCE_PROGRESSION" }, a1->get_bval());

        if (a1->get_bval()) {
            os_developer_options::instance()->set_flag(mString { "FORCE_PROGRESSION" }, true);
        }
        debug_menu_enabled();
        // TODO
        // sub_66C242(&g_game_ptr->field_4C);
        break;
    }

    case 119u: // Show Districts
    {
        debug_menu::hide();
        os_developer_options::instance()->set_flag(mString { "LINK" }, a1->get_bval());

        if (a1->get_bval()) {
            os_developer_options::instance()->set_flag(mString { "LINK" }, true);
        }
        debug_menu_enabled();
        // TODO
        // sub_66C242(&g_game_ptr->field_4C);
        break;
    }

    case 120u: // Show Districts
    {
        debug_menu::hide();
        os_developer_options::instance()->set_flag(mString { "WAIT_FOR_LINK" }, a1->get_bval());

        if (a1->get_bval()) {
            os_developer_options::instance()->set_flag(mString { "WAIT_FOR_LINK" }, true);
        }
        debug_menu_enabled();
        // TODO
        // sub_66C242(&g_game_ptr->field_4C);
        break;
    }

    case 121u: // Show Districts
    {
        debug_menu::hide();
        os_developer_options::instance()->set_flag(mString { "SHOW_END_OF_PACK" }, a1->get_bval());

        if (a1->get_bval()) {
            os_developer_options::instance()->set_flag(mString { "SHOW_END_OF_PACK" }, true);
        }
        debug_menu_enabled();
        // TODO
        // sub_66C242(&g_game_ptr->field_4C);
        break;
    }

    case 122u: // Show Districts
    {
        debug_menu::hide();
        os_developer_options::instance()->set_flag(mString { "LIVE_IN_GLASS_HOUSE" }, a1->get_bval());

        if (a1->get_bval()) {
            os_developer_options::instance()->set_flag(mString { "LIVE_IN_GLASS_HOUSE" }, true);
        }
        debug_menu_enabled();
        // TODO
        // sub_66C242(&g_game_ptr->field_4C);
        break;
    }

    case 123u: // Show Districts
    {
        debug_menu::hide();
        os_developer_options::instance()->set_flag(mString { "SHOW_GLASS_HOUSE" }, a1->get_bval());

        if (a1->get_bval()) {
            os_developer_options::instance()->set_flag(mString { "SHOW_GLASS_HOUSE" }, true);
        }
        debug_menu_enabled();
        // TODO
        // sub_66C242(&g_game_ptr->field_4C);
        break;
    }

    case 124u: // Show Districts
    {
        debug_menu::hide();
        os_developer_options::instance()->set_flag(mString { "DISABLE_RACE_PREVIEW" }, a1->get_bval());

        if (a1->get_bval()) {
            os_developer_options::instance()->set_flag(mString { "DISABLE_RACE_PREVIEW" }, true);
        }
        debug_menu_enabled();
        // TODO
        // sub_66C242(&g_game_ptr->field_4C);
        break;
    }

    case 125u: // Show Districts
    {
        debug_menu::hide();
        os_developer_options::instance()->set_flag(mString { "FREE_MINI_MAP" }, a1->get_bval());

        if (a1->get_bval()) {
            os_developer_options::instance()->set_flag(mString { "FREE_MINI_MAP" }, true);
        }
        debug_menu_enabled();
        // TODO
        // sub_66C242(&g_game_ptr->field_4C);
        break;
    }

    case 126u: // Show Districts
    {
        debug_menu::hide();
        os_developer_options::instance()->set_flag(mString { "SHOW_LOOPING_ANIM_WARNINGS" }, a1->get_bval());

        if (a1->get_bval()) {
            os_developer_options::instance()->set_flag(mString { "SHOW_LOOPING_ANIM_WARNINGS" }, true);
        }
        debug_menu_enabled();
        // TODO
        // sub_66C242(&g_game_ptr->field_4C);
        break;
    }

    case 127u: // Show Districts
    {
        debug_menu::hide();
        os_developer_options::instance()->set_flag(mString { "SHOW_PERF_INFO" }, a1->get_bval());

        if (a1->get_bval()) {
            os_developer_options::instance()->set_flag(mString { "SHOW_PERF_INFO" }, true);
        }
        debug_menu_enabled();
        // TODO
        // sub_66C242(&g_game_ptr->field_4C);
        break;
    }

    case 128u: // Show Districts
    {
        debug_menu::hide();
        os_developer_options::instance()->set_flag(mString { "COPY_ERROR_TO_CLIPBOARD" }, a1->get_bval());

        if (a1->get_bval()) {
            os_developer_options::instance()->set_flag(mString { "COPY_ERROR_TO_CLIPBOARD" }, true);
        }
        debug_menu_enabled();
        // TODO
        // sub_66C242(&g_game_ptr->field_4C);
        break;
    }

    case 129u: // Show Districts
    {
        debug_menu::hide();
        os_developer_options::instance()->set_flag(mString { "BOTH_HANDS_UP_REORIENT" }, a1->get_bval());

        if (a1->get_bval()) {
            os_developer_options::instance()->set_flag(mString { "BOTH_HANDS_UP_REORIENT" }, true);
        }
        debug_menu_enabled();
        // TODO
        // sub_66C242(&g_game_ptr->field_4C);
        break;
    }

    case 130u: // Show Districts
    {
        debug_menu::hide();
        os_developer_options::instance()->set_flag(mString { "SHOW_CAMERA_PROJECTION" }, a1->get_bval());

        if (a1->get_bval()) {
            os_developer_options::instance()->set_flag(mString { "SHOW_CAMERA_PROJECTION" }, true);
        }
        debug_menu_enabled();
        // TODO
        // sub_66C242(&g_game_ptr->field_4C);
        break;
    }

    case 131u: // Show Districts
    {
        debug_menu::hide();
        os_developer_options::instance()->set_flag(mString { "OLD_DEFAULT_CONTROL_SETTINGS" }, a1->get_bval());

        if (a1->get_bval()) {
            os_developer_options::instance()->set_flag(mString { "OLD_DEFAULT_CONTROL_SETTINGS" }, true);
        }
        debug_menu_enabled();
        // TODO
        // sub_66C242(&g_game_ptr->field_4C);
        break;
    }

    case 132u: // Show Districts
    {
        debug_menu::hide();
        os_developer_options::instance()->set_flag(mString { "IGC_SPEED_CONTROL" }, a1->get_bval());

        if (a1->get_bval()) {
            os_developer_options::instance()->set_flag(mString { "IGC_SPEED_CONTROL" }, true);
        }
        debug_menu_enabled();
        // TODO
        // sub_66C242(&g_game_ptr->field_4C);
        break;
    }

    case 133u: // Show Districts
    {
        debug_menu::hide();
        os_developer_options::instance()->set_flag(mString { "RTDT_ENABLED" }, a1->get_bval());

        if (a1->get_bval()) {
            os_developer_options::instance()->set_flag(mString { "RTDT_ENABLED" }, true);
        }
        debug_menu_enabled();
        // TODO
        // sub_66C242(&g_game_ptr->field_4C);
        break;
    }

    case 134u: // Show Districts
    {
        debug_menu::hide();
        os_developer_options::instance()->set_flag(mString { "ENABLE_DECALS" }, a1->get_bval());

        if (a1->get_bval()) {
            os_developer_options::instance()->set_flag(mString { "ENABLE_DECALS" }, true);
        }
        debug_menu_enabled();
        // TODO
        // sub_66C242(&g_game_ptr->field_4C);
        break;
    }

    case 135u: // Show Districts
    {
        debug_menu::hide();
        os_developer_options::instance()->set_flag(mString { "AUTO_STICK_TO_WALLS" }, a1->get_bval());

        if (a1->get_bval()) {
            os_developer_options::instance()->set_flag(mString { "AUTO_STICK_TO_WALLS" }, true);
        }
        debug_menu_enabled();
        // TODO
        // sub_66C242(&g_game_ptr->field_4C);
        break;
    }

    case 136u: // Show Districts
    {
        debug_menu::hide();
        os_developer_options::instance()->set_flag(mString { "ENABLE_PEDESTRIANS" }, a1->get_bval());

        if (a1->get_bval()) {
            os_developer_options::instance()->set_flag(mString { "ENABLE_PEDESTRIANS" }, true);
        }
        debug_menu_enabled();
        // TODO
        // sub_66C242(&g_game_ptr->field_4C);
        break;
    }

    case 137u: // Show Districts
    {
        debug_menu::hide();
        os_developer_options::instance()->set_flag(mString { "CAMERA_INVERT_LOOKAROUND" }, a1->get_bval());

        if (a1->get_bval()) {
            os_developer_options::instance()->set_flag(mString { "CAMERA_INVERT_LOOKAROUND" }, true);
        }
        debug_menu_enabled();
        // TODO
        // sub_66C242(&g_game_ptr->field_4C);
        break;
    }

    case 138u: // Show Districts
    {
        debug_menu::hide();
        os_developer_options::instance()->set_flag(mString { "CAMERA_INVERT_LOOKAROUND_X" }, a1->get_bval());

        if (a1->get_bval()) {
            os_developer_options::instance()->set_flag(mString { "CAMERA_INVERT_LOOKAROUND_X" }, true);
        }
        debug_menu_enabled();
        // TODO
        // sub_66C242(&g_game_ptr->field_4C);
        break;
    }

    case 139u: // Show Districts
    {
        debug_menu::hide();
        os_developer_options::instance()->set_flag(mString { "CAMERA_INVERT_LOOKAROUND_Y" }, a1->get_bval());

        if (a1->get_bval()) {
            os_developer_options::instance()->set_flag(mString { "CAMERA_INVERT_LOOKAROUND_Y" }, true);
        }
        debug_menu_enabled();
        // TODO
        // sub_66C242(&g_game_ptr->field_4C);
        break;
    }

    case 140u: // Show Districts
    {
        debug_menu::hide();
        os_developer_options::instance()->set_flag(mString { "FORCE_COMBAT_CAMERA_OFF" }, a1->get_bval());

        if (a1->get_bval()) {
            os_developer_options::instance()->set_flag(mString { "FORCE_COMBAT_CAMERA_OFF" }, true);
        }
        debug_menu_enabled();
        // TODO
        // sub_66C242(&g_game_ptr->field_4C);
        break;
    }

    case 141u: // Show Districts
    {
        debug_menu::hide();
        os_developer_options::instance()->set_flag(mString { "DISPLAY_THOUGHT_BUBBLES" }, a1->get_bval());

        if (a1->get_bval()) {
            os_developer_options::instance()->set_flag(mString { "DISPLAY_THOUGHT_BUBBLES" }, true);
        }
        debug_menu_enabled();
        // TODO
        // sub_66C242(&g_game_ptr->field_4C);
        break;
    }

    case 142u: // Show Districts
    {
        debug_menu::hide();
        os_developer_options::instance()->set_flag(mString { "ENABLE_DEBUG_LOG" }, a1->get_bval());

        if (a1->get_bval()) {
            os_developer_options::instance()->set_flag(mString { "ENABLE_DEBUG_LOG" }, true);
        }
        debug_menu_enabled();
        // TODO
        // sub_66C242(&g_game_ptr->field_4C);
        break;
    }

    case 143u: // Show Districts
    {
        debug_menu::hide();
        os_developer_options::instance()->set_flag(mString { "ENABLE_LONG_CALLSTACK" }, a1->get_bval());

        if (a1->get_bval()) {
            os_developer_options::instance()->set_flag(mString { "ENABLE_LONG_CALLSTACK" }, true);
        }
        debug_menu_enabled();
        // TODO
        // sub_66C242(&g_game_ptr->field_4C);
        break;
    }

    case 144u: // Show Districts
    {
        debug_menu::hide();
        os_developer_options::instance()->set_flag(mString { "RENDER_FE_UI" }, a1->get_bval());

        if (a1->get_bval()) {
            os_developer_options::instance()->set_flag(mString { "RENDER_FE_UI" }, true);
        }
        debug_menu_enabled();
        // TODO
        // sub_66C242(&g_game_ptr->field_4C);
        break;
    }

    case 145u: // Show Districts
    {
        debug_menu::hide();
        os_developer_options::instance()->set_flag(mString { "LOCK_INTERIORS" }, a1->get_bval());

        if (a1->get_bval()) {
            os_developer_options::instance()->set_flag(mString { "LOCK_INTERIORS" }, true);
        }
        debug_menu_enabled();
        // TODO
        // sub_66C242(&g_game_ptr->field_4C);
        break;
    }

    case 146u: // Show Districts
    {
        debug_menu::hide();
        os_developer_options::instance()->set_flag(mString { "MEMCHECK_ON_LOAD" }, a1->get_bval());

        if (a1->get_bval()) {
            os_developer_options::instance()->set_flag(mString { "MEMCHECK_ON_LOAD" }, true);
        }
        debug_menu_enabled();
        // TODO
        // sub_66C242(&g_game_ptr->field_4C);
        break;
    }

    case 147u: // Show Districts
    {
        debug_menu::hide();
        os_developer_options::instance()->set_flag(mString { "DISPLAY_ALS_USAGE_PROFILE" }, a1->get_bval());

        if (a1->get_bval()) {
            os_developer_options::instance()->set_flag(mString { "DISPLAY_ALS_USAGE_PROFILE" }, true);
        }
        debug_menu_enabled();
        // TODO
        // sub_66C242(&g_game_ptr->field_4C);
        break;
    }

    case 148u: // Show Districts
    {
        debug_menu::hide();
        os_developer_options::instance()->set_flag(mString { "ENABLE_FPU_EXCEPTION_HANDLING" }, a1->get_bval());

        if (a1->get_bval()) {
            os_developer_options::instance()->set_flag(mString { "ENABLE_FPU_EXCEPTION_HANDLING" }, false);
        }
        debug_menu_enabled();
        // TODO
        // sub_66C242(&g_game_ptr->field_4C);
        break;
    }

    case 149u: // Show Districts
    {
        debug_menu::hide();
        os_developer_options::instance()->set_flag(mString { "UNLOCK_ALL_UNLOCKABLES" }, a1->get_bval());

        if (a1->get_bval()) {
            os_developer_options::instance()->set_flag(mString { "UNLOCK_ALL_UNLOCKABLES" }, true);
        }
        debug_menu_enabled();
        // TODO
        // sub_66C242(&g_game_ptr->field_4C);
        break;
    }
    default:
        return;
    }
}


void create_devopt_menu(debug_menu* parent)
{
    if (parent->used_slots != 0) {
        return;
    }

    assert(parent != nullptr);

    auto* v10 = parent;



    debug_menu_entry v90 = (mString { "CD_ONLY" });
    v90.set_bval(false);
    v90.set_game_flags_handler(devopt_flags_handler);
    v90.set_id(0);
    v10->add_entry(&v90);

    // v92->add_entry(&v90);

    v90 = debug_menu_entry(mString { "ENVMAP_TOOL" });
    v90.set_bval(false);
    v90.set_game_flags_handler(devopt_flags_handler);
    v90.set_id(1);
    add_debug_menu_entry(devopt_menu, &v90);

    v90 = debug_menu_entry { mString { "NO_CD" } };
    v90.set_bval(false);
    v90.set_game_flags_handler(devopt_flags_handler);
    v90.set_id(2);
    add_debug_menu_entry(devopt_menu, &v90);

    v90 = debug_menu_entry { mString { "CHATTY_LOAD" } };
    v90.set_bval(false);
    v90.set_game_flags_handler(devopt_flags_handler);
    v90.set_id(3);
    add_debug_menu_entry(devopt_menu, &v90);

    v90 = debug_menu_entry { mString { "WINDOW_DEFAULT" } };
    v90.set_bval(false);
    v90.set_game_flags_handler(devopt_flags_handler);
    v90.set_id(4);
    add_debug_menu_entry(devopt_menu, &v90);

    v90 = debug_menu_entry { mString { "SHOW_FPS" } };
    static bool show_fps = false;
    v90.set_pt_bval(&show_fps);
    v90.set_game_flags_handler(devopt_flags_handler);
    v90.set_id(5);
    add_debug_menu_entry(devopt_menu, &v90);

    v90 = debug_menu_entry { mString { "SHOW_STREAMER_INFO" } };
    v90.set_bval(false);
    v90.set_game_flags_handler(devopt_flags_handler);
    v90.set_id(6);
    add_debug_menu_entry(devopt_menu, &v90);

    v90 = debug_menu_entry { mString { "SHOW_STREAMER_SPAM" } };
    v90.set_bval(false);
    v90.set_game_flags_handler(devopt_flags_handler);
    v90.set_id(7);
    add_debug_menu_entry(devopt_menu, &v90);

    v90 = debug_menu_entry { mString { "SHOW_RESOURCE_SPAM" } };
    v90.set_bval(false);
    v90.set_game_flags_handler(devopt_flags_handler);
    v90.set_id(8);
    add_debug_menu_entry(devopt_menu, &v90);

    v90 = debug_menu_entry { mString { "SHOW MEMORY INFO" } };
    v90.set_bval(false);
    v90.set_game_flags_handler(devopt_flags_handler);
    v90.set_id(9);
    add_debug_menu_entry(devopt_menu, &v90);

    v90 = debug_menu_entry { mString { "SHOW_SPIDEY_SPEED" } };
    v90.set_bval(false);
    v90.set_game_flags_handler(devopt_flags_handler);
    v90.set_id(10);
    add_debug_menu_entry(devopt_menu, &v90);

    v90 = debug_menu_entry { mString { "TRACE_MISSION_MANAGER" } };
    v90.set_bval(false);
    v90.set_game_flags_handler(devopt_flags_handler);
    v90.set_id(11);
    add_debug_menu_entry(devopt_menu, &v90);

    v90 = debug_menu_entry { mString { "DUMP_MISSION_HEAP" } };
    v90.set_bval(false);
    v90.set_game_flags_handler(devopt_flags_handler);
    v90.set_id(12);
    add_debug_menu_entry(devopt_menu, &v90);

    v90 = debug_menu_entry { mString { "CAMERA_CENTRIC_STREAMER" } };
    v90.set_bval(false);
    v90.set_game_flags_handler(devopt_flags_handler);
    v90.set_id(13);
    add_debug_menu_entry(devopt_menu, &v90);

    v90 = debug_menu_entry { mString { "RENDER_LOWLODS" } };
    v90.set_bval(false);
    v90.set_game_flags_handler(devopt_flags_handler);
    v90.set_id(14);
    add_debug_menu_entry(devopt_menu, &v90);

    v90 = debug_menu_entry { mString { "LOAD_STRING_HASH_DICTIONARY" } };
    v90.set_bval(false);
    v90.set_game_flags_handler(devopt_flags_handler);
    v90.set_id(15);
    add_debug_menu_entry(devopt_menu, &v90);

    v90 = debug_menu_entry { mString { "LOG_RUNTIME_STRING_HASHES" } };
    v90.set_bval(false);
    v90.set_game_flags_handler(devopt_flags_handler);
    v90.set_id(16);
    add_debug_menu_entry(devopt_menu, &v90);

    v90 = debug_menu_entry { mString { "SHOW_WATERMARK_VELOCITY" } };
    v90.set_bval(false);
    v90.set_game_flags_handler(devopt_flags_handler);
    v90.set_id(17);
    add_debug_menu_entry(devopt_menu, &v90);

    v90 = debug_menu_entry { mString { "SHOW_STATS" } };
    v90.set_bval(false);
    v90.set_game_flags_handler(devopt_flags_handler);
    v90.set_id(18);
    add_debug_menu_entry(devopt_menu, &v90);

    v90 = debug_menu_entry { mString { "ENABLE_ZOOM_MAP" } };
    v90.set_bval(false);
    v90.set_game_flags_handler(devopt_flags_handler);
    v90.set_id(19);
    add_debug_menu_entry(devopt_menu, &v90);

    v90 = debug_menu_entry { mString { "SHOW_DEBUG_INFO" } };
    v90.set_bval(false);
    v90.set_game_flags_handler(devopt_flags_handler);
    v90.set_id(20);
    add_debug_menu_entry(devopt_menu, &v90);

    v90 = debug_menu_entry { mString { "SHOW_PROFILE_INFO" } };
    v90.set_bval(false);
    v90.set_game_flags_handler(devopt_flags_handler);
    v90.set_id(21);
    add_debug_menu_entry(devopt_menu, &v90);

    v90 = debug_menu_entry { mString { "SHOW_LOCOMOTION_INFO" } };
    v90.set_bval(false);
    v90.set_game_flags_handler(devopt_flags_handler);
    v90.set_id(22);
    add_debug_menu_entry(devopt_menu, &v90);

    v90 = debug_menu_entry { mString { "GRAVITY" } };
    v90.set_bval(false);
    v90.set_game_flags_handler(devopt_flags_handler);
    v90.set_id(23);
    add_debug_menu_entry(devopt_menu, &v90);

    v90 = debug_menu_entry { mString { "TEST_MEMORY_LEAKS" } };
    v90.set_bval(false);
    v90.set_game_flags_handler(devopt_flags_handler);
    v90.set_id(24);
    add_debug_menu_entry(devopt_menu, &v90);

    v90 = debug_menu_entry { mString { "HALT_ON_ASSERT" } };
    v90.set_bval(false);
    v90.set_game_flags_handler(devopt_flags_handler);
    v90.set_id(25);
    add_debug_menu_entry(devopt_menu, &v90);

    v90 = debug_menu_entry { mString { "SCREEN_ASSERT" } };
    v90.set_bval(false);
    v90.set_game_flags_handler(devopt_flags_handler);
    v90.set_id(26);
    add_debug_menu_entry(devopt_menu, &v90);

    v90 = debug_menu_entry { mString { "NO_ANIM_WARNINGS" } };
    v90.set_bval(false);
    v90.set_game_flags_handler(devopt_flags_handler);
    v90.set_id(27);
    add_debug_menu_entry(devopt_menu, &v90);

    v90 = debug_menu_entry { mString { "PROFILING_ON" } };
    v90.set_bval(false);
    v90.set_game_flags_handler(devopt_flags_handler);
    v90.set_id(28);
    add_debug_menu_entry(devopt_menu, &v90);

    v90 = debug_menu_entry { mString { "MONO_AUDIO" } };
    v90.set_bval(false);
    v90.set_game_flags_handler(devopt_flags_handler);
    v90.set_id(29);
    add_debug_menu_entry(devopt_menu, &v90);

    v90 = debug_menu_entry { mString { "NO_MESSAGGES" } };
    v90.set_bval(false);
    v90.set_game_flags_handler(devopt_flags_handler);
    v90.set_id(30);
    add_debug_menu_entry(devopt_menu, &v90);

    v90 = debug_menu_entry { mString { "LOCK_STEP" } };
    v90.set_bval(false);
    v90.set_game_flags_handler(devopt_flags_handler);
    v90.set_id(31);
    add_debug_menu_entry(devopt_menu, &v90);

    v90 = debug_menu_entry { mString { "TEXTURE_DUMP" } };
    v90.set_bval(false);
    v90.set_game_flags_handler(devopt_flags_handler);
    v90.set_id(32);
    add_debug_menu_entry(devopt_menu, &v90);

    v90 = debug_menu_entry { mString { "DISABLE_SOUND_WARNINGS" } };
    v90.set_bval(false);
    v90.set_game_flags_handler(devopt_flags_handler);
    v90.set_id(33);
    add_debug_menu_entry(devopt_menu, &v90);

    v90 = debug_menu_entry { mString { "DISABLE_SOUND_DEBUG_OUTPUT" } };
    v90.set_bval(false);
    v90.set_game_flags_handler(devopt_flags_handler);
    v90.set_id(34);
    add_debug_menu_entry(devopt_menu, &v90);

    v90 = debug_menu_entry { mString { "DELETE_UNUSED_SOUND_BANKS_ON_PACK" } };
    v90.set_bval(false);
    v90.set_game_flags_handler(devopt_flags_handler);
    v90.set_id(35);
    add_debug_menu_entry(devopt_menu, &v90);

    v90 = debug_menu_entry { mString { "LOCKED_HERO" } };
    v90.set_bval(false);
    v90.set_game_flags_handler(devopt_flags_handler);
    v90.set_id(36);
    add_debug_menu_entry(devopt_menu, &v90);

    v90 = debug_menu_entry { mString { "FOG_OVERR_IDE" } };
    v90.set_bval(false);
    v90.set_game_flags_handler(devopt_flags_handler);
    v90.set_id(37);
    add_debug_menu_entry(devopt_menu, &v90);

    v90 = debug_menu_entry { mString { "FOG_DISABLE" } };
    v90.set_bval(false);
    v90.set_game_flags_handler(devopt_flags_handler);
    v90.set_id(38);
    add_debug_menu_entry(devopt_menu, &v90);

    v90 = debug_menu_entry { mString { "MOVE_EDITOR" } };
    v90.set_bval(false);
    v90.set_game_flags_handler(devopt_flags_handler);
    v90.set_id(39);
    add_debug_menu_entry(devopt_menu, &v90);

    v90 = debug_menu_entry { mString { "PATH_DEBUG" } };
    v90.set_bval(false);
    v90.set_game_flags_handler(devopt_flags_handler);
    v90.set_id(40);
    add_debug_menu_entry(devopt_menu, &v90);

    v90 = debug_menu_entry { mString { "PATH_COLOR" } };
    v90.set_bval(false);
    v90.set_game_flags_handler(devopt_flags_handler);
    v90.set_id(41);
    add_debug_menu_entry(devopt_menu, &v90);

    v90 = debug_menu_entry { mString { "CRITTER_ACTIVITY" } };
    v90.set_bval(false);
    v90.set_game_flags_handler(devopt_flags_handler);
    v90.set_id(42);
    add_debug_menu_entry(devopt_menu, &v90);

    v90 = debug_menu_entry { mString { "PATH_COLOR_CRITTER" } };
    v90.set_bval(false);
    v90.set_game_flags_handler(devopt_flags_handler);
    v90.set_id(43);
    add_debug_menu_entry(devopt_menu, &v90);

    v90 = debug_menu_entry { mString { "PATH_COLOR_HERO" } };
    v90.set_bval(false);
    v90.set_game_flags_handler(devopt_flags_handler);
    v90.set_id(44);
    add_debug_menu_entry(devopt_menu, &v90);

    v90 = debug_menu_entry { mString { "NO_PARTICLES" } };
    v90.set_bval(false);
    v90.set_game_flags_handler(devopt_flags_handler);
    v90.set_id(45);
    add_debug_menu_entry(devopt_menu, &v90);

    v90 = debug_menu_entry { mString { "NO_PARTICLE_PUMP" } };
    v90.set_bval(false);
    v90.set_game_flags_handler(devopt_flags_handler);
    v90.set_id(46);
    add_debug_menu_entry(devopt_menu, &v90);

    v90 = debug_menu_entry { mString { "SHOW_SHADOW_BALL" } };
    v90.set_bval(false);
    v90.set_game_flags_handler(devopt_flags_handler);
    v90.set_id(47);
    add_debug_menu_entry(devopt_menu, &v90);

    v90 = debug_menu_entry { mString { "SHOW_NORMALS" } };
    v90.set_bval(false);
    v90.set_game_flags_handler(devopt_flags_handler);
    v90.set_id(48);
    add_debug_menu_entry(devopt_menu, &v90);

    v90 = debug_menu_entry { mString { "SHOW_LIGHTS" } };
    v90.set_bval(false);
    v90.set_game_flags_handler(devopt_flags_handler);
    v90.set_id(49);
    add_debug_menu_entry(devopt_menu, &v90);

    v90 = debug_menu_entry { mString { "SHOW_PLRCTRL" } };
    v90.set_bval(false);
    v90.set_game_flags_handler(devopt_flags_handler);
    v90.set_id(50);
    add_debug_menu_entry(devopt_menu, &v90);

    v90 = debug_menu_entry { mString { "SHOW_PSX_INFO" } };
    v90.set_bval(false);
    v90.set_game_flags_handler(devopt_flags_handler);
    v90.set_id(51);
    add_debug_menu_entry(devopt_menu, &v90);

    v90 = debug_menu_entry { mString { "FLAT_SHADE" } };
    v90.set_bval(false);
    v90.set_game_flags_handler(devopt_flags_handler);
    v90.set_id(52);
    add_debug_menu_entry(devopt_menu, &v90);

    v90 = debug_menu_entry { mString { "INTERFACE_DISABLE" } };
    v90.set_bval(false);
    v90.set_game_flags_handler(devopt_flags_handler);
    v90.set_id(53);
    add_debug_menu_entry(devopt_menu, &v90);

    v90 = debug_menu_entry { mString { "WIDGET_TOOLS" } };
    v90.set_bval(false);
    v90.set_game_flags_handler(devopt_flags_handler);
    v90.set_id(54);
    add_debug_menu_entry(devopt_menu, &v90);

    v90 = debug_menu_entry { mString { "LIGHTING" } };
    v90.set_bval(false);
    v90.set_game_flags_handler(devopt_flags_handler);
    v90.set_id(55);
    add_debug_menu_entry(devopt_menu, &v90);

    v90 = debug_menu_entry { mString { "FAKE_POINT_LIGHTS" } };
    v90.set_bval(false);
    v90.set_game_flags_handler(devopt_flags_handler);
    v90.set_id(56);
    add_debug_menu_entry(devopt_menu, &v90);

    v90 = debug_menu_entry { mString { "BSP_SPRAY_PAINT" } };
    v90.set_bval(false);
    v90.set_game_flags_handler(devopt_flags_handler);
    v90.set_id(57);
    add_debug_menu_entry(devopt_menu, &v90);

    v90 = debug_menu_entry { mString { "CAMERA_EDITOR" } };
    v90.set_bval(false);
    v90.set_game_flags_handler(devopt_flags_handler);
    v90.set_id(58);
    add_debug_menu_entry(devopt_menu, &v90);

    v90 = debug_menu_entry { mString { "IGNORE_RAMPING" } };
    v90.set_bval(false);
    v90.set_game_flags_handler(devopt_flags_handler);
    v90.set_id(59);
    add_debug_menu_entry(devopt_menu, &v90);

    v90 = debug_menu_entry { mString { "POINT_SAMPLE" } };
    v90.set_bval(false);
    v90.set_game_flags_handler(devopt_flags_handler);
    v90.set_id(60);
    add_debug_menu_entry(devopt_menu, &v90);

    v90 = debug_menu_entry { mString { "DISTANCE_FADING" } };
    v90.set_bval(false);
    v90.set_game_flags_handler(devopt_flags_handler);
    v90.set_id(61);
    add_debug_menu_entry(devopt_menu, &v90);

    v90 = debug_menu_entry { mString { "OVERRIDE_CONTROLLER_OPTIONS" } };
    v90.set_bval(false);
    v90.set_game_flags_handler(devopt_flags_handler);
    v90.set_id(62);
    add_debug_menu_entry(devopt_menu, &v90);

    v90 = debug_menu_entry { mString { "DISABLE_MOUSE_PLAYER_CONTROL" } };
    v90.set_bval(false);
    v90.set_game_flags_handler(devopt_flags_handler);
    v90.set_id(63);
    add_debug_menu_entry(devopt_menu, &v90);

    v90 = debug_menu_entry { mString { "NO_MOVIES" } };
    v90.set_bval(false);
    v90.set_game_flags_handler(devopt_flags_handler);
    v90.set_id(64);
    add_debug_menu_entry(devopt_menu, &v90);

    v90 = debug_menu_entry { mString { "XBOX_USER_CAM" } };
    v90.set_bval(false);
    v90.set_game_flags_handler(devopt_flags_handler);
    v90.set_id(65);
    add_debug_menu_entry(devopt_menu, &v90);

    v90 = debug_menu_entry { mString { "NO_LOAD_SCREEN" } };
    v90.set_bval(false);
    v90.set_game_flags_handler(devopt_flags_handler);
    v90.set_id(66);
    add_debug_menu_entry(devopt_menu, &v90);

    v90 = debug_menu_entry { mString { "EXCEPTION_HANDLER" } };
    v90.set_bval(false);
    v90.set_game_flags_handler(devopt_flags_handler);
    v90.set_id(67);
    add_debug_menu_entry(devopt_menu, &v90);

    v90 = debug_menu_entry { mString { "NO_EXCEPTION_HANDLER" } };
    v90.set_bval(false);
    v90.set_game_flags_handler(devopt_flags_handler);
    v90.set_id(68);
    add_debug_menu_entry(devopt_menu, &v90);

    v90 = debug_menu_entry { mString { "NO_CD_CHECK" } };
    v90.set_bval(false);
    v90.set_game_flags_handler(devopt_flags_handler);
    v90.set_id(69);
    add_debug_menu_entry(devopt_menu, &v90);

    v90 = debug_menu_entry { mString { "NO_LOAD_METER" } };
    v90.set_bval(false);
    v90.set_game_flags_handler(devopt_flags_handler);
    v90.set_id(70);
    add_debug_menu_entry(devopt_menu, &v90);

    v90 = debug_menu_entry { mString { "NO_MOVIE_BUFFER_WARNING" } };
    v90.set_bval(false);
    v90.set_game_flags_handler(devopt_flags_handler);
    v90.set_id(71);
    add_debug_menu_entry(devopt_menu, &v90);

    v90 = debug_menu_entry { mString { "LIMITED_EDITION_DISC" } };
    v90.set_game_flags_handler(devopt_flags_handler);
    v90.set_bval(false);
    v90.set_id(72);
    add_debug_menu_entry(devopt_menu, &v90);

    v90 = debug_menu_entry { mString { "NEW_COMBAT_LOCO" } };
    v90.set_bval(false);
    v90.set_game_flags_handler(devopt_flags_handler);
    v90.set_id(73);
    add_debug_menu_entry(devopt_menu, &v90);

    v90 = debug_menu_entry { mString { "LEVEL_WARP" } };
    v90.set_bval(false);
    v90.set_game_flags_handler(devopt_flags_handler);
    v90.set_id(74);
    add_debug_menu_entry(devopt_menu, &v90);

    v90 = debug_menu_entry { mString { "SMOKE_TEST" } };
    v90.set_bval(false);
    v90.set_game_flags_handler(devopt_flags_handler);
    v90.set_id(75);
    add_debug_menu_entry(devopt_menu, &v90);

    v90 = debug_menu_entry { mString { "SMOKE_TEST_LEVEL" } };
    v90.set_bval(false);
    v90.set_game_flags_handler(devopt_flags_handler);
    v90.set_id(76);
    add_debug_menu_entry(devopt_menu, &v90);

    v90 = debug_menu_entry { mString { "COMBO_TESTER" } };
    v90.set_bval(false);
    v90.set_game_flags_handler(devopt_flags_handler);
    v90.set_id(77);
    add_debug_menu_entry(devopt_menu, &v90);

    v90 = debug_menu_entry { mString { "DROP_SHADOWS_ALWAYS_RAYCAST" } };
    v90.set_bval(false);
    v90.set_game_flags_handler(devopt_flags_handler);
    v90.set_id(78);
    add_debug_menu_entry(devopt_menu, &v90);

    v90 = debug_menu_entry { mString { "DISABLE_DROP_SHADOWS" } };
    v90.set_bval(false);
    v90.set_game_flags_handler(devopt_flags_handler);
    v90.set_id(79);
    add_debug_menu_entry(devopt_menu, &v90);

    v90 = debug_menu_entry { mString { "DISABLE_HIRES_SHADOWS" } };
    v90.set_bval(false);
    v90.set_game_flags_handler(devopt_flags_handler);
    v90.set_id(80);
    add_debug_menu_entry(devopt_menu, &v90);

    v90 = debug_menu_entry { mString { "DISABLE_STENCIL_SHADOWS" } };
    v90.set_bval(false);
    v90.set_game_flags_handler(devopt_flags_handler);
    v90.set_id(81);
    add_debug_menu_entry(devopt_menu, &v90);

    v90 = debug_menu_entry { mString { "DISABLE_COLORVOLS" } };
    v90.set_bval(false);
    v90.set_game_flags_handler(devopt_flags_handler);
    v90.set_id(82);
    add_debug_menu_entry(devopt_menu, &v90);

    v90 = debug_menu_entry { mString { "DISABLE_RENDER_ENTS" } };
    v90.set_bval(false);
    v90.set_game_flags_handler(devopt_flags_handler);
    v90.set_id(83);
    add_debug_menu_entry(devopt_menu, &v90);

    v90 = debug_menu_entry { mString { "DISABLE_FULLSCREEN_BLUR" } };
    v90.set_bval(false);
    v90.set_game_flags_handler(devopt_flags_handler);
    v90.set_id(84);
    add_debug_menu_entry(devopt_menu, &v90);

    v90 = debug_menu_entry { mString { "FORCE_TIGHTCRAWL" } };
    v90.set_bval(false);
    v90.set_game_flags_handler(devopt_flags_handler);
    v90.set_id(85);
    add_debug_menu_entry(devopt_menu, &v90);

    v90 = debug_menu_entry { mString { "FORCE_NONCRAWL" } };
    v90.set_bval(false);
    v90.set_game_flags_handler(devopt_flags_handler);
    v90.set_id(86);
    add_debug_menu_entry(devopt_menu, &v90);

    v90 = debug_menu_entry { mString { "SHOW_DEBUG_TEXT" } };
    v90.set_bval(false);
    v90.set_game_flags_handler(devopt_flags_handler);
    v90.set_id(87);
    add_debug_menu_entry(devopt_menu, &v90);

    v90 = debug_menu_entry { mString { "SHOW_STYLE_POINTS" } };
    v90.set_bval(false);
    v90.set_game_flags_handler(devopt_flags_handler);
    v90.set_id(88);
    add_debug_menu_entry(devopt_menu, &v90);

    v90 = debug_menu_entry { mString { "CAMERA_MOUSE_MODE" } };
    v90.set_bval(false);
    v90.set_game_flags_handler(devopt_flags_handler);
    v90.set_id(89);
    add_debug_menu_entry(devopt_menu, &v90);

    v90 = debug_menu_entry { mString { "USERCAM_ON_CONTROLLER2" } };
    v90.set_bval(false);
    v90.set_game_flags_handler(devopt_flags_handler);
    v90.set_id(90);
    add_debug_menu_entry(devopt_menu, &v90);

    v90 = debug_menu_entry { mString { "DISABLE_ANCHOR_RETICLE_RENDERING" } };
    v90.set_bval(false);
    v90.set_game_flags_handler(devopt_flags_handler);
    v90.set_id(91);
    add_debug_menu_entry(devopt_menu, &v90);

    v90 = debug_menu_entry { mString { "SHOW_ANCHOR_LINE" } };
    v90.set_bval(false);
    v90.set_game_flags_handler(devopt_flags_handler);
    v90.set_id(92);
    add_debug_menu_entry(devopt_menu, &v90);

    v90 = debug_menu_entry { mString { "FREE_SPIDER_REFLEXES" } };
    v90.set_bval(false);
    v90.set_game_flags_handler(devopt_flags_handler);
    v90.set_id(93);
    add_debug_menu_entry(devopt_menu, &v90);

    v90 = debug_menu_entry { mString { "SHOW_BAR_OF_SHAME" } };
    v90.set_bval(false);
    v90.set_game_flags_handler(devopt_flags_handler);
    v90.set_id(94);
    add_debug_menu_entry(devopt_menu, &v90);

    v90 = debug_menu_entry { mString { "SHOW_ENEMY_HEALTH_WIDGETS" } };
    v90.set_bval(false);
    v90.set_game_flags_handler(devopt_flags_handler);
    v90.set_id(95);
    add_debug_menu_entry(devopt_menu, &v90);

    v90 = debug_menu_entry { mString { "ALLOW_IGC_PAUSE" } };
    v90.set_bval(false);
    v90.set_game_flags_handler(devopt_flags_handler);
    v90.set_id(96);
    add_debug_menu_entry(devopt_menu, &v90);

    v90 = debug_menu_entry { mString { "SHOW_OBBS" } };
    v90.set_bval(false);
    v90.set_game_flags_handler(devopt_flags_handler);
    v90.set_id(97);
    add_debug_menu_entry(devopt_menu, &v90);

    v90 = debug_menu_entry { mString { "SHOW_DISTRICTS" } };
    v90.set_bval(false);
    v90.set_game_flags_handler(devopt_flags_handler);
    v90.set_id(98);
    add_debug_menu_entry(devopt_menu, &v90);

    v90 = debug_menu_entry { mString { "SHOW_CHUCK_DEBUGGER" } };
    v90.set_bval(false);
    v90.set_game_flags_handler(devopt_flags_handler);
    v90.set_id(99);
    add_debug_menu_entry(devopt_menu, &v90);

    v90 = debug_menu_entry { mString { "CHUCK_OLD_FASHIONED" } };
    v90.set_bval(false);
    v90.set_game_flags_handler(devopt_flags_handler);
    v90.set_id(100);
    add_debug_menu_entry(devopt_menu, &v90);

    v90 = debug_menu_entry { mString { "CHUCK_DISABLE_BREAKPOINTS" } };
    v90.set_bval(false);
    v90.set_game_flags_handler(devopt_flags_handler);
    v90.set_id(101);
    add_debug_menu_entry(devopt_menu, &v90);

    v90 = debug_menu_entry { mString { "SHOW_AUDIO_BOXES" } };
    v90.set_bval(false);
    v90.set_game_flags_handler(devopt_flags_handler);
    v90.set_id(102);
    add_debug_menu_entry(devopt_menu, &v90);

    v90 = debug_menu_entry { mString { "DISABLE_SOUNDS" } };
    v90.set_bval(false);
    v90.set_game_flags_handler(devopt_flags_handler);
    v90.set_id(103);
    add_debug_menu_entry(devopt_menu, &v90);

    v90 = debug_menu_entry { mString { "SHOW_TERRAIN_INFO" } };
    v90.set_bval(false);
    v90.set_game_flags_handler(devopt_flags_handler);
    v90.set_id(104);
    add_debug_menu_entry(devopt_menu, &v90);

    v90 = debug_menu_entry { mString { "DISABLE_AUDIO_BOXES" } };
    v90.set_bval(false);
    v90.set_game_flags_handler(devopt_flags_handler);
    v90.set_id(105);
    add_debug_menu_entry(devopt_menu, &v90);

    v90 = debug_menu_entry { mString { "NSL_OLD_FASHIONED" } };
    v90.set_bval(false);
    v90.set_game_flags_handler(devopt_flags_handler);
    v90.set_id(106);
    add_debug_menu_entry(devopt_menu, &v90);

    v90 = debug_menu_entry { mString { "SHOW_MASTER_CLOCK" } };
    v90.set_bval(false);
    v90.set_game_flags_handler(devopt_flags_handler);
    v90.set_id(107);
    add_debug_menu_entry(devopt_menu, &v90);

    v90 = debug_menu_entry { mString { "LOAD_GRADIENTS" } };
    v90.set_bval(false);
    v90.set_game_flags_handler(devopt_flags_handler);
    v90.set_id(108);
    add_debug_menu_entry(devopt_menu, &v90);

    v90 = debug_menu_entry { mString { "BONUS_LEVELS_AVAILABLE" } };
    v90.set_bval(false);
    v90.set_game_flags_handler(devopt_flags_handler);
    v90.set_id(109);
    add_debug_menu_entry(devopt_menu, &v90);

    v90 = debug_menu_entry { mString { "COMBAT_DISPLAY" } };
    v90.set_bval(false);
    v90.set_game_flags_handler(devopt_flags_handler);
    v90.set_id(110);
    add_debug_menu_entry(devopt_menu, &v90);

    v90 = debug_menu_entry { mString { "COMBAT_DEBUGGER" } };
    v90.set_bval(false);
    v90.set_game_flags_handler(devopt_flags_handler);
    v90.set_id(111);
    add_debug_menu_entry(devopt_menu, &v90);

    v90 = debug_menu_entry { mString { "ALLOW_ERROR_POPUPS" } };
    v90.set_bval(false);
    v90.set_game_flags_handler(devopt_flags_handler);
    v90.set_id(112);
    add_debug_menu_entry(devopt_menu, &v90);

    v90 = debug_menu_entry { mString { "ALLOW_WARNING_POPUPS" } };
    v90.set_bval(false);
    v90.set_game_flags_handler(devopt_flags_handler);
    v90.set_id(113);
    add_debug_menu_entry(devopt_menu, &v90);

    v90 = debug_menu_entry { mString { "OUTPUT_WARNING_DISABLE" } };
    v90.set_bval(false);
    v90.set_game_flags_handler(devopt_flags_handler);
    v90.set_id(114);
    add_debug_menu_entry(devopt_menu, &v90);

    v90 = debug_menu_entry { mString { "OUTPUT_ASSERT_DISABLE" } };
    v90.set_bval(false);
    v90.set_game_flags_handler(devopt_flags_handler);
    v90.set_id(115);
    add_debug_menu_entry(devopt_menu, &v90);

    v90 = debug_menu_entry { mString { "ASSERT_ON_WARNING" } };
    v90.set_bval(false);
    v90.set_game_flags_handler(devopt_flags_handler);
    v90.set_id(116);
    add_debug_menu_entry(devopt_menu, &v90);

    v90 = debug_menu_entry { mString { "ALWAYS_ACTIVE" } };
    v90.set_bval(false);
    v90.set_game_flags_handler(devopt_flags_handler);
    v90.set_id(117);
    add_debug_menu_entry(devopt_menu, &v90);

    v90 = debug_menu_entry { mString { "FORCE_PROGRESSION" } };
    v90.set_bval(false);
    v90.set_game_flags_handler(devopt_flags_handler);
    v90.set_id(118);
    add_debug_menu_entry(devopt_menu, &v90);

    v90 = debug_menu_entry { mString { "LINK" } };
    v90.set_bval(false);
    v90.set_game_flags_handler(devopt_flags_handler);
    v90.set_id(119);
    add_debug_menu_entry(devopt_menu, &v90);

    v90 = debug_menu_entry { mString { "WAIT_FOR_LINK" } };
    v90.set_bval(false);
    v90.set_game_flags_handler(devopt_flags_handler);
    v90.set_id(120);
    add_debug_menu_entry(devopt_menu, &v90);

    v90 = debug_menu_entry { mString { "SHOW_END_OF_PACK" } };
    v90.set_bval(false);
    v90.set_game_flags_handler(devopt_flags_handler);
    v90.set_id(121);
    add_debug_menu_entry(devopt_menu, &v90);

    v90 = debug_menu_entry { mString { "LIVE_IN_GLASS_HOUSE" } };
    v90.set_bval(false);
    v90.set_game_flags_handler(devopt_flags_handler);
    v90.set_id(122);
    add_debug_menu_entry(devopt_menu, &v90);

    v90 = debug_menu_entry { mString { "SHOW_GLASS_HOUSE" } };
    v90.set_bval(false);
    v90.set_game_flags_handler(devopt_flags_handler);
    v90.set_id(123);
    add_debug_menu_entry(devopt_menu, &v90);

    v90 = debug_menu_entry { mString { "DISABLE_RACE_PREVIEW" } };
    v90.set_bval(false);
    v90.set_game_flags_handler(devopt_flags_handler);
    v90.set_id(124);
    add_debug_menu_entry(devopt_menu, &v90);

    v90 = debug_menu_entry { mString { "FREE_MINI_MAP" } };
    v90.set_bval(false);
    v90.set_game_flags_handler(devopt_flags_handler);
    v90.set_id(125);
    add_debug_menu_entry(devopt_menu, &v90);

    v90 = debug_menu_entry { mString { "SHOW_LOOPING_ANIM_WARNINGS" } };
    v90.set_bval(false);
    v90.set_game_flags_handler(devopt_flags_handler);
    v90.set_id(126);
    add_debug_menu_entry(devopt_menu, &v90);

    v90 = debug_menu_entry { mString { "SHOW_PERF_INFO" } };
    v90.set_bval(false);
    v90.set_game_flags_handler(devopt_flags_handler);
    v90.set_id(127);
    add_debug_menu_entry(devopt_menu, &v90);

    v90 = debug_menu_entry { mString { "COPY_ERROR_TO_CLIPBOARD" } };
    v90.set_bval(false);
    v90.set_game_flags_handler(devopt_flags_handler);
    v90.set_id(128);
    add_debug_menu_entry(devopt_menu, &v90);

    v90 = debug_menu_entry { mString { "BOTH_HANDS_UP_REORIENT" } };
    v90.set_bval(false);
    v90.set_game_flags_handler(devopt_flags_handler);
    v90.set_id(129);
    add_debug_menu_entry(devopt_menu, &v90);

    v90 = debug_menu_entry { mString { "SHOW_CAMERA_PROJECTION" } };
    v90.set_bval(false);
    v90.set_game_flags_handler(devopt_flags_handler);
    v90.set_id(130);
    add_debug_menu_entry(devopt_menu, &v90);

    v90 = debug_menu_entry { mString { "OLD_DEFAULT_CONTROL_SETTINGS" } };
    v90.set_bval(false);
    v90.set_game_flags_handler(devopt_flags_handler);
    v90.set_id(131);
    add_debug_menu_entry(devopt_menu, &v90);

    v90 = debug_menu_entry { mString { "IGC_SPEED_CONTROL" } };
    v90.set_bval(false);
    v90.set_game_flags_handler(devopt_flags_handler);
    v90.set_id(132);
    add_debug_menu_entry(devopt_menu, &v90);

    v90 = debug_menu_entry { mString { "RTDT_ENABLED" } };
    v90.set_bval(false);
    v90.set_game_flags_handler(devopt_flags_handler);
    v90.set_id(133);
    add_debug_menu_entry(devopt_menu, &v90);

    v90 = debug_menu_entry { mString { "ENABLE_DECALS" } };
    v90.set_bval(false);
    v90.set_game_flags_handler(devopt_flags_handler);
    v90.set_id(134);
    add_debug_menu_entry(devopt_menu, &v90);

    v90 = debug_menu_entry { mString { "AUTO_STICK_TO_WALLS" } };
    v90.set_bval(false);
    v90.set_game_flags_handler(devopt_flags_handler);
    v90.set_id(135);
    add_debug_menu_entry(devopt_menu, &v90);

    v90 = debug_menu_entry { mString { "ENABLE_PEDESTRIANS" } };
    v90.set_bval(false);
    v90.set_game_flags_handler(devopt_flags_handler);
    v90.set_id(136);
    add_debug_menu_entry(devopt_menu, &v90);

    v90 = debug_menu_entry { mString { "CAMERA_INVERT_LOOKAROUND" } };
    v90.set_bval(false);
    v90.set_game_flags_handler(devopt_flags_handler);
    v90.set_id(137);
    add_debug_menu_entry(devopt_menu, &v90);

    v90 = debug_menu_entry { mString { "CAMERA_INVERT_LOOKAROUND_X" } };
    v90.set_bval(false);
    v90.set_game_flags_handler(devopt_flags_handler);
    v90.set_id(138);
    add_debug_menu_entry(devopt_menu, &v90);

    v90 = debug_menu_entry { mString { "CAMERA_INVERT_LOOKAROUND_Y" } };
    v90.set_bval(false);
    v90.set_game_flags_handler(devopt_flags_handler);
    v90.set_id(139);
    add_debug_menu_entry(devopt_menu, &v90);

    v90 = debug_menu_entry { mString { "FORCE_COMBAT_CAMERA_OFF" } };
    v90.set_bval(false);
    v90.set_game_flags_handler(devopt_flags_handler);
    v90.set_id(140);
    add_debug_menu_entry(devopt_menu, &v90);

    v90 = debug_menu_entry { mString { "DISPLAY_THOUGHT_BUBBLES" } };
    v90.set_bval(false);
    v90.set_game_flags_handler(devopt_flags_handler);
    v90.set_id(141);
    add_debug_menu_entry(devopt_menu, &v90);

    v90 = debug_menu_entry { mString { "ENABLE_DEBUG_LOG" } };
    v90.set_bval(false);
    v90.set_game_flags_handler(devopt_flags_handler);
    v90.set_id(142);
    add_debug_menu_entry(devopt_menu, &v90);

    v90 = debug_menu_entry { mString { "ENABLE_LONG_CALLSTACK" } };
    v90.set_bval(false);
    v90.set_game_flags_handler(devopt_flags_handler);
    v90.set_id(143);
    add_debug_menu_entry(devopt_menu, &v90);

    v90 = debug_menu_entry { mString { "RENDER_FE_UI" } };
    v90.set_bval(false);
    v90.set_game_flags_handler(devopt_flags_handler);
    v90.set_id(144);
    add_debug_menu_entry(devopt_menu, &v90);

    v90 = debug_menu_entry { mString { "LOCK_INTERIORS" } };
    v90.set_bval(false);
    v90.set_game_flags_handler(devopt_flags_handler);
    v90.set_id(145);
    add_debug_menu_entry(devopt_menu, &v90);

    v90 = debug_menu_entry { mString { "MEMCHECK_ON_LOAD" } };
    v90.set_bval(false);
    v90.set_game_flags_handler(devopt_flags_handler);
    v90.set_id(146);
    add_debug_menu_entry(devopt_menu, &v90);

    v90 = debug_menu_entry { mString { "DISPLAY_ALS_USAGE_PROFILE" } };
    v90.set_bval(false);
    v90.set_game_flags_handler(devopt_flags_handler);
    v90.set_id(147);
    add_debug_menu_entry(devopt_menu, &v90);

    v90 = debug_menu_entry { mString { "ENABLE_FPU_EXCEPTION_HANDLING" } };
    v90.set_bval(false);
    v90.set_game_flags_handler(devopt_flags_handler);
    v90.set_id(148);
    add_debug_menu_entry(devopt_menu, &v90);

    v90 = debug_menu_entry { mString { "UNLOCK_ALL_UNLOCKABLES" } };
    v90.set_bval(false);
    v90.set_game_flags_handler(devopt_flags_handler);
    v90.set_id(149);
    add_debug_menu_entry(devopt_menu, &v90);

    // INTS

    v90 = debug_menu_entry { mString { "DIFFICULTY" } };
    v90.set_ival(0);
    v90.set_min_value(-1000.0);
    v90.set_max_value(1000.0);
    v90.set_game_flags_handler(devopt_ints_handler);
    v90.set_id(0);
    add_debug_menu_entry(devopt_menu, &v90);

    v90 = debug_menu_entry { mString { "CAMERA_STYLE" } };
    v90.set_ival(0);
    v90.set_min_value(-1000.0);
    v90.set_max_value(1000.0);
    v90.set_game_flags_handler(devopt_ints_handler);
    v90.set_id(1);
    add_debug_menu_entry(devopt_menu, &v90);

    v90 = debug_menu_entry { mString { "CAMERA_STATE" } };
    v90.set_ival(0);
    v90.set_min_value(-1000.0);
    v90.set_max_value(1000.0);
    v90.set_game_flags_handler(devopt_ints_handler);
    v90.set_id(2);
    add_debug_menu_entry(devopt_menu, &v90);

    v90 = debug_menu_entry { mString { "CAMERA_FOV" } };
    v90.set_ival(0);
    v90.set_min_value(-1000.0);
    v90.set_max_value(1000.0);
    v90.set_game_flags_handler(devopt_ints_handler);
    v90.set_id(3);
    add_debug_menu_entry(devopt_menu, &v90);

    v90 = debug_menu_entry { mString { "FOG_RED" } };
    v90.set_ival(0);
    v90.set_min_value(-1000.0);
    v90.set_max_value(1000.0);
    v90.set_game_flags_handler(devopt_ints_handler);
    v90.set_id(4);
    add_debug_menu_entry(devopt_menu, &v90);

    v90 = debug_menu_entry { mString { "FOG_GREEN" } };
    v90.set_ival(0);
    v90.set_min_value(-1000.0);
    v90.set_max_value(1000.0);
    v90.set_game_flags_handler(devopt_ints_handler);
    v90.set_id(5);
    add_debug_menu_entry(devopt_menu, &v90);

    v90 = debug_menu_entry { mString { "FOG_BLUE" } };
    v90.set_ival(0);
    v90.set_min_value(-1000.0);
    v90.set_max_value(1000.0);
    v90.set_game_flags_handler(devopt_ints_handler);
    v90.set_id(6);
    add_debug_menu_entry(devopt_menu, &v90);

    v90 = debug_menu_entry { mString { "FOG_DISTANCE" } };
    v90.set_ival(0);
    v90.set_min_value(-1000.0);
    v90.set_max_value(1000.0);
    v90.set_game_flags_handler(devopt_ints_handler);
    v90.set_id(7);
    add_debug_menu_entry(devopt_menu, &v90);

    v90 = debug_menu_entry { mString { "BIT_DEPTH" } };
    v90.set_ival(0);
    v90.set_min_value(-1000.0);
    v90.set_max_value(1000.0);
    v90.set_game_flags_handler(devopt_ints_handler);
    v90.set_id(8);
    add_debug_menu_entry(devopt_menu, &v90);

    v90 = debug_menu_entry { mString { "MONKEY_MODE" } };
    v90.set_ival(0);
    v90.set_min_value(-1000.0);
    v90.set_max_value(1000.0);
    v90.set_game_flags_handler(devopt_ints_handler);
    v90.set_id(9);
    add_debug_menu_entry(devopt_menu, &v90);

    v90 = debug_menu_entry { mString { "RANDOM_SEED" } };
    v90.set_ival(0);
    v90.set_min_value(-1000.0);
    v90.set_max_value(1000.0);
    v90.set_game_flags_handler(devopt_ints_handler);
    v90.set_id(10);
    add_debug_menu_entry(devopt_menu, &v90);

    v90 = debug_menu_entry { mString { "FORCE_WIN" } };
    v90.set_ival(0);
    v90.set_game_flags_handler(devopt_ints_handler);
    v90.set_min_value(-1000.0);
    v90.set_max_value(1000.0);
    v90.set_id(11);
    add_debug_menu_entry(devopt_menu, &v90);

    v90 = debug_menu_entry { mString { "CONTROLLER_TYPE" } };
    v90.set_ival(0);
    v90.set_min_value(-1000.0);
    v90.set_max_value(1000.0);
    v90.set_game_flags_handler(devopt_ints_handler);
    v90.set_id(12);
    add_debug_menu_entry(devopt_menu, &v90);

    v90 = debug_menu_entry { mString { "FRAME_LOCK" } };
    v90.set_ival(0);
    v90.set_min_value(-2.0);
    v90.set_max_value(10.0);
    v90.set_game_flags_handler(devopt_ints_handler);
    v90.set_id(13);
    add_debug_menu_entry(devopt_menu, &v90);

    v90 = debug_menu_entry { mString { "FRAME_LIMIT" } };
    v90.set_ival(0);
    v90.set_min_value(-1000.0);
    v90.set_max_value(1000.0);
    v90.set_game_flags_handler(devopt_ints_handler);
    v90.set_id(14);
    add_debug_menu_entry(devopt_menu, &v90);

    v90 = debug_menu_entry { mString { "SWING_DEBUG_TRAILS " } };
    v90.set_ival(0);
    v90.set_min_value(-1000.0);
    v90.set_max_value(1000.0);
    v90.set_game_flags_handler(devopt_ints_handler);
    v90.set_id(15);
    add_debug_menu_entry(devopt_menu, &v90);

    v90 = debug_menu_entry { mString { "SOAK_SMOKE" } };
    v90.set_ival(0);
    v90.set_min_value(-1000.0);
    v90.set_max_value(1000.0);
    v90.set_game_flags_handler(devopt_ints_handler);
    v90.set_id(16);
    add_debug_menu_entry(devopt_menu, &v90);

    v90 = debug_menu_entry { mString { "FAR_CLIP_PLANE" } };
    v90.set_ival(0);
    v90.set_min_value(-1000.0);
    v90.set_max_value(1000.0);
    v90.set_game_flags_handler(devopt_ints_handler);
    v90.set_id(17);

    add_debug_menu_entry(devopt_menu, &v90);
    v90 = debug_menu_entry { mString { "POI_DISPLAY_TYPE" } };
    v90.set_ival(0);
    v90.set_min_value(-1000.0);
    v90.set_max_value(1000.0);
    v90.set_game_flags_handler(devopt_ints_handler);
    v90.set_id(18);

    add_debug_menu_entry(devopt_menu, &v90);
    v90 = debug_menu_entry { mString { "STORY_MISSION" } };
    v90.set_ival(0);
    v90.set_min_value(-1000.0);
    v90.set_max_value(1000.0);
    v90.set_game_flags_handler(devopt_ints_handler);
    v90.set_id(19);

    add_debug_menu_entry(devopt_menu, &v90);
    v90 = debug_menu_entry { mString { "EXEC_DELAY" } };
    v90.set_ival(0);
    v90.set_min_value(-1000.0);
    v90.set_max_value(1000.0);
    v90.set_game_flags_handler(devopt_ints_handler);
    v90.set_id(20);

    add_debug_menu_entry(devopt_menu, &v90);
    v90 = debug_menu_entry { mString { "RUN_LENGTH" } };
    v90.set_ival(0);
    v90.set_min_value(-1000.0);
    v90.set_max_value(1000.0);
    v90.set_game_flags_handler(devopt_ints_handler);
    v90.set_id(21);

    add_debug_menu_entry(devopt_menu, &v90);
    v90 = debug_menu_entry { mString { "PC_WINDOW_TOP" } };
    v90.set_ival(0);
    v90.set_min_value(-1000.0);
    v90.set_max_value(1000.0);
    v90.set_game_flags_handler(devopt_ints_handler);
    v90.set_id(22);

    add_debug_menu_entry(devopt_menu, &v90);
    v90 = debug_menu_entry { mString { "PC_WINDOW_LEFT" } };
    v90.set_ival(0);
    v90.set_min_value(-1000.0);
    v90.set_max_value(1000.0);
    v90.set_game_flags_handler(devopt_ints_handler);
    v90.set_id(23);

    add_debug_menu_entry(devopt_menu, &v90);
    v90 = debug_menu_entry { mString { "PC_WINDOW_WIDTH" } };
    v90.set_ival(0);
    v90.set_min_value(-1000.0);
    v90.set_max_value(1000.0);
    v90.set_game_flags_handler(devopt_ints_handler);
    v90.set_id(24);
    add_debug_menu_entry(devopt_menu, &v90);

    v90 = debug_menu_entry { mString { "PC_WINDOW_HEIGHT" } };
    v90.set_ival(0);
    v90.set_min_value(-1000.0);
    v90.set_max_value(1000.0);
    v90.set_game_flags_handler(devopt_ints_handler);
    v90.set_id(25);
    add_debug_menu_entry(devopt_menu, &v90);

    v90 = debug_menu_entry { mString { "ALLOW_SCREENSHOT" } };
    v90.set_ival(0);
    v90.set_min_value(-1000.0);
    v90.set_max_value(1000.0);
    v90.set_game_flags_handler(devopt_ints_handler);
    v90.set_id(26);
    add_debug_menu_entry(devopt_menu, &v90);

    v90 = debug_menu_entry { mString { "AMALGA_REFRESH_INTERVAL" } };
    v90.set_ival(0);
    v90.set_min_value(-1000.0);
    v90.set_max_value(1000.0);
    v90.set_game_flags_handler(devopt_ints_handler);
    v90.set_id(27);
    add_debug_menu_entry(devopt_menu, &v90);

    v90 = debug_menu_entry { mString { "ENABLE_LONG_MALOR_ASSERTS" } };
    v90.set_ival(0);
    v90.set_min_value(-1000.0);
    v90.set_max_value(1000.0);
    v90.set_game_flags_handler(devopt_ints_handler);
    v90.set_id(28);
    add_debug_menu_entry(devopt_menu, &v90);

    v90 = debug_menu_entry { mString { "GOD_MODE" } };
    v90.set_ival(0);
    v90.set_min_value(-1000.0);
    v90.set_max_value(1000.0);
    v90.set_game_flags_handler(devopt_ints_handler);
    v90.set_id(29);
    add_debug_menu_entry(devopt_menu, &v90);

    v90 = debug_menu_entry { mString { "PCLISTBUFFER" } };
    v90.set_ival(0);
    v90.set_min_value(-1000.0);
    v90.set_max_value(1000.0);
    v90.set_game_flags_handler(devopt_ints_handler);
    v90.set_id(30);
    add_debug_menu_entry(devopt_menu, &v90);

    v90 = debug_menu_entry { mString { "PCSCRATCHBUFFER" } };
    v90.set_ival(0);
    v90.set_min_value(-1000.0);
    v90.set_max_value(1000.0);
    v90.set_game_flags_handler(devopt_ints_handler);
    v90.set_id(31);
    add_debug_menu_entry(devopt_menu, &v90);

    v90 = debug_menu_entry { mString { "PCSCRATCHINDEXBUFFER" } };
    v90.set_ival(0);
    v90.set_min_value(-1000.0);
    v90.set_max_value(1000.0);
    v90.set_game_flags_handler(devopt_ints_handler);
    v90.set_id(32);
    add_debug_menu_entry(devopt_menu, &v90);

    v90 = debug_menu_entry { mString { "PCSCRATCHVERTEXBUFFER" } };
    v90.set_ival(0);
    v90.set_min_value(-1000.0);
    v90.set_max_value(1000.0);
    v90.set_game_flags_handler(devopt_ints_handler);
    v90.set_id(33);
    add_debug_menu_entry(devopt_menu, &v90);

    v90 = debug_menu_entry { mString { "NAL_HEAP_SIZE" } };
    v90.set_ival(0);
    v90.set_min_value(-1000.0);
    v90.set_max_value(1000.0);
    v90.set_game_flags_handler(devopt_ints_handler);
    v90.set_id(34);
    add_debug_menu_entry(devopt_menu, &v90);

    v90 = debug_menu_entry { mString { "ASSERT_BOX_MARGIN" } };
    v90.set_ival(0);
    v90.set_min_value(-1000.0);
    v90.set_max_value(1000.0);
    v90.set_game_flags_handler(devopt_ints_handler);
    v90.set_id(35);
    add_debug_menu_entry(devopt_menu, &v90);

    v90 = debug_menu_entry { mString { "ASSERT_TEXT_MARGIN" } };
    v90.set_ival(0);
    v90.set_min_value(-1000.0);
    v90.set_max_value(1000.0);
    v90.set_game_flags_handler(devopt_ints_handler);
    v90.set_id(36);
    add_debug_menu_entry(devopt_menu, &v90);

    v90 = debug_menu_entry { mString { "ASSERT_FONT_PCT_X" } };
    v90.set_ival(0);
    v90.set_min_value(-1000.0);
    v90.set_max_value(1000.0);
    v90.set_game_flags_handler(devopt_ints_handler);
    v90.set_id(38);
    add_debug_menu_entry(devopt_menu, &v90);

    v90 = debug_menu_entry { mString { "ASSERT_FONT_PCT_Y" } };
    v90.set_ival(0);
    v90.set_min_value(-1000.0);
    v90.set_max_value(1000.0);
    v90.set_game_flags_handler(devopt_ints_handler);
    v90.set_id(39);
    add_debug_menu_entry(devopt_menu, &v90);

    v90 = debug_menu_entry { mString { "STREAMER_INFO_FONT_PCT" } };
    v90.set_ival(0);
    v90.set_min_value(-1000.0);
    v90.set_max_value(1000.0);
    v90.set_game_flags_handler(devopt_ints_handler);
    v90.set_id(40);
    add_debug_menu_entry(devopt_menu, &v90);

    v90 = debug_menu_entry { mString { "DEBUG_INFO_FONT_PCT" } };
    v90.set_ival(0);
    v90.set_min_value(-1000.0);
    v90.set_max_value(1000.0);
    v90.set_game_flags_handler(devopt_ints_handler);
    v90.set_id(40);
    add_debug_menu_entry(devopt_menu, &v90);

    v90 = debug_menu_entry { mString { "PITCH_FACTOR" } };
    v90.set_ival(0);
    v90.set_min_value(-1000.0);
    v90.set_max_value(1000.0);
    v90.set_game_flags_handler(devopt_ints_handler);
    v90.set_id(41);
    add_debug_menu_entry(devopt_menu, &v90);

    v90 = debug_menu_entry { mString { "BANK_FACTOR" } };
    v90.set_ival(0);
    v90.set_min_value(-1000.0);
    v90.set_max_value(1000.0);
    v90.set_game_flags_handler(devopt_ints_handler);
    v90.set_id(42);
    add_debug_menu_entry(devopt_menu, &v90);

    v90 = debug_menu_entry { mString { "SWING_INTERPOLATION_TIME" } };
    v90.set_ival(0);
    v90.set_min_value(-1000.0);
    v90.set_max_value(1000.0);
    v90.set_game_flags_handler(devopt_ints_handler);
    v90.set_id(43);
    add_debug_menu_entry(devopt_menu, &v90);

    v90 = debug_menu_entry { mString { "BOTH_HANDS_INTERPOLATION_TIME" } };
    v90.set_ival(0);
    v90.set_min_value(-1000.0);
    v90.set_max_value(1000.0);
    v90.set_game_flags_handler(devopt_ints_handler);
    v90.set_id(44);
    add_debug_menu_entry(devopt_menu, &v90);

    v90 = debug_menu_entry { mString { "MEM_DUMP_FRAME" } };
    v90.set_ival(0);
    v90.set_min_value(-1000.0);
    v90.set_max_value(1000.0);
    v90.set_game_flags_handler(devopt_ints_handler);
    v90.set_id(45);
    add_debug_menu_entry(devopt_menu, &v90);

    v90 = debug_menu_entry { mString { "HERO_START_X" } };
    v90.set_ival(0);
    v90.set_min_value(-1000.0);
    v90.set_max_value(1000.0);
    v90.set_game_flags_handler(devopt_ints_handler);
    v90.set_id(46);
    add_debug_menu_entry(devopt_menu, &v90);

    v90 = debug_menu_entry { mString { "HERO_START_Y" } };
    v90.set_ival(0);
    v90.set_min_value(-1000.0);
    v90.set_max_value(1000.0);
    v90.set_game_flags_handler(devopt_ints_handler);
    v90.set_id(47);
    add_debug_menu_entry(devopt_menu, &v90);

    v90 = debug_menu_entry { mString { "HERO_START_Z" } };
    v90.set_ival(0);
    v90.set_min_value(-1000.0);
    v90.set_max_value(1000.0);
    v90.set_game_flags_handler(devopt_ints_handler);
    v90.set_id(48);
    add_debug_menu_entry(devopt_menu, &v90);

    v90 = debug_menu_entry { mString { "SHOW_SOUND_INFO" } };
    v90.set_ival(0);
    v90.set_min_value(-1000.0);
    v90.set_max_value(1000.0);
    v90.set_game_flags_handler(devopt_ints_handler);
    v90.set_id(49);
    add_debug_menu_entry(devopt_menu, &v90);

    v90 = debug_menu_entry { mString { "SHOW_VOICE_BOX_INFO" } };
    v90.set_ival(0);
    v90.set_min_value(-1000.0);
    v90.set_max_value(1000.0);
    v90.set_game_flags_handler(devopt_ints_handler);
    v90.set_id(50);
    add_debug_menu_entry(devopt_menu, &v90);

    v90 = debug_menu_entry { mString { "DEBUG_CAMERA_PITCH_MULTIPLIER" } };
    v90.set_ival(0);
    v90.set_min_value(-1000.0);
    v90.set_max_value(1000.0);
    v90.set_game_flags_handler(devopt_ints_handler);
    v90.set_id(51);
    add_debug_menu_entry(devopt_menu, &v90);

    v90 = debug_menu_entry { mString { "DEBUG_CAMERA_YAW_MULTIPLIER" } };
    v90.set_ival(0);
    v90.set_min_value(-1000.0);
    v90.set_max_value(1000.0);
    v90.set_game_flags_handler(devopt_ints_handler);
    v90.set_id(52);
    add_debug_menu_entry(devopt_menu, &v90);

    v90 = debug_menu_entry { mString { "DEBUG_CAMERA_MOVE_MULTIPLIER" } };
    v90.set_ival(0);
    v90.set_min_value(-1000.0);
    v90.set_max_value(1000.0);
    v90.set_game_flags_handler(devopt_ints_handler);
    v90.set_id(53);
    add_debug_menu_entry(devopt_menu, &v90);

    v90 = debug_menu_entry { mString { "DEBUG_CAMERA_STRAFE_MULTIPLIER" } };
    v90.set_ival(0);
    v90.set_min_value(-1000.0);
    v90.set_max_value(1000.0);
    v90.set_game_flags_handler(devopt_ints_handler);
    v90.set_id(54);
    add_debug_menu_entry(devopt_menu, &v90);

    v90 = debug_menu_entry { mString { "TAM_SCALE_MIN_DISTANCE" } };
    v90.set_ival(0);
    v90.set_min_value(-1000.0);
    v90.set_max_value(1000.0);
    v90.set_game_flags_handler(devopt_ints_handler);
    v90.set_id(55);
    add_debug_menu_entry(devopt_menu, &v90);

    v90 = debug_menu_entry { mString { "TAM_SCALE_MAX_ DISTANCE" } };
    v90.set_ival(0);
    v90.set_min_value(-1000.0);
    v90.set_max_value(1000.0);
    v90.set_game_flags_handler(devopt_ints_handler);
    v90.set_id(56);
    add_debug_menu_entry(devopt_menu, &v90);

    v90 = debug_menu_entry { mString { "TAM_SCALE_MIN_PERCENT" } };
    v90.set_ival(0);
    v90.set_min_value(-1000.0);
    v90.set_max_value(1000.0);
    v90.set_game_flags_handler(devopt_ints_handler);
    v90.set_id(57);
    add_debug_menu_entry(devopt_menu, &v90);

    v90 = debug_menu_entry { mString { "THUG_HEALTH_UI_SCALE_MIN_DISTANCE" } };
    v90.set_ival(0);
    v90.set_min_value(-1000.0);
    v90.set_max_value(1000.0);
    v90.set_game_flags_handler(devopt_ints_handler);
    v90.set_id(58);
    add_debug_menu_entry(devopt_menu, &v90);

    v90 = debug_menu_entry { mString { "THUG_HEALTH_UI_SCALE_MAX_DISTANCE" } };
    v90.set_ival(0);
    v90.set_min_value(-1000.0);
    v90.set_max_value(1000.0);
    v90.set_game_flags_handler(devopt_ints_handler);
    v90.set_id(59);
    add_debug_menu_entry(devopt_menu, &v90);

    v90 = debug_menu_entry { mString { "THUG_HEALTH_UI_SCALE_MIN_PERCENT" } };
    v90.set_ival(0);
    v90.set_min_value(-1000.0);
    v90.set_max_value(1000.0);
    v90.set_game_flags_handler(devopt_ints_handler);
    v90.set_id(60);
    add_debug_menu_entry(devopt_menu, &v90);

    v90 = debug_menu_entry { mString { "TARGETING_RETICLE_SCALE_MIN_DISTANCE" } };
    v90.set_ival(0);
    v90.set_min_value(-1000.0);
    v90.set_max_value(1000.0);
    v90.set_game_flags_handler(devopt_ints_handler);
    v90.set_id(61);
    add_debug_menu_entry(devopt_menu, &v90);

    v90 = debug_menu_entry { mString { "TARGETING_RETICLE_SCALE_MAX_DISTANCE" } };
    v90.set_ival(0);
    v90.set_min_value(-1000.0);
    v90.set_max_value(1000.0);
    v90.set_game_flags_handler(devopt_ints_handler);
    v90.set_id(62);
    add_debug_menu_entry(devopt_menu, &v90);

    v90 = debug_menu_entry { mString { "TARGETING_RETICLE_SCALE_MIN_PERCENT" } };
    v90.set_ival(0);
    v90.set_min_value(-1000.0);
    v90.set_max_value(1000.0);
    v90.set_game_flags_handler(devopt_ints_handler);
    v90.set_id(63);
    add_debug_menu_entry(devopt_menu, &v90);

    v90 = debug_menu_entry { mString { "HIRES_SCREENSHOT_X" } };
    v90.set_ival(0);
    v90.set_min_value(-1000.0);
    v90.set_max_value(1000.0);
    v90.set_game_flags_handler(devopt_ints_handler);
    v90.set_id(64);
    add_debug_menu_entry(devopt_menu, &v90);

    v90 = debug_menu_entry { mString { "HIRES_SCREENSHOT_Y" } };
    v90.set_ival(0);
    v90.set_min_value(-1000.0);
    v90.set_max_value(1000.0);
    v90.set_game_flags_handler(devopt_ints_handler);
    v90.set_id(65);
    add_debug_menu_entry(devopt_menu, &v90);

    v90 = debug_menu_entry { mString { "TIME_OF_DAY" } };
    v90.set_ival(0);
    v90.set_max_value(3.0);
    v90.set_game_flags_handler(devopt_ints_handler);
    v90.set_id(66);
    add_debug_menu_entry(devopt_menu, &v90);

    v90 = debug_menu_entry { mString { "MINI_MAP_ZOOM" } };
    v90.set_ival(0);
    v90.set_max_value(28.0);
    v90.set_game_flags_handler(devopt_ints_handler);
    v90.set_id(67);
    add_debug_menu_entry(devopt_menu, &v90);

    v90 = debug_menu_entry { mString { "RTDT_REPLAY_BUFFER_SIZE" } };
    v90.set_ival(0);
    v90.set_min_value(-1000.0);
    v90.set_max_value(1000.0);
    v90.set_game_flags_handler(devopt_ints_handler);
    v90.set_id(68);
    add_debug_menu_entry(devopt_menu, &v90);

    v90 = debug_menu_entry { mString { "TIMER_WIDGET_TIME_DELTA_PERCENT" } };
    v90.set_ival(0);
    v90.set_min_value(-1000.0);
    v90.set_max_value(1000.0);
    v90.set_game_flags_handler(devopt_ints_handler);
    v90.set_id(69);
    add_debug_menu_entry(devopt_menu, &v90);

    v90 = debug_menu_entry { mString { "DEBUG_PARTICLE_LEVEL" } };
    v90.set_ival(0);
    v90.set_min_value(-1000.0);
    v90.set_max_value(1000.0);
    v90.set_game_flags_handler(devopt_ints_handler);
    v90.set_id(70);
    add_debug_menu_entry(devopt_menu, &v90);

    v90 = debug_menu_entry { mString { "DEBUG_PARTICLE_MEMORY" } };
    v90.set_ival(0);
    v90.set_min_value(-1000.0);
    v90.set_max_value(1000.0);
    v90.set_game_flags_handler(devopt_ints_handler);
    v90.set_id(71);
    add_debug_menu_entry(devopt_menu, &v90);

    v90 = debug_menu_entry { mString { "MAX_AEPS_ENTITIES" } };
    v90.set_ival(0);
    v90.set_min_value(-1000.0);
    v90.set_max_value(1000.0);
    v90.set_game_flags_handler(devopt_ints_handler);
    v90.set_id(72);
    add_debug_menu_entry(devopt_menu, &v90);

    v90 = debug_menu_entry { mString { "MAX_AEPS_SPAWNERS" } };
    v90.set_ival(0);
    v90.set_min_value(-1000.0);
    v90.set_max_value(1000.0);
    v90.set_game_flags_handler(devopt_ints_handler);
    v90.set_id(73);
    add_debug_menu_entry(devopt_menu, &v90);

    v90 = debug_menu_entry { mString { "MAX_AEPS_EMITTERS" } };
    v90.set_ival(0);
    v90.set_min_value(-1000.0);
    v90.set_max_value(1000.0);
    v90.set_game_flags_handler(devopt_ints_handler);
    v90.set_id(74);
    add_debug_menu_entry(devopt_menu, &v90);

    v90 = debug_menu_entry { mString { "MAX_AEPS_PARTICLES" } };
    v90.set_ival(0);
    v90.set_min_value(-1000.0);
    v90.set_max_value(1000.0);
    v90.set_game_flags_handler(devopt_ints_handler);
    v90.set_id(75);
    add_debug_menu_entry(devopt_menu, &v90);
        }





void create_game_flags_menu(debug_menu* parent)
{
    if (parent->used_slots != 0) {
        return;
    }

    assert(parent != nullptr);

    auto* v92 = parent;

    auto v89 = debug_menu_entry(mString { "Report SLF Recall Timeouts" });
    static bool byte_1597BC0 = false;
    v89.set_pt_bval(&byte_1597BC0);
    // v92->add_entry(&v89);

    v89 = debug_menu_entry(mString { "Physics Enabled" });
    v89.set_bval(true);
    v89.set_game_flags_handler(game_flags_handler);
    v89.set_id(0);
    v92->add_entry(&v89);

    v89 = debug_menu_entry(mString { "Single Step" });
    v89.set_game_flags_handler(game_flags_handler);
    v89.set_id(1);
    v92->add_entry(&v89);

    v89 = debug_menu_entry(mString { "Slow Motion Enabled" });
    v89.set_bval(false);
    v89.set_game_flags_handler(game_flags_handler);
    v89.set_id(2);
    v92->add_entry(&v89);

    v89 = debug_menu_entry { mString { "Monkey Enabled" } };

    auto v1 = spider_monkey::is_running();
    v89.set_bval(v1);
    v89.set_game_flags_handler(game_flags_handler);
    v89.set_id(3);
    v92->add_entry(&v89);

    v89 = debug_menu_entry { mString { "Rumble Enabled" } };
    v89.set_bval(true);
    v89.set_game_flags_handler(game_flags_handler);
    v89.set_id(4);
    v92->add_entry(&v89);

    v89 = debug_menu_entry(mString { "God Mode" });
    v89.set_ival(os_developer_options::instance()->get_int(mString { "GOD_MODE" }));

    const float v2[4] = { 0, 5, 1, 1 };
    v89.set_fl_values(v2);
    v89.set_game_flags_handler(game_flags_handler);
    v89.set_id(5);
    v92->add_entry(&v89);

    v89 = debug_menu_entry(mString { "Show Districts" });
    v89.set_bval(os_developer_options::instance()->get_flag(mString { "SHOW_STREAMER_INFO" }));
    v89.set_game_flags_handler(game_flags_handler);
    v89.set_id(6);
    v92->add_entry(&v89);

    v89 = debug_menu_entry(mString { "Show Hero Position" });
    v89.set_bval(os_developer_options::instance()->get_flag(mString { "SHOW_DEBUG_INFO" }));
    v89.set_game_flags_handler(game_flags_handler);
    v89.set_id(7);
    v92->add_entry(&v89);

    v89 = debug_menu_entry(mString { "Show FPS" });
    v89.set_bval(os_developer_options::instance()->get_flag(mString { "SHOW_FPS" }));
    v89.set_game_flags_handler(game_flags_handler);
    v89.set_id(8);
    v92->add_entry(&v89);

    v89 = debug_menu_entry(mString { "User Camera on Controller 2" });
    v89.set_bval(os_developer_options::instance()->get_flag(mString { "USERCAM_ON_CONTROLLER2" }));
    v89.set_game_flags_handler(game_flags_handler);
    v89.set_id(9);
    v92->add_entry(&v89);

    v89 = debug_menu_entry(mString { "Toggle Unload All Districts" });
    v89.set_game_flags_handler(game_flags_handler);
    v89.set_id(13);
    v92->add_entry(&v89);

    {
        auto* v88 = create_menu("Save/Load", handle_game_entry, 10);
        auto v18 = debug_menu_entry(v88);
        v92->add_entry(&v18);

        v89 = debug_menu_entry(mString { "Save Game" });
        v89.set_game_flags_handler(game_flags_handler);
        v89.set_id(14);
        v88->add_entry(&v89);

        v89 = debug_menu_entry(mString { "Load Game" });
        v89.set_game_flags_handler(game_flags_handler);
        v89.set_id(15);
        v88->add_entry(&v89);

        v89 = debug_menu_entry(mString { "Attemp Auto Load" });
        v89.set_game_flags_handler(game_flags_handler);
        v89.set_id(16);
        v88->add_entry(&v89);
    }

    {
        auto* v87 = create_menu("Screenshot", handle_game_entry, 10);
        auto v23 = debug_menu_entry(v87);
        v92->add_entry(&v23);

        v89 = debug_menu_entry(mString { "Hires Screenshot" });
        v89.set_game_flags_handler(game_flags_handler);
        v89.set_id(11);
        v87->add_entry(&v89);

        v89 = debug_menu_entry(mString { "Lores Screenshot" });
        v89.set_game_flags_handler(game_flags_handler);
        v89.set_id(12);
        v87->add_entry(&v89);
    }

    create_gamefile_menu(v92);
}







// Mocked functions
bool sub_6694CA(rtdt_replay_mgr* instance)
{
    g_game_ptr()->enable_physics(false);
    return true;
}

void sub_66ADD4(rtdt_replay_mgr* instance, int flag)
{
}


void replay_handler(debug_menu_entry* entry)
{

    if (entry->get_id() == 0) {
        auto instance = rtdt_replay_mgr::m_instance;


        if (sub_6694CA(instance)) {
            auto replay_instance = rtdt_replay_mgr::m_instance;


            sub_66ADD4(replay_instance, 1);

            g_game_ptr()->enable_physics(true); 
            debug_menu::physics_state_on_exit = true;
        }


        debug_menu::hide();
    }
}

void populate_replay_menu(debug_menu_entry* entry)
{

    auto* head_menu = create_menu("Replay", debug_menu::sort_mode_t::ascending);
    entry->set_submenu(head_menu);

    mString v25 { "Start" };
    debug_menu_entry v38 { v25.c_str() };

    v38.set_game_flags_handler(replay_handler);

    head_menu->add_entry(&v38);
}




void create_replay_menu(debug_menu* parent)
{
    auto* replay_menu = create_menu("Replay");
    auto* v2 = create_menu_entry(replay_menu);
    v2->set_game_flags_handler(populate_replay_menu);
    parent->add_entry(v2);
}

void movie_play_handler(debug_menu_entry* entry)
{

    if (!movie_manager::load_and_play_movie("Attract", "Attract", false)) {
        close_debug();
    }
}

void populate_movie_play_menu(debug_menu_entry* entry)
{

    entry->set_game_flags_handler(movie_play_handler);
}



void create_movie_play_menu(debug_menu* parent)
{
    auto* script_menu = create_menu("DEMO USM Trailer");
    auto* v2 = create_menu_entry(script_menu);
    v2->entry_type = UNDEFINED;
    v2->set_game_flags_handler(populate_movie_play_menu);
    v2->set_game_flags_handler(movie_play_handler);
    parent->add_entry(v2);
}

constexpr auto NUM_SCRIPTS = 39u;

const char* scripts[NUM_SCRIPTS] = {
    "ch_venom_viewer",
    "ch_vwr_sable",
    "ch_vwr_johnny_storm",
    "ch_vwr_gmu_cop_fat",
    "ch_vwr_gmu_sable_merc",
    "ch_vwr_shield_agent",
    "ch_vwr_gang_mercs_base",
    "ch_vwr_gang_mercs_boss",
    "ch_vwr_gang_mercs_lt",
    "ch_vwr_gang_ftb_base",
    "ch_vwr_gang_ftb_boss",
    "ch_vwr_gang_ftb_lt",
    "ch_gang_skin_base",
    "ch_vwr_gang_skin_boss",
    "ch_vwr_gang_skin_lt",
    "ch_vwr_gang_srk_base",
    "ch_vwr_gang_srk_boss",
    "ch_vwr_gang_srk_lt",
    "ch_vwr_electro_suit",
    "ch_vwr_electro_nosuit",
    "gmu_businessman",
    "gang_hellions",
    "ch_vwr_shocker",
    "ch_vwr_boomerang",
    "beetle_viewer",
    "ch_venom_eddie",
    "ch_venom_spider",
    "ultimate_spiderman_vwr",
    "ch_usm_blacksuit",
    "ch_vwr_green_goblin",
    "ch_vwr_carnage",
    "ch_mary_jane",
    "ch_vwr_rhino",
    "ch_gmu_child_male",
    "ch_gmu_cop_thin",
    "ch_gmu_man",
    "ch_gmu_woman",
    "ch_gmu_medic_fem",
    "gang1"

};

void script_toggle_handler(debug_menu_entry* entry)
{
    printf("script_toggle_handler\n");
    assert(entry->get_id() < NUM_SCRIPTS);
    hero_selected = entry->get_id();
    hero_status = hero_status_e::REMOVE_PLAYER;
}

void script_entry_callback(debug_menu_entry* entry)
{

    printf("hero_entry_callback: character_status = %d\n", hero_status);

    auto v18 = g_world_ptr()->num_players;
    switch (hero_status) {
    case hero_status_e::REMOVE_PLAYER: {
        auto v3 = entry->get_bval();
        g_world_ptr()->remove_player2(v18 - 1);
        hero_status = hero_status_e::ADD_PLAYER;
        frames_to_skip = 2;
        g_game_ptr()->enable_marky_cam(true, true, -1000.0, 0.0);
        break;
    }
    case hero_status_e::ADD_PLAYER: {
        auto v1 = frames_to_skip--;
        if (v1 <= 0) {
            assert(hero_selected > -1 && hero_selected < NUM_SCRIPTS);

            [[maybe_unused]] auto v2 = g_world_ptr()->add_player(mString { scripts[hero_selected] });

            /*
            auto v10 = v2 <= v18;

            assert(v10 && "Cannot add another_player");
            */

            g_game_ptr()->enable_marky_cam(false, true, -1000.0, 0.0);
            frames_to_skip = 2;
            hero_status = hero_status_e::CHANGE_HEALTH_TYPE;
        }
        break;
    }
    case hero_status_e::CHANGE_HEALTH_TYPE: {
        auto v3 = frames_to_skip--;
        if (v3 <= 0) {
            auto v17 = 0;
            auto* v5 = (actor*)g_world_ptr()->get_hero_ptr(0);
            auto* v6 = v5->get_player_controller();
            auto v9 = v6->field_420;
            switch (v9) {
            case 1:
                v17 = 0;
                break;
            case 2:
                v17 = 4;
                break;
            case 3:
                v17 = 5;
                break;
            }

            auto* v7 = g_world_ptr()->get_hero_ptr(0);
            auto v8 = v7->my_handle;
            g_femanager().IGO->hero_health->SetType(v17, v8.field_0);
            g_femanager().IGO->hero_health->SetShown(false);
            close_debug();
            hero_status = hero_status_e::UNDEFINED;
        }
        break;
    }
    default:
        break;
    }
}

void create_script_menu(debug_menu* script_menu)
{
    for (auto i = 0u; i < NUM_SCRIPTS; ++i) {
        auto v6 = 25;
        string_hash v5 { scripts[i] };
        auto v11 = resource_key { v5, v6 };
        auto v30 = resource_manager::get_pack_file_stats(v11, nullptr, nullptr, nullptr);
        if (v30) {
            mString v35 { scripts[i] };

            debug_menu_entry v37 { v35.c_str() };

            v37.set_game_flags_handler(script_toggle_handler);
            v37.m_id = i;
            v37.set_frame_advance_cb(script_entry_callback);
            script_menu->add_entry(&v37);
        }
    }
}

void create_entity_variants_menu(debug_menu* parent)
{
    auto* entity_variants_menu = create_menu("Entity Variants", debug_menu::sort_mode_t::undefined);
    auto* v1 = create_menu_entry(entity_variants_menu);
    parent->add_entry(v1);

    auto* entity_variants_menu0 = create_menu("0x00823d088", debug_menu::sort_mode_t::undefined);
    auto* v0 = create_menu_entry(entity_variants_menu0);
    entity_variants_menu->add_entry(v0);

    auto* entry = create_menu_entry(mString { "0xeb61a603" });
    entity_variants_menu0->add_entry(entry);

    auto* entry1 = create_menu_entry(mString { "0x98e9ec31" });
    entity_variants_menu0->add_entry(entry1);

    auto* entry2 = create_menu_entry(mString { "0x98eb8450" });
    entity_variants_menu0->add_entry(entry2);

    auto* entry3 = create_menu_entry(mString { "0x6a16b8a9" });
    entity_variants_menu0->add_entry(entry3);

    auto* entry4 = create_menu_entry(mString { "0x6a1650c8" });
    entity_variants_menu0->add_entry(entry4);

    auto* entity_variants_menu1 = create_menu("0x00823d089", debug_menu::sort_mode_t::undefined);
    auto* v3 = create_menu_entry(entity_variants_menu1);
    entity_variants_menu->add_entry(v3);

    auto* entry5 = create_menu_entry(mString { "0xeb61a303" });
    entity_variants_menu1->add_entry(entry5);

    auto* entry6 = create_menu_entry(mString { "0x435bbbac" });
    entity_variants_menu1->add_entry(entry6);

    auto* entry7 = create_menu_entry(mString { "0x435f580f" });
    entity_variants_menu1->add_entry(entry7);

    auto* entity_variants_menu2 = create_menu("0x00823d08a", debug_menu::sort_mode_t::undefined);
    auto* v4 = create_menu_entry(entity_variants_menu2);
    entity_variants_menu->add_entry(v4);

    auto* entry8 = create_menu_entry(mString { "0xeb61a303" });
    entity_variants_menu2->add_entry(entry8);

    auto* entry9 = create_menu_entry(mString { "0x435bbbac" });
    entity_variants_menu2->add_entry(entry9);

    auto* entry10 = create_menu_entry(mString { "0x435f580f" });
    entity_variants_menu2->add_entry(entry10);

    auto* entity_variants_menu3 = create_menu("0x00823d08b", debug_menu::sort_mode_t::undefined);
    auto* v5 = create_menu_entry(entity_variants_menu3);
    entity_variants_menu->add_entry(v5);

    auto* entry11 = create_menu_entry(mString { "0xeb61a303" });
    entity_variants_menu3->add_entry(entry11);

    auto* entry12 = create_menu_entry(mString { "0x435bbbac" });
    entity_variants_menu3->add_entry(entry12);

    auto* entry13 = create_menu_entry(mString { "0x435f580f" });
    entity_variants_menu3->add_entry(entry13);

    auto* entity_variants_menu4 = create_menu("0x00823d08c", debug_menu::sort_mode_t::undefined);
    auto* v6 = create_menu_entry(entity_variants_menu4);
    entity_variants_menu->add_entry(v6);

    auto* entry14 = create_menu_entry(mString { "0xeb61a603" });
    entity_variants_menu4->add_entry(entry14);

    auto* entry15 = create_menu_entry(mString { "0x98e9ec31" });
    entity_variants_menu4->add_entry(entry15);

    auto* entry16 = create_menu_entry(mString { "0x98eb8450" });
    entity_variants_menu4->add_entry(entry16);

    auto* entry17 = create_menu_entry(mString { "0x6a16b8a9" });
    entity_variants_menu4->add_entry(entry17);

    auto* entry18 = create_menu_entry(mString { "0x6a1650c8" });
    entity_variants_menu4->add_entry(entry18);

    auto* entity_variants_menu5 = create_menu("0x00823d08d", debug_menu::sort_mode_t::undefined);
    auto* v7 = create_menu_entry(entity_variants_menu5);
    entity_variants_menu->add_entry(v7);

    auto* entry19 = create_menu_entry(mString { "0xeb61a303" });
    entity_variants_menu5->add_entry(entry19);

    auto* entry20 = create_menu_entry(mString { "0x435bbbac" });
    entity_variants_menu5->add_entry(entry20);

    auto* entry21 = create_menu_entry(mString { "0x435f580f" });
    entity_variants_menu5->add_entry(entry21);

    auto* entity_variants_menu6 = create_menu("0x00823d08e", debug_menu::sort_mode_t::undefined);
    auto* v8 = create_menu_entry(entity_variants_menu6);
    entity_variants_menu->add_entry(v8);

    auto* entry22 = create_menu_entry(mString { "0xeb61a603" });
    entity_variants_menu6->add_entry(entry22);

    auto* entry23 = create_menu_entry(mString { "0x98e9ec31" });
    entity_variants_menu6->add_entry(entry23);

    auto* entry24 = create_menu_entry(mString { "0x98eb8450" });
    entity_variants_menu6->add_entry(entry24);

    auto* entry25 = create_menu_entry(mString { "0x6a16b8a9" });
    entity_variants_menu6->add_entry(entry25);

    auto* entry26 = create_menu_entry(mString { "0x6a1650c8" });
    entity_variants_menu6->add_entry(entry26);

    auto* entity_variants_menu7 = create_menu("0x00823d08f", debug_menu::sort_mode_t::undefined);
    auto* v9 = create_menu_entry(entity_variants_menu7);
    entity_variants_menu->add_entry(v9);

    auto* entry27 = create_menu_entry(mString { "0xeb61a303" });
    entity_variants_menu7->add_entry(entry27);

    auto* entry28 = create_menu_entry(mString { "0x435bbbac" });
    entity_variants_menu7->add_entry(entry28);

    auto* entry29 = create_menu_entry(mString { "0x435f580f" });
    entity_variants_menu7->add_entry(entry29);

    auto* entity_variants_menu8 = create_menu("0x00823d090", debug_menu::sort_mode_t::undefined);
    auto* v10 = create_menu_entry(entity_variants_menu8);
    entity_variants_menu->add_entry(v10);

    auto* entry30 = create_menu_entry(mString { "0xeb61a603" });
    entity_variants_menu8->add_entry(entry30);

    auto* entry31 = create_menu_entry(mString { "0x98e9ec31" });
    entity_variants_menu8->add_entry(entry31);

    auto* entry32 = create_menu_entry(mString { "0x98eb8450" });
    entity_variants_menu8->add_entry(entry32);

    auto* entry33 = create_menu_entry(mString { "0x6a16b8a9" });
    entity_variants_menu8->add_entry(entry33);

    auto* entry34 = create_menu_entry(mString { "0x6a1650c8" });
    entity_variants_menu8->add_entry(entry34);

    auto* entity_variants_menu9 = create_menu("0x00823d091", debug_menu::sort_mode_t::undefined);
    auto* v11 = create_menu_entry(entity_variants_menu9);
    entity_variants_menu->add_entry(v11);

    auto* entry35 = create_menu_entry(mString { "0xeb61a603" });
    entity_variants_menu9->add_entry(entry35);

    auto* entry36 = create_menu_entry(mString { "0x98e9ec31" });
    entity_variants_menu9->add_entry(entry36);

    auto* entry37 = create_menu_entry(mString { "0x98eb8450" });
    entity_variants_menu9->add_entry(entry37);

    auto* entry38 = create_menu_entry(mString { "0x6a16b8a9" });
    entity_variants_menu9->add_entry(entry38);

    auto* entry39 = create_menu_entry(mString { "0x6a1650c8" });
    entity_variants_menu9->add_entry(entry39);
}



void debug_menu::init() {

	root_menu = create_menu("Debug Menu", handle_debug_entry, 10);
    game_menu = create_menu("Game", handle_game_entry, 300);
    devopt_menu = create_menu("Devopts");
	script_menu = create_menu("Script");
    progression_menu = create_menu("Progression");
    level_select_menu = create_menu("Level Select");

    debug_menu_entry game_entry { game_menu };
    debug_menu_entry devopt_entry { devopt_menu };
	debug_menu_entry script_entry { script_menu };
	debug_menu_entry progression_entry { progression_menu };
	debug_menu_entry level_select_entry { level_select_menu };


    create_dvar_menu(root_menu);
    create_warp_menu(root_menu);
    create_game_flags_menu(game_menu);
    add_debug_menu_entry(root_menu, &game_entry);
    create_devopt_menu(devopt_menu);
    add_debug_menu_entry(game_menu, &devopt_entry);
	create_missions_menu(root_menu);
    create_debug_render_menu(root_menu);
    create_debug_district_variants_menu(root_menu);
    create_replay_menu(root_menu);
    create_ai_root_menu(root_menu);
    create_memory_menu(root_menu);
    create_entity_variants_menu(root_menu);
    create_entity_animation_menu(root_menu);
    add_debug_menu_entry(root_menu, &level_select_entry);
    add_debug_menu_entry(root_menu, &script_entry);
    add_debug_menu_entry(root_menu, &progression_entry);
    create_movie_play_menu(script_menu);
    create_script_menu(script_menu);
    create_camera_menu_items(root_menu);
	
    

	/*
	for (int i = 0; i < 5; i++) {

		debug_menu_entry asdf;
		sprintf(asdf.text, "entry %d", i);
		printf("AQUI %s\n", asdf.text);

		add_debug_menu_entry(debug_menu::root_menu, &asdf);
	}
	add_debug_menu_entry(debug_menu::root_menu, &teste);
	*/
}

BOOL WINAPI DllMain(HINSTANCE hInstDll, DWORD fdwReason, LPVOID reserverd) {

	if (sizeof(region) != 0x134) {
		__debugbreak();
	}

	memset(keys, 0, sizeof(keys));
	if (fdwReason == DLL_PROCESS_ATTACH) {
#if 0
		AllocConsole();

		if (!freopen("CONOUT$", "w", stdout)) {
			MessageBoxA(NULL, "Error", "Couldn't allocate console...Closing", 0);
			return FALSE;
		}
#endif

		set_text_writeable();
		set_rdata_writeable();
		install_patches();

	}
	else if (fdwReason == DLL_PROCESS_DETACH)
		FreeConsole();

	return TRUE;
}

int main() {
	return 0;
}
