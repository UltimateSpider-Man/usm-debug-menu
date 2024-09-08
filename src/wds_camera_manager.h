#pragma once

#include "float.hpp"

#include <vector>

#include <cstdint>

struct marky_camera;
struct motion_control_system;
struct camera;
struct controller;

struct wds_camera_manager {
    std::vector<motion_control_system *> field_0;
    std::vector<controller *> field_10;
    camera *field_20;
    motion_control_system *field_24;
    motion_control_system *field_28;
    controller *field_2C;
    camera *field_30;
    motion_control_system *field_34;
    motion_control_system *field_38;
    controller *field_3C;
    camera *field_40;
    marky_camera *field_44;
    bool field_48;


    bool is_marky_cam_enabled() const {
        return this->field_48;
    }

};

