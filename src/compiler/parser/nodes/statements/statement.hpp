#pragma once
#include "../ast_node.hpp"

struct IStatement : IASTNode {
    virtual ~IStatement() = default;
};