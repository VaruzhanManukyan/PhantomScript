#pragma once
#include <string>
#include <vector>

#include "declaration.hpp"
#include "function_declaration.hpp"

class QueryDeclaration : public IDeclaration {
public:
    std::string name_;
    std::vector<FunctionParameter> parameters_;
    std::unique_ptr<TypeNode> return_type_;
    std::string sql_query_;

    explicit QueryDeclaration(
        std::int32_t line,
        std::int32_t column) :
            IDeclaration(line, column) {}

    void accept(IAstVisitor& visitor) override {
        visitor.visit(*this);
    }

    virtual ~QueryDeclaration() = default;
};