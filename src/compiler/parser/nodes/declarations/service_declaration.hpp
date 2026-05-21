#pragma once
#include <string>
#include <cstdint>
#include <memory>
#include <vector>

#include "database_declaration.hpp"
#include "declaration.hpp"
#include "group_node.hpp"
#include "route_node.hpp"

class ServiceDeclaration : public IDeclaration {
public:
    std::string name_;

    std::string server_name_;
    std::string server_host_;
    std::int32_t server_port_;

    std::vector<std::unique_ptr<DatabaseDeclaration>> database_;
    std::vector<std::unique_ptr<RouteNode>> flat_routes_;
    std::vector<std::unique_ptr<GroupNode>> route_groups_;
    std::vector<std::string> published_events_;

    explicit ServiceDeclaration(
        std::int32_t line,
        std::int32_t column) :
            server_port_(0),
            IDeclaration(line, column) {}

    void accept(IAstVisitor& visitor) override {
        visitor.visit(*this);
    }

    virtual ~ServiceDeclaration() = default;
};
