#pragma once
#include <memory>

#include "expression.hpp"

class MemberAccessExpression : public IExpression {
public:
    std::unique_ptr<IExpression> object_;
    std::string member_name_;

    explicit MemberAccessExpression(
        std::unique_ptr<IExpression> object,
        std::string member_name,
        std::int32_t line,
        std::int32_t column) :
            object_(std::move(object)),
            member_name_(std::move(member_name)),
            IExpression(line, column) {}

    void accept(IAstVisitor& visitor) override {
        visitor.visit(*this);
    }

    virtual ~MemberAccessExpression() = default;
};