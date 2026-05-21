#pragma once
#include "../../../analyzer/ast_visitor.hpp"
#include "../statements/statement.hpp"

class IDeclaration : public IASTNode {
public:
    explicit IDeclaration(std::int32_t line, std::int32_t column) :
        IASTNode(line, column) {}
    virtual void accept(IAstVisitor& visitor) = 0;
    virtual ~IDeclaration() = default;
};