#include "game.h"

#include "common.h"
#include "debug_menu.h"
#include "message_board.h"
#include "link_system.h"
#include "smoke_test.h"
#include "geometry_manager.h"
#include "debug_struct.h"
#include "keyboard.h"
#include "game_data_meat.h"
#include "game_button.h"
#include "entity.h"
#include "entity_base.h"
#include "po.h"

#include "resource_manager.h"



#include "spider_monkey.h"
#include "app.h"

#include "base_ai_core.h"

#include "color32.h"
#include "common.h"
#include "chuck_callbacks.h"


#include "physical_interface.h"
#include "region.h"

#include "resource_pack_streamer.h"
#include "resource_partition.h"
#include "script_manager.h"

#include "slc_manager.h"
#include "smoke_test.h"

#include "utility.h"
#include "variables.h"
#include "vector2di.h"
#include "vtbl.h"
#include "wds.h"

#include <cassert>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <numeric>

#include <algorithm>

VALIDATE_OFFSET(game, gamefile, 0xc0);
VALIDATE_OFFSET(game, m_user_camera_enabled, 0x172);
VALIDATE_OFFSET(game, field_280, 0x280);

static int main_flow[] = { 5, 6, 14 };

static game_process main_process { "main", main_flow, 3 };

static int start_flow[] = { 1, 2, 3, 4, 14 };
static game_process start_process { "start", start_flow, 5 };

static int pause_flow[] = { 7, 14 };
static game_process pause_process { "pause", pause_flow, 2 };

static int& g_debug_mem_dump_frame = var<int>(0x00921DCC);

static auto& off_921DAC = var<char* [1]>(0x00921DAC);

void game__setup_input_registrations(game* a1)
{
    if constexpr (0) {
    } else {
        CDECL_CALL(0x006063C0, a1);
    }
}

void game::setup_input_registrations()
{
    setup_input_registrations_p(this);
}

void game::setup_inputs()
{
    setup_inputs_p(this);
}

void map_spiderman_controls()
{
    auto* v1 = input_mgr::instance();
    auto id = v1->field_58;
    v1->map_control(96, id, 10);
    v1->map_control(98, id, 11);
    v1->map_control(97, id, 13);
    v1->map_control(99, id, 14);
    v1->map_control(100, id, 17);
    v1->map_control(101, id, 19);
    v1->map_control(102, id, 9);
    v1->map_control(103, id, 16);
    v1->map_control(104, id, 18);
    v1->map_control(105, id, 6);
    v1->map_control(106, id, 5);
    v1->map_control(107, id, 4);
    v1->map_control(108, id, 8);
    v1->map_control(109, id, 7);
    v1->map_control(110, id, 1);
    v1->map_control(111, id, 0);
    v1->map_control(112, id, 3);
    v1->map_control(113, id, 2);
    v1->map_control(112, id, 1);
    v1->map_control(113, id, 0);
    v1->map_control(114, id, 20);
    v1->map_control(115, id, 21);
}

void map_pc_2_playstation_inputs()
{
    auto* v0 = input_mgr::instance();
    auto id = v0->field_58;
    v0->map_control(80, id, 10);
    v0->map_control(81, id, 11);
    v0->map_control(82, id, 14);
    v0->map_control(83, id, 13);
    v0->map_control(84, id, 16);
    v0->map_control(85, id, 18);
    v0->map_control(86, id, 6);
    v0->map_control(87, id, 17);
    v0->map_control(88, id, 19);
    v0->map_control(89, id, 9);
    v0->map_control(90, id, 3);
    v0->map_control(91, id, 2);
    v0->map_control(90, id, 1);
    v0->map_control(91, id, 0);
    v0->map_control(94, id, 8);
    v0->map_control(95, id, 7);
    v0->map_control(92, id, 20);
    v0->map_control(93, id, 21);
}


