#pragma once

#include <vector>

struct path_graph;
struct resource_key;

struct wds_ai_manager {
    std::vector<path_graph *> path_graph_list;

    wds_ai_manager();

    //0x0053CE80
    path_graph *get_path_graph(const resource_key &id);

    //0x00537330
    void add_path_graph(path_graph *pg);
};
