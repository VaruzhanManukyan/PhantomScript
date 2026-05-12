#pragma once
#include <string>
#include <vector>

#include "route_node.hpp"

struct GroupNode {
    std::string base_name_;
    std::vector<RouteNode> routes_;
};
