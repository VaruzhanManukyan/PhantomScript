#pragma once
#include <string>
#include <cstdint>
#include <memory>
#include <vector>

#include "database_declaration.hpp"
#include "declaration.hpp"
#include "group_node.hpp"
#include "route_node.hpp"

struct ServiceDeclaration : IDeclaration {
    std::string name_;

    std::string server_name_;
    std::string server_host_;
    std::int32_t server_port_;

    std::vector<std::unique_ptr<DatabaseDeclaration>> database_;
    std::vector<std::unique_ptr<RouteNode>> flat_routes_;
    std::vector<std::unique_ptr<GroupNode>> route_groups_;
    std::vector<std::string> published_events_;
};
