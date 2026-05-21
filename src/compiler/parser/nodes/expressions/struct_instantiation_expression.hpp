#pragma once
#include <string>
#include <memory>
#include <vector>

#include "expression.hpp"

class StructInstantiationExpression : public IExpression {
public:
    std::string struct_name_;
    struct FieldInit {
        std::string name_;
        std::unique_ptr<IExpression> value_;
    };
    std::vector<FieldInit> fields_;

    explicit StructInstantiationExpression(
        std::string struct_name,
        std::vector<FieldInit> fields,
        std::int32_t line,
        std::int32_t column) :
            struct_name_(std::move(struct_name)),
            fields_(std::move(fields)),
            IExpression(line, column) {}

    void accept(IAstVisitor& visitor) override {
        visitor.visit(*this);
    }

    virtual ~StructInstantiationExpression() = default;
};
