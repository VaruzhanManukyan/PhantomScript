#pragma once
#include <string>
#include <vector>

#include "declaration.hpp"
#include "struct_declaration.hpp"

struct DataTransferObjectDeclaration : IDeclaration {
    bool is_event_;
    std::string name_;
    std::vector<StructField> fields_;
};
