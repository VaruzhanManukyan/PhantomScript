#pragma once
#include <string>
#include <memory>
#include <vector>

#include "interface_type_node.hpp"

struct TypeNode : public ITypeNode {
    std::string base_name_;
    std::vector<std::unique_ptr<TypeNode>> generics_;

    explicit TypeNode(const std::string& base_name) :
        base_name_(std::move(base_name)) {}
    virtual ~TypeNode() = default;
};