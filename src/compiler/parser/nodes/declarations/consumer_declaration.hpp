#pragma once
#include <string>

#include "declaration.hpp"

class ConsumerDeclaration : public IDeclaration {
public:
    std::string event_name_;
    std::string handler_function_name_;

    explicit ConsumerDeclaration(
        std::int32_t line,
        std::int32_t column) :
            IDeclaration(line, column) {}

    void accept(IAstVisitor& visitor) override {
        visitor.visit(*this);
    }

    virtual ~ConsumerDeclaration() = default;
};
