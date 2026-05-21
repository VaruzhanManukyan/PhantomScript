#pragma once
#include <string>
#include <memory>
#include <vector>
#include <cstdint>

#include "interface_type_node.hpp"

class TypeNode : public ITypeNode {
public:
    std::string base_name_;
    std::vector<std::unique_ptr<TypeNode>> generics_;

    explicit TypeNode(
        std::string base_name,
        std::int32_t line = 0,
        std::int32_t column = 0) :
            base_name_(std::move(base_name)),
            ITypeNode(line, column) {}
    virtual ~TypeNode() = default;
};