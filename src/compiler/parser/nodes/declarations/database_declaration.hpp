#pragma once
#include <string>
#include <cstdint>

#include "declaration.hpp"

class DatabaseDeclaration : public IDeclaration {
public:
    std::string name_;
    std::string engine_;
    std::string host_;
    std::int32_t port_;
    std::string db_name_;

    explicit DatabaseDeclaration(
        std::int32_t line,
        std::int32_t column) :
            port_(0),
            IDeclaration(line, column) {}

    void accept(IAstVisitor& visitor) override {
        visitor.visit(*this);
    }

    virtual ~DatabaseDeclaration() = default;
};
