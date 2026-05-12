#pragma once
#include <memory>

#include "expression.hpp"

struct MemberAccessExpression : IExpression {
    std::unique_ptr<IExpression> object_;
    std::string member_name_;

    MemberAccessExpression(
        std::unique_ptr<IExpression> object,
        std::string member_name) :
            object_(std::move(object)),
            member_name_(std::move(member_name)) {}
    virtual ~MemberAccessExpression() = default;
};