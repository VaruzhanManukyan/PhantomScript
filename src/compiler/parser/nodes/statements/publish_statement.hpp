#pragma once
#include <string>
#include <vector>

#include "statement.hpp"
#include "../expressions/struct_instantiation_expression.hpp"

class PublishStatement : public IStatement {
public:
    std::string event_name_;
    std::vector<StructInstantiationExpression::FieldInit> fields_;

    explicit PublishStatement(
        std::string event_name,
        std::vector<StructInstantiationExpression::FieldInit> fields,
        std::int32_t line,
        std::int32_t column) :
            event_name_(std::move(event_name)),
            fields_(std::move(fields)),
            IStatement(line, column) {}

    void accept(IAstVisitor& visitor) override {
        visitor.visit(*this);
    }

    virtual ~PublishStatement() = default;
};
