#pragma once
#include "../ast_node.hpp"

class ITypeNode : public IASTNode {
public:
    explicit ITypeNode(std::int32_t line, std::int32_t column)
        : IASTNode(line, column) {}
    virtual ~ITypeNode() = default;
};