#pragma once
#include <string>
#include <vector>
#include <memory>

#include "route_node.hpp"

class GroupNode {
public:
    std::string base_name_;
    std::vector<std::unique_ptr<RouteNode>> routes_;
};