void game__setup_inputs(game* a1)
{
    TRACE("game::setup_inputs");

    if constexpr (0) {
        auto* v2 = input_mgr::instance();
        v2->clear_mapping();
        auto id = v2->field_58;

        v2->map_control(1, id, 10);
        v2->map_control(2, id, 11);
        v2->map_control(3, id, 13);
        v2->map_control(4, id, 14);
        v2->map_control(5, id, 3);
        v2->map_control(6, id, 2);
        v2->map_control(5, id, 8);
        v2->map_control(6, id, 7);
        v2->map_control(61, id, 20);
        v2->map_control(67, id, 20);
        v2->map_control(63, id, 16);
        v2->map_control(64, id, 17);
        v2->map_control(65, id, 3);
        v2->map_control(66, id, 2);
        v2->map_control(61, id, 10);
        v2->map_control(62, id, 11);
        v2->map_control(62, id, 21);
        v2->map_control(70, id, 3);
        v2->map_control(71, id, 2);
        v2->map_control(78, id, 16);
        v2->map_control(79, id, 17);
        v2->map_control(70, id, 1);
        v2->map_control(71, id, 0);
        v2->map_control(68, id, 10);
        v2->map_control(69, id, 11);
        v2->map_control(69, id, 14);
        v2->map_control(72, id, 20);
        v2->map_control(73, id, 21);

        static constexpr device_id_t KEYBOARD_DEVICE = static_cast<device_id_t>(0x1E8480);

        {
            enum {
                EDITCAM_FORWARD = 14,
                EDITCAM_BACKWARD = 15,
            };

            v2->map_control(EDITCAM_FORWARD, KEYBOARD_DEVICE, KB_I);
            v2->map_control(EDITCAM_BACKWARD, KEYBOARD_DEVICE, KB_K);
        }

        if constexpr (0) {
            v2->map_control(74, KEYBOARD_DEVICE, 86);
            v2->map_control(75, KEYBOARD_DEVICE, 85);
            v2->map_control(76, KEYBOARD_DEVICE, 84);
            v2->map_control(77, KEYBOARD_DEVICE, 83);
            v2->map_control(68, KEYBOARD_DEVICE, 45);
            v2->map_control(69, KEYBOARD_DEVICE, 54);
            v2->map_control(53, KEYBOARD_DEVICE, 77);
            v2->map_control(10, KEYBOARD_DEVICE, 92);

            v2->map_control(16, KEYBOARD_DEVICE, 10);
            v2->map_control(17, KEYBOARD_DEVICE, 12);
            v2->map_control(16, KEYBOARD_DEVICE, 17);
            v2->map_control(17, KEYBOARD_DEVICE, 5);
            v2->map_control(18, KEYBOARD_DEVICE, 15);
            v2->map_control(19, KEYBOARD_DEVICE, 21);
            v2->map_control(18, KEYBOARD_DEVICE, 24);
            v2->map_control(19, KEYBOARD_DEVICE, 26);
            v2->map_control(22, KEYBOARD_DEVICE, 18);
            v2->map_control(23, KEYBOARD_DEVICE, 6);
            v2->map_control(24, KEYBOARD_DEVICE, 4);
            v2->map_control(25, KEYBOARD_DEVICE, 1);
            v2->map_control(26, KEYBOARD_DEVICE, 37);
            v2->map_control(26, KEYBOARD_DEVICE, 38);
            v2->map_control(27, KEYBOARD_DEVICE, 41);
            v2->map_control(27, KEYBOARD_DEVICE, 42);
            v2->map_control(29, KEYBOARD_DEVICE, 20);
            v2->map_control(30, KEYBOARD_DEVICE, 22);
            v2->map_control(31, KEYBOARD_DEVICE, 3);
            v2->map_control(32, KEYBOARD_DEVICE, 49);
            v2->map_control(28, KEYBOARD_DEVICE, 54);
            v2->map_control(33, KEYBOARD_DEVICE, 91);
            v2->map_control(34, KEYBOARD_DEVICE, 90);
            v2->map_control(35, KEYBOARD_DEVICE, 27);
            v2->map_control(36, KEYBOARD_DEVICE, 28);
            v2->map_control(37, KEYBOARD_DEVICE, 29);
            v2->map_control(38, KEYBOARD_DEVICE, 30);
            v2->map_control(39, KEYBOARD_DEVICE, 31);
            v2->map_control(40, KEYBOARD_DEVICE, 32);
            v2->map_control(41, KEYBOARD_DEVICE, 33);
            v2->map_control(42, KEYBOARD_DEVICE, 34);
            v2->map_control(43, KEYBOARD_DEVICE, 35);
            v2->map_control(44, KEYBOARD_DEVICE, 36);
            v2->map_control(45, KEYBOARD_DEVICE, 13);
            v2->map_control(46, KEYBOARD_DEVICE, 16);
            v2->map_control(47, KEYBOARD_DEVICE, 60);
            v2->map_control(48, KEYBOARD_DEVICE, 38);
            v2->map_control(49, KEYBOARD_DEVICE, 66);
            v2->map_control(50, KEYBOARD_DEVICE, 65);
            v2->map_control(51, KEYBOARD_DEVICE, 72);
            v2->map_control(7, KEYBOARD_DEVICE, 7);
            v2->map_control(8, KEYBOARD_DEVICE, 70);
            v2->map_control(9, KEYBOARD_DEVICE, 69);
            v2->map_control(119, KEYBOARD_DEVICE, 68);

            v2->map_control(116, KEYBOARD_DEVICE, 51);
            v2->map_control(117, KEYBOARD_DEVICE, 50);
            v2->map_control(118, KEYBOARD_DEVICE, 47);
        }

        {
            static constexpr device_id_t MOUSE_DEVICE = static_cast<device_id_t>(0x2DC6C0);
            v2->map_control(20, MOUSE_DEVICE, 1);
            v2->map_control(21, MOUSE_DEVICE, 0);
            v2->map_control(26, MOUSE_DEVICE, 3);
        }

        v2->map_control(54, id, 20);
        v2->map_control(54, id, 22);
        v2->map_control(55, id, 10);

        map_pc_2_playstation_inputs();

        map_spiderman_controls();
    } else {
        CDECL_CALL(0x00605950, a1);
    }
}




