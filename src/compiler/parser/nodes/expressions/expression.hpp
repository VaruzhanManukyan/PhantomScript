#pragma once
#include "../ast_node.hpp"

struct  IExpression : IASTNode {
    virtual ~IExpression() = default;
};