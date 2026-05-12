#pragma once
#include <memory>
#include <string>

#include "declaration.hpp"
#include "../types/type_node.hpp"

struct StructField {
    std::string name_;
    std::unique_ptr<TypeNode> return_type_;
};

struct StructDeclaration : IDeclaration {
    std::string name_;
    std::vector<StructField> fields_;
};