game::game()
{
    TRACE("game::game");

    if constexpr (1) {
        static auto& setup_inputs_p = var<void (*)(game*)>(0x0095C8FC);
        setup_inputs_p = game__setup_inputs;
        ;
    }

    if constexpr (1) {
        this->field_2B4 = false;
        this->field_1 = false;
        this->field_BC = false;
        float fov = os_developer_options::instance()->get_int(mString { "CAMERA_FOV" }) * 0.017453292;

        geometry_manager::set_field_of_view(fov);

        g_debug().field_0 |= 0x80u;
        if (os_developer_options::instance()->get_flag(mString { "OUTPUT_WARNING_DISABLE" })) {
            g_debug().field_0 &= 0x7Fu;
        }

        g_debug().field_1 |= 4u;
        if (os_developer_options::instance()->get_flag(mString { "OUTPUT_ASSERT_DISABLE" })) {
            g_debug().field_1 &= 0xFBu;
        }

        if (os_developer_options::instance()->get_flag(mString { "SMOKE_TEST" })) {
            float a3 = os_developer_options::instance()->get_int(mString { "SOAK_SMOKE" });
            {
            }

            static Var<char* [2]> smoke_test_levels { 0x00921DB0 };

            if (os_developer_options::instance()->get_flag(mString { "SMOKE_TEST_LEVEL" })) {
                g_smoke_test() = new smoke_test(bit_cast<const char**>(&g_scene_name), a3);
            } else {
                g_smoke_test() = new smoke_test(bit_cast<const char**>(&smoke_test_levels()[0]),
                    a3);

                strcpy(g_scene_name(), smoke_test_levels()[0]);
            }
        } else if (!os_developer_options::instance()->get_flag(mString { "SMOKE_TEST_LEVEL" })) {
            auto v23 = os_developer_options::instance()->get_string(os_developer_options::strings_t::SCENE_NAME);

            if (v23) {
                strcpy(g_scene_name(), v23->c_str());
            }
        }

        this->the_world = new world_dynamics_system();
        g_world_ptr() = this->the_world;

        this->mb = nullptr;

        this->gamefile = new game_settings();

        this->field_278 = 0.0;
        this->field_27C = 0;
        this->field_280 = 0;
        this->field_284 = 1.0e10;
        this->field_288 = 0.0;
        this->field_28C = 0.1;
        this->field_270 = 0;
        this->field_274 = 0;
        this->field_15E = 0;
        this->field_15F = 0;
        this->field_160 = 0;
        this->field_164 = false;
        this->field_165 = false;
        this->field_166 = false;
        this->field_163 = false;
        this->field_161 = false;
        this->field_162 = false;
        this->flag.level_is_loaded = 0;
        this->field_170 = 0;
        this->field_167 = 0;
        this->flag.single_step = false;
        this->flag.physics_enabled = false;
        this->flag.field_3 = false;
        this->flag.game_paused = false;
        this->field_16D = 0;
        this->field_16E = 0;
        this->m_hero_start_enabled = true;
        this->field_171 = 0;
        this->m_user_camera_enabled = false;
        this->field_2 = 0;
        this->field_148 = 0;
        this->field_149 = 0;
        this->field_13C = 9999.9502f;
        this->field_140 = 3.0;
        this->field_144 = -1;
        this->field_14A = 64;
        this->field_158 = 0;
        this->field_159 = 0;

        this->field_14C = 9999.9004f;
        this->field_150 = -3.0;
        this->field_154 = -1;
        this->field_15A = 64;
        this->field_58 = 0;
        this->current_game_camera = nullptr;
        this->field_5C = nullptr;
        this->field_64 = nullptr;
        this->field_7C = nullptr;

        this->setup_input_registrations();
        this->setup_inputs();

        static Var<bool> g_console_command { 0x0095C068 };
        g_console_command() = false;

        {
            this->push_process(main_process);
            this->push_process(start_process);
        }

        this->field_2B8 = 0;
        this->field_2BC = 0;

        this->field_15C = false;
        this->field_15D = false;

        this->field_2B5 = false;

        this->field_80 = {

            
        };



        g_debug_mem_dump_frame = os_developer_options::instance()->get_int(mString { "MEM_DUMP_FRAME" });

    } else {
        CDECL_CALL(0x00557610, this);
    }
}

