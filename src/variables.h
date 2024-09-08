#pragma once

#include "string_hash.h"
#include "variable.h"

#include <d3d9.h>

#include <windows.h>
inline Var<int> nglListWorkPos { 0x00971F0C };

inline Var<float> flt_87EBD4 = { 0x87EBD4 };

inline Var<float> flt_86F860 = { 0x86F860 };

inline Var<bool> s_freeze_game_time = { 0x009680AC };

inline Var<bool> g_generating_vtables = { 0x0095A6F1 };

inline Var<float> g_tan_half_fov_ratio { 0x00921D7C };

inline Var<bool> byte_965C20 { 0x00965C20 };

inline Var<bool> g_distance_clipping_enabled { 0x0095C718 };

inline Var<int> g_distance_clipping { 0x0095C2F8 };

inline Var<int> g_disable_occlusion_culling { 0x0095C7AC };

inline Var<int> globalTextLanguage { 0x0096B430 };

inline Var<HWND> g_appHwnd { 0x009874C4 };

inline Var<bool> g_is_the_packer { 0x009682E4 };

inline Var<bool> g_master_clock_is_up { 0x00965EB0 };

inline Var<int> g_TOD { 0x0091E000 };

inline Var<char* [14]> dword_965C24 { 0x00965C24 };

inline Var<char[1024]> g_scene_name = (0x0095C308);

inline Var<bool> bExit { 0x0095C178 };

inline Var<bool> byte_965BF9 = { 0x00965BF9 };

inline Var<HCURSOR> hCursor = { 0x0096596C };

inline Var<HANDLE> hEvent { 0x00965C5C };

inline Var<HANDLE> hObject { 0x00965C60 };

inline Var<bool> byte_965BF5 = { 0x00965BF5 };
inline Var<bool> byte_965BF6 = { 0x00965BF6 };

inline Var<int> dword_922908 = { 0x00922908 };

inline Var<bool> byte_922994 { 0x00922994 };

inline Var<IDirect3DDevice9*> g_Direct3DDevice = (0x00971F94);

inline Var<char> byte_965C21 = { 0x00965C21 };

inline Var<char> byte_965BF8 = { 0x00965BF8 };

inline Var<int> g_cx = { 0x00965978 };
inline Var<int> g_cy = { 0x00965968 };

inline Var<char*> dword_95C730 = { 0x0095C730 };
inline Var<char*> dword_95C72C = { 0x0095C72C };

inline Var<int> dword_91E1D8 { 0x0091E1D8 };

inline Var<bool> g_indoors { 0x0095C87B };

inline Var<bool> g_player_shadows_enabled = { 0x00922C5C };
inline Var<char> g_enable_stencil_shadows = { 0x00921C98 };

inline Var<int> g_cur_shadow_target = { 0x00965F44 };

inline Var<bool> byte_975468 { 0x00975468 };

inline Var<string_hash> bip01_l_calf = { 0x0095BA1C };
inline Var<string_hash> bip01_r_calf = { 0x0095AB1C };
inline Var<string_hash> bip01_pelvis = { 0x0095AAFC };
inline Var<string_hash> bip01_head = { 0x0095B9DC };
inline Var<string_hash> bip01_spine = { 0x0095BA18 };

inline Var<char[260]> byte_9659B8 { 0x009659B8 };

inline Var<int> nWidth = { 0x0093AE84 };
inline Var<int> nHeight = { 0x0093AE88 };

inline Var<HWND> g_hWnd = { 0x00971F98 };

inline Var<float> flt_965BDC { 0x00965BDC };

inline Var<char> byte_95C718 = { 0x0095C718 };
inline Var<int> dword_95C2F8 = { 0x0095C2F8 };

inline Var<bool> ChromeEffect = { 0x0091E1D4 };

inline Var<int> g_Windowed = { 0x00946530 };

inline Var<float> flt_88E518 { 0x0088E518 };

inline Var<float> flt_88E51C { 0x0088E51C };

inline Var<bool> cam_target_locked { 0x0095C754 };

inline Var<bool> EnableShader { 0x00972AB0 };

inline constexpr uint32_t RESOURCE_VERSION_INVALID = 0xFFFFFFFF;

inline constexpr uint32_t RESOURCE_PACK_VERSION = 0xE;
inline constexpr uint32_t RESOURCE_ENTITY_MASH_VERSION = 0x24D;
inline constexpr uint32_t RESOURCE_NONENTITY_MASH_VERSION = 0x12D;
inline constexpr uint32_t RESOURCE_AUTO_MASH_VERSION = 0x249;
inline constexpr uint32_t RESOURCE_RAW_MASH_VERSION = 0x115;
