#pragma once

#include <vector>

struct patrol_def;
struct patrol_def_set;
struct string_hash;

struct wds_patrol_def_manager {
    std::vector<patrol_def *> field_0;

    wds_patrol_def_manager() = default;

    patrol_def * get_patrol_def(const string_hash &a2) const;

    void add_patrol_defs_from_set(patrol_def_set *a2);

    void remove_patrol_defs_from_set(patrol_def_set *a1);
};