mString sub_55DD80(const vector3d &a2)
{
    mString a1 {0, "<%.3f,%.3f,%.3f>", a2.x, a2.y, a2.z};
    return a1;
}

mString game::get_camera_info()
{
    auto *v2 = this->field_5C;

    mString v22;
    if ( v2->get_primary_region() != nullptr )
    {
        auto *v4 = v2->get_primary_region();
        auto &v5 = v4->get_name();
        auto *v6 = v5.to_string();
        v22 = mString{v6};
    }
    else
    {
        v22 = mString{"none"};
    }

    mString v33 = v22;


    auto &v18 = *v2->get_abs_position();
    auto *v8 = g_world_ptr()->the_terrain;
    auto *v32 = v8->find_region(v18, nullptr);
    if ( v32 != nullptr )
    {
        auto &v9 = v32->get_name();
        auto *v10 = v9.to_string();
        v33 = {v10};
    }

    auto &v12 = *v2->get_abs_position();
    auto v31 = sub_55DD80(v12);

    auto &v14 = *v2->get_abs_po();
    auto &v15 = v14.get_z_facing();

    auto v30 = sub_55DD80(v15);
    auto *v20 = v30.c_str();
    auto *v19 = v31.c_str();
    auto *v16 = v33.c_str();

    mString v29 {0, "CAMERA @ %s %s, f = %s", v16, v19, v20};

    auto v24 = " " + v33;

    v29 += v24;
    
    return v29;
}

