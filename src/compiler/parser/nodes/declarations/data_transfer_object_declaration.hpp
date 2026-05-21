#pragma once
#include <string>
#include <vector>

#include "declaration.hpp"
#include "struct_declaration.hpp"

class DataTransferObjectDeclaration : public IDeclaration {
public:
    bool is_event_;
    std::string name_;
    std::vector<StructField> fields_;

    explicit DataTransferObjectDeclaration(
        std::int32_t line,
        std::int32_t column) :
            is_event_(false),
            IDeclaration(line, column) {}

    void accept(IAstVisitor& visitor) override {
        visitor.visit(*this);
    }

    virtual ~DataTransferObjectDeclaration() = default;
};
