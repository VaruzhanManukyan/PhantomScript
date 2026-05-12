#pragma once
#include <memory>
#include <vector>

#include "../ast_node.hpp"
#include "declaration.hpp"

struct ProgramNode : IASTNode {
    std::vector<std::unique_ptr<IDeclaration>> declarations_;

    explicit ProgramNode(std::vector<std::unique_ptr<IDeclaration>> declarations) :
        declarations_(std::move(declarations)) {}
    virtual ~ProgramNode() = default;
};