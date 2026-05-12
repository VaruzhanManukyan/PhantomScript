#pragma once
#include <string>
#include <vector>

#include "statement.hpp"
#include "../expressions/struct_instantiation_expression.hpp"

struct PublishActionStatement : IStatement {
    std::string event_name_;
    std::vector<StructInstantiationExpression::FieldInit> fields_;

    PublishActionStatement(
        std::string event_name,
        std::vector<StructInstantiationExpression::FieldInit> fields) :
            event_name_(std::move(event_name)),
            fields_(std::move(fields)) {}
    virtual ~PublishActionStatement() = default;
};
