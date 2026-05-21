#pragma once
#include "../../../analyzer/ast_visitor.hpp"
#include "../ast_node.hpp"

class IExpression : public IASTNode {
public:
    explicit IExpression(std::int32_t line, std::int32_t column) :
        IASTNode(line, column) {}
    virtual void accept(IAstVisitor& visitor) = 0;
    virtual ~IExpression() = default;
};