#pragma once
#include <memory>
#include <string>

#include "declaration.hpp"
#include "../types/type_node.hpp"

struct StructField {
    std::string name_;
    std::unique_ptr<TypeNode> return_type_;
};

class StructDeclaration : public IDeclaration {
public:
    std::string name_;
    std::vector<StructField> fields_;

    explicit StructDeclaration(
        std::int32_t line,
        std::int32_t column) :
            IDeclaration(line, column) {}

    void accept(IAstVisitor& visitor) override {
        visitor.visit(*this);
    }

    virtual ~StructDeclaration() = default;
};