mString game::get_analyzer_info()
{
    auto v16 = string_hash("SCENE_ANALYZER_CAM");
    auto *v3 = entity_handle_manager::find_entity(v16, entity_flavor_t::CAMERA, false);

    auto &v14 = *v3->get_abs_position();
    auto *v4 = g_world_ptr()->the_terrain;
    auto *v26 = v4->find_region(v14, nullptr);

    mString v25 {""};
    if ( v26 != nullptr )
    {
        auto &v5 = v26->get_name();
        auto *v6 = v5.to_string();
        v25 = v6;
    }

    auto &v8 = *v3->get_abs_position();
    auto v24 = sub_55DD80(v8);

    auto &v10 = *v3->get_abs_po();
    auto &v11 = v10.get_z_facing();
    auto v23 = sub_55DD80(v11);

    auto *v15 = v23.c_str();
    auto *v12 = v24.c_str();

    mString a1 {0, "ANALYZER @ %s, f = %s", v12, v15};
    auto v17 = " " + v25;
    a1 += v17;
    return a1;
}


mString game::get_hero_info()
{
    auto *hero_ptr = g_world_ptr()->get_hero_ptr(0);
    if ( hero_ptr == nullptr )
    {
        mString result {"(hero does not exist!)"};
        return result;
    }

    region *v29 = nullptr;
    if ( hero_ptr != nullptr )
    {
        v29 = hero_ptr->get_primary_region();
    }

    mString v28 {"none"};
    if ( v29 != nullptr )
    {
        auto &v4 = v29->get_name();
        auto *v5 = v4.to_string();
        v28 = {v5};
    }

    auto v27 = [&]() -> mString
    {
        if ( hero_ptr != nullptr )
        {
            auto &v6 = *hero_ptr->get_abs_position();
            return sub_55DD80(v6);
        }

        return mString{"N/A"};
    }();

    vector3d v15;
    if ( hero_ptr != nullptr )
    {
        auto *v7 = bit_cast<actor *>(hero_ptr)->physical_ifc();
        v15 = v7->get_velocity();
    }
    else
    {
        v15 = ZEROVEC;
    }

    mString v25{0, "HERO @ %s ", v28.c_str()};

    auto *v9 = v27.c_str();
    v25.append(v9, -1);
    v25.append(", v = ", -1);

    auto v14 = sub_55DD80(v15);
    v25.append(v14.c_str(), -1);

    return v25;
}

void game::show_debug_info()
{
    auto DEBUG_INFO_FONT_PCT = os_developer_options::instance()->get_int(mString{"DEBUG_INFO_FONT_PCT"});
    auto v15 = (float)DEBUG_INFO_FONT_PCT / 100.0;
    auto a1 = this->get_hero_info();

    vector2di v13 {50, 40};
    auto *v4 = a1.c_str();
    nglListAddString(nglSysFont(), (float)v13.x, (float)v13.y, 1.0, v15, v15, v4);

    auto v12 = this->get_camera_info();
    v13.y += 20;
    auto *v5 = v12.c_str();
    nglListAddString(nglSysFont(), (float)v13.x, (float)v13.y, 1.0, v15, v15, v5);

    auto v11 = this->get_analyzer_info();
    v13.y += 20;
    auto *v6 = v11.c_str();
    nglListAddString(nglSysFont(), (float)v13.x, (float)v13.y, 1.0, v15, v15, v6);
}

void game::frame_advance_level(Float time_inc)
{
    printf("game::frame_advance_level\n");

    {
        static int dword_14EEAC4 {-1};
        mem_check_leaks_since_checkpoint(dword_14EEAC4, 1);
        dword_14EEAC4 = mem_set_checkpoint();
        mem_print_stats("\nMemory log\n");
    }

    void (__fastcall *func)(void *, void *edx, Float) = (decltype(func)) 0x0055D650;
    func(this, nullptr, time_inc);
}

