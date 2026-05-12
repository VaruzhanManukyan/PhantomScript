#pragma once
#include "../ast_node.hpp"

struct ITypeNode : public IASTNode {
    virtual ~ITypeNode() = default;
};