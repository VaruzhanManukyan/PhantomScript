#pragma once
#include <memory>
#include <vector>

#include "../ast_node.hpp"
#include "declaration.hpp"

class ProgramNode : public IASTNode {
public:
    std::vector<std::unique_ptr<IDeclaration>> declarations_;

    explicit ProgramNode(
        std::vector<std::unique_ptr<IDeclaration>> declarations,
        std::int32_t line,
        std::int32_t column) :
        declarations_(std::move(declarations)),
        IASTNode(line, column) {}

    void accept(IAstVisitor& visitor) {
        visitor.visit(*this);
    }

    virtual ~ProgramNode() = default;
};