void game::handle_frame_locking(float* a1)
{
    auto frame_lock = os_developer_options::instance()->get_int(mString { "FRAME_LOCK" });
    if (frame_lock > 0.1) {
        *a1 = 2.0 / frame_lock;
    }
}

void game::set_camera(int camera_state)
{
    printf("camera_state = %d\n", camera_state);

    void (__fastcall *func)(void *, void *edx, int) = CAST(func, 0x0054F8C0);
    func(this, nullptr, camera_state);

    {
        if ( 1 == camera_state )
        {
            if ( g_debug_camera_entry != nullptr ) {
                g_debug_camera_entry->set_ival(true, true);
            }
        }
        else if ( 2 != camera_state )
        { 
            if ( g_debug_camera_entry != nullptr ) {
                g_debug_camera_entry->set_ival(false, true);
            }
        }
    }
}

void game::clear_screen()
{

        CDECL_CALL(0x00515140, this);
    }

static int& g_mem_checkpoint_level = var<int>(0x00921DC4);


void register_chuck_callbacks()
{

    CDECL_CALL(0x006607E0);
}

void game::load_this_level()
{
    TRACE("game::load_this_level");

    if constexpr (1) {
        assert(!flag.level_is_loaded);

        this->field_2C0.reset();
        this->field_15F = false;
        this->field_15E = false;
        this->field_160 = false;
        this->flag.game_paused = false;
        this->clear_screen();
        app::instance()->field_38 += 10;
        mem_print_stats("at beginning of load_this_level()");

        g_mem_checkpoint_level = 0; // mem_set_checkpoint()
        this->init_motion_blur();


        slc_manager::init();

             auto* common_partition = resource_manager::get_partition_pointer(4);
        assert(common_partition != nullptr);

        assert(common_partition->get_pack_slots().size() == 1);



        auto* common_slot = common_partition->get_pack_slots().front();
        assert(common_slot != nullptr);


        register_chuck_callbacks();
        script_manager::init_game_var();

        assert(script_manager::get_total_loaded() == 0);

        resource_key v14 = create_resource_key_from_path("init_gv", 0);
     {
            resource_key v69 {};

            resource_key v73 { string_hash { "init_gv" }, 15 };
            script_manager::load(v73, 0u, common_slot, v69);

            v73 = resource_key { string_hash { "init_sv" }, 15 };
            script_manager::load(v73, 0u, common_slot, v69);

            script_manager::link();
            script_manager::run(0.0, false);
            script_manager::clear();
            register_chuck_callbacks();

        mString hero_name { this->gamefile->field_340.field_114.to_string() };

        this->the_world->add_player(hero_name);

        po v86;

        if (g_game_ptr()->m_hero_start_enabled) {


            auto v73 = os_developer_options::instance()->get_string(os_developer_options::HERO_START_DISTRICT);
            if (v73) {
                auto* ter = g_world_ptr()->get_the_terrain();
                assert(ter != nullptr);

 

            auto hero_start_x = os_developer_options::instance()->get_int(mString { "HERO_START_X" });
                auto hero_start_y = os_developer_options::instance()->get_int(mString { "HERO_START_Y" });
            auto hero_start_z = os_developer_options::instance()->get_int(mString { "HERO_START_Z" });

  
            auto time = this->field_2C0.elapsed();
            printf("Post-load-scene load time: %f seconds\n", time);
        }


        this->current_game_camera = this->the_world->get_chase_cam_ptr(0);

        this->set_current_camera(this->the_world->get_chase_cam_ptr(0), true);


        if (this->field_16E) {
            this->freeze_hero(true);
        }

        spider_monkey::on_level_load();
        this->level.field_34.reset();

        mem_print_stats("at very end of load_this_level()");

    } else {
        CDECL_CALL(0x0055C6E0, this);
    }
        }
    }
}




bool game::is_physics_enabled() const
{
    return this->flag.physics_enabled;
}

bool game::is_paused() const
{
    return this->flag.game_paused;
}


bool game::level_is_loaded() const
{
    return this->flag.level_is_loaded;
}


