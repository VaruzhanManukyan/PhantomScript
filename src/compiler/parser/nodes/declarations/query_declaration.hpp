#pragma once
#include <string>
#include <vector>

#include "function_declaration.hpp"

struct QueryDeclaration {
    std::string name_;
    std::vector<FunctionParameter> parameters_;
    std::unique_ptr<TypeNode> return_type_;
    std::string sql_query_;
};