inline double sub_A26B70()
{
    if (g_game_ptr()->field_290 == 0.0) {
        return 0.0;
    } else {
        return 1.0 / g_game_ptr()->field_290;
    }
}

inline double sub_690BF1()
{
    return sub_A26B70();
}

inline void nglSetQuadBlend(nglQuad* a1, nglBlendModeType a2, unsigned a3)
{
    a1->field_58 = a2;
    a1->field_5C = a3;
}


void game::render_bar_of_shame()
{
    if (os_developer_options::instance()->get_flag(mString { "SHOW_BAR_OF_SHAME" })) {
        auto v39 = g_game_ptr()->field_284 * 1000.0;

        int dword_1568744 = 0;
        if (v39 > dword_1568744) {
            dword_1568744 = v39;
        }

        color v38 { 0 };
        if (sub_690BF1() >= 25.0) {
            if (sub_690BF1() >= 30.0) {
                color v5 { 0.0, 1.0, 0.0, 1.0 };
                v38 = v5;
            } else {
                color v4 { 1.0, 1.0, 0.0, 1.0 };
                v38 = v4;
            }
        } else {
            color v3 { 0.0, 1.0, 0.0, 1.0 };
            v38 = v3;
        }

        nglQuad v1;
        nglInitQuad(&v1);
        nglSetQuadColor(&v1, (int)(v38.b * 255.0) | ((int)(v38.g * 255.0) << 8) | ((int)(v38.r * 255.0) << 16) | ((int)(v38.a * 255.0) << 24));
        nglSetQuadBlend(&v1, (nglBlendModeType)2, 0);
        nglSetQuadRect(&v1, 32.0, 16.0, (v39 * 3.0) + 32.0, 16.0 + 16.0);
        nglSetQuadZ(&v1, 1.0);
        nglListAddQuad(&v1);
    }
}
static Var<float> waiting_time { 0x00960B60 };

void game::advance_state_wait_link(Float a2)
{
    if constexpr (1) {
        if (!link_system::use_link_system() || !os_developer_options::instance()->get_flag(mString { "WAIT_FOR_LINK" })) {
            ++this->process_stack.back().index;
        }

        auto v2 = waiting_time() > 10.0f;

        if (v2 || this->field_171) {
            ++this->process_stack.back().index;
        }

        waiting_time() += a2;

    } else {
        CDECL_CALL(0x00536410, this, a2);
    }
}


 void game::show_max_velocity()
{
    if constexpr (1) {
    } else {
        CDECL_CALL(0x00514D00, this);
    }
}
void game::message_board_init()
{
    this->mb = new message_board {};
}



void game::_load_new_level(const mString& a2)
{
    if (!a2.empty()) {
        strcpy(g_scene_name(), a2.c_str());
    }

    this->field_15D = true;
    this->field_15C = false;
    this->field_167 = true;
}

void game::load_new_level(const mString& a2, const vector3d& a3)
{
    if constexpr (1) {
        this->_load_new_level(a2);
        this->level.field_24 = a3;
        this->m_hero_start_enabled = false;

    } else {
        CDECL_CALL(0x00514C40, this, &a2, &a3);
    }
}

void game::load_new_level(const mString& a1, int a2)
{
    if constexpr (1) {
        this->_load_new_level(a1);
        this->m_hero_start_enabled = true;

    } else {
        CDECL_CALL(0x00514C70, this, &a1, a2);
    }
}

void game::set_current_camera(camera* a2, bool a3)
{

    CDECL_CALL(0x00514FD0, this, a2, a3);
}

void game_patch()
{
    auto address = func_address(&game::frame_advance_level);
    REDIRECT(0x0055D8C2, address);

    {
        auto address = func_address(&game::set_camera);
        REDIRECT(0x00552FA3, address);
        REDIRECT(0x00552FC2, address);
    }

    {
        auto address = func_address(&game::message_board_init);
        REDIRECT(0x00552E7C, address);
    